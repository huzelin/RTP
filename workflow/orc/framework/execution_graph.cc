#include "orc/framework/execution_graph.h"
#include "orc/framework/execution_node.h"

#include "orc/util/log.h"

namespace orc {

std::unique_ptr<ExecutionGraph> ExecutionGraph::Clone() const {
  auto graph = std::unique_ptr<ExecutionGraph>(new ExecutionGraph(name_));
  for (const auto& entry : nodes_) {
    auto node = entry.second->Clone();
    if (!node) {
      ORC_ERROR("Clone fail for can't node: %s clone fail.", entry.first.c_str());
      return nullptr;
    }
    graph->nodes_.emplace(entry.first, std::move(node));
  }

  for (const auto& entry : nodes_) {
    auto true_next = entry.second->true_next();
    auto false_next = entry.second->false_next();

    if (true_next == ExecutionNode::EndNode()) {
      graph->nodes_[entry.first]->set_true_next(ExecutionNode::EndNode());
    } else {
      graph->nodes_[entry.first]->set_true_next(graph->nodes_[true_next->name()].get());
    }

    if (false_next == ExecutionNode::EndNode()) {
      graph->nodes_[entry.first]->set_false_next(ExecutionNode::EndNode());
    } else {
      graph->nodes_[entry.first]->set_false_next(graph->nodes_[false_next->name()].get());
    }
  }

  graph->version_ = version_;
  graph->entry_ = graph->nodes_[entry_->name()].get();
  return graph;
}

void ExecutionGraph::AddNode(const std::string& name,
                             std::unique_ptr<ExecutionNode> node) {
  nodes_[name] = std::move(node);
}

ExecutionNode* ExecutionGraph::GetNode(const std::string& name) const {
  auto it = nodes_.find(name);
  if (it == nodes_.end()) return nullptr;
  return it->second.get();
}

}  // namespace orc
