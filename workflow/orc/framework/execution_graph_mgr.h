#ifndef ORC_FRAMEWORK_EXECUTION_GRAPH_MGR_H_
#define ORC_FRAMEWORK_EXECUTION_GRAPH_MGR_H_

#include <string>
#include <memory>
#include <queue>
#include <mutex>

#include "orc/util/macros.h"
#include "orc/workflow/graph.h"
#include "orc/framework/execution_graph.h"
#include "yaml-cpp/yaml.h"

namespace orc {

class ExecutionGraphMgr {
 public:
  ~ExecutionGraphMgr() = default;

  static ExecutionGraphMgr* Instance();

  bool Setup(const YAML::Node& config);

  ExecutionGraph* GetExeGraph(const std::string& name);
  void ReleaseExeGraph(ExecutionGraph* graph);

  using GraphsMap = std::map<std::string, std::unique_ptr<wf::Graph>>;
  using ExeGraphsMap = std::map<std::string, std::queue<std::unique_ptr<ExecutionGraph>>>;

 private:
  ExecutionGraphMgr();

  enum class Mode { Pool, Singleton };

  bool ReloadGraph(Mode mode, size_t pool_size, bool use_tls,
                   size_t tls_size, const std::string& file);

  bool ReloadExeGraph(uint32_t version, size_t pool_size,
                      const GraphsMap& graphs, ExeGraphsMap* exe_graphs);

  uint32_t version() const { return version_; }

  std::unique_ptr<ExecutionGraph> BuildGraph(const wf::Graph* graph);
  ExecutionNode* BuildNode(const wf::Node* node, ExecutionGraph* exe_graph);

  ExecutionGraph* GetExeGraphFromPool(const std::string& name, ExeGraphsMap* graph_map, int idx);
  ExecutionGraph* GetExeGraphFromSingleton(const std::string& name, ExeGraphsMap* graph_map);

 private:
  Mode mode_;
  size_t pool_size_;
  size_t tls_size_;
  bool use_tls_;

  std::string src_file_;
  std::atomic<uint32_t> version_;

  std::mutex mutex_;
  ExeGraphsMap exe_graphs_;

  thread_local static ExeGraphsMap  tls_exe_graphs_;

  ORC_DISALLOW_COPY_AND_ASSIGN(ExecutionGraphMgr);
};

}  // namespace orc

#endif  // ORC_FRAMEWORK_EXECUTION_GRAPH_MGR_H_
