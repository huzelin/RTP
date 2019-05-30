#include "orc/framework/execution_graph_mgr.h"

#include <map>
#include <string>
#include <memory>

#include "orc/workflow/graph.h"
#include "orc/workflow/compiler.h"
#include "orc/util/log.h"
#include "orc/util/utils.h"

#include "orc/framework/handler_mgr.h"
#include "orc/framework/configure.h"
#include "common/monitor/monitor_status_impl.h"

namespace orc {

thread_local ExecutionGraphMgr::ExeGraphsMap ExecutionGraphMgr::tls_exe_graphs_;

ExecutionGraphMgr::ExecutionGraphMgr()
    : version_(0) {}

ExecutionGraphMgr* ExecutionGraphMgr::Instance() {
  static ExecutionGraphMgr mgr;
  return &mgr;
}

bool ExecutionGraphMgr::Setup(const YAML::Node& config) {
  Mode mode;
  size_t pool_size;

  std::string mode_str;
  ORC_CONFIG_OR_FAIL(config, Options::SvrWorkflowPoolMode, mode_str);
  if (mode_str == "singleton") {
    mode = Mode::Singleton;
    pool_size = 1;
  } else if (mode_str == "pool") {
    mode = Mode::Pool;
    if (!GetOrcConfig(config, Options::SvrWorkflowPoolSize, &pool_size)) {
      if (!GetOrcConfig(config, Options::SvrSessionPoolSize, &pool_size)) {
        ORC_ERROR("Not found config item: %s.", Options::SvrWorkflowPoolSize.c_str());
        return false;
      }
    }
  } else {
    ORC_ERROR("Unkown value for option '%s': %s, which should be one of these: 'pool, singleton'.",
              Options::SvrWorkflowPoolMode.c_str(), mode_str.c_str());
    return false;
  }

  std::string file;
  ORC_CONFIG_OR_FAIL(config, Options::SvrWorkflowFile, file);

  size_t worker_num;
  ORC_CONFIG_OR_FAIL(config, Options::SvrWorkerNum, worker_num);
  size_t tls_size = pool_size / worker_num;


  bool use_tls = false;
  ORC_CONFIG_OR_DEFAULT(config, Options::SvrWorkflowUseTls, use_tls, false);

  return ReloadGraph(mode, pool_size, use_tls, tls_size, file);
}

bool ExecutionGraphMgr::ReloadGraph(Mode mode, size_t pool_size, bool use_tls,
                                    size_t tls_size, const std::string& file) {
  GraphsMap graphs;
  wf::Compiler compiler;

  if (!compiler.Compile(file, &graphs)) {
    ORC_ERROR("Setup fail for Compile file: %s error.", file.c_str());
    return false;
  }

  ExeGraphsMap exe_graphs;
  uint32_t version = version_.load() + 1;

  if (!ReloadExeGraph(version, pool_size, graphs, &exe_graphs)) {
    ORC_ERROR("RloadExeGraph fail for config file: %s.", file.c_str());
    return false;
  }

  {
    std::lock_guard<std::mutex> lock(mutex_);
    mode_ = mode;
    use_tls_ = use_tls;
    pool_size_ = pool_size;
    tls_size_ = tls_size;
    src_file_ = file;
    version_.store(version);
    exe_graphs_.swap(exe_graphs);
  }

  return true;
}

bool ExecutionGraphMgr::ReloadExeGraph(
    uint32_t version, size_t pool_size,
    const ExecutionGraphMgr::GraphsMap& graphs,
    ExecutionGraphMgr::ExeGraphsMap* exe_graphs) {
  for (const auto& p : graphs) {
    auto exe_graph = BuildGraph(p.second.get());
    if (!exe_graph) {
      ORC_ERROR("BuildGraph: %s error.", p.first.c_str());
      return false;
    }
    exe_graph->set_version(version);

    for (uint32_t i = 1; i < pool_size; ++i) {
      auto cl_graph = exe_graph->Clone();
      if (!cl_graph) {
        return false;
      }
      (*exe_graphs)[p.first].emplace(std::move(cl_graph));
    }

    (*exe_graphs)[p.first].emplace(std::move(exe_graph));
  }

  return true;
}

ExecutionNode* ExecutionGraphMgr::BuildNode(const wf::Node* node, ExecutionGraph* exe_graph) {
  if (node == wf::Node::EndNode()) return ExecutionNode::EndNode();

  // If this node has been built already.
  auto already_node = exe_graph->GetNode(node->name());
  if (already_node != nullptr) return already_node;

  // If this node hasn't been built.
  std::unique_ptr<ExecutionNode> exe_node{new ExecutionNode(node->name())};

  auto handler = HandlerMgr::Instance()->GetHandler(node->name());
  if (!handler) {
    ORC_ERROR("BuildNode fail for can't get handler by name: %s", node->name().c_str());
    return nullptr;
  }

  auto r = exe_node.get();
  exe_graph->AddNode(node->name(), std::move(exe_node));

  auto true_next = BuildNode(node->true_edge(), exe_graph);
  if (true_next == nullptr) return nullptr;

  auto false_next = BuildNode(node->false_edge(), exe_graph);
  if (false_next == nullptr) return nullptr;

  r->set_handler(std::move(handler));
  r->set_true_next(true_next);
  r->set_false_next(false_next);

  return r;
}

std::unique_ptr<ExecutionGraph> ExecutionGraphMgr::BuildGraph(const wf::Graph* graph) {
  std::unique_ptr<ExecutionGraph> exe_graph{new ExecutionGraph(graph->name())};
  auto entry = BuildNode(graph->entry(), exe_graph.get());
  if (entry == nullptr) {
    ORC_ERROR("BuildGraph: %s fail.", graph->name().c_str());
    return nullptr;
  }

  exe_graph->set_entry(entry);
  return exe_graph;
}

ExecutionGraph* ExecutionGraphMgr::GetExeGraphFromPool(
    const std::string& name, ExecutionGraphMgr::ExeGraphsMap* graphs_map, int idx) {
  auto it = graphs_map->find(name);

  if (it == graphs_map->end()) return nullptr;

  MONITOR_STATUS_NORMAL_TIMER_BY("ExeGraphsMap",  ((idx == 1) ? "tls" : "glb"), it->second.size() * 1000, 1);

  if (it->second.empty()) return nullptr;

  auto p_graph = std::move(it->second.front());
  it->second.pop();

  return p_graph.release();
}

ExecutionGraph* ExecutionGraphMgr::GetExeGraphFromSingleton(
    const std::string& name, ExecutionGraphMgr::ExeGraphsMap* graphs_map) {
  auto it = graphs_map->find(name);

  if (it == graphs_map->end()) return nullptr;
  if (it->second.empty()) return nullptr;

  return it->second.front().get();
}

ExecutionGraph* ExecutionGraphMgr::GetExeGraph(const std::string& name) {
  if (mode_ == Mode::Singleton) {
    // Singleton mode, ExecutionGraph is thread-safe, all worker threads use the
    // same ExecutionGraph. There is no lock, so the 'hot reload' feature is not
    // supported.
    return GetExeGraphFromSingleton(name, &exe_graphs_);
  } else {
    if (use_tls_) {
      auto graph = GetExeGraphFromPool(name, &tls_exe_graphs_, 1);
      if (graph) return graph;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    return GetExeGraphFromPool(name, &exe_graphs_, 0);
  }
}

void ExecutionGraphMgr::ReleaseExeGraph(ExecutionGraph* graph) {
  if (mode_ == Mode::Singleton) {
    // For graph is singleton, no need to Release.
  } else {
    if (graph->version() != version()) {
      delete graph;
      return;
    }

    if (use_tls_) {
      if (tls_exe_graphs_[graph->name()].size() < tls_size_) {
        tls_exe_graphs_[graph->name()].emplace(graph);
        return;
      }
    }

    std::lock_guard<std::mutex> lock(mutex_);
    exe_graphs_[graph->name()].emplace(graph);
  }
}

}  // namespace orc
