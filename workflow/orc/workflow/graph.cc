#include "orc/workflow/graph.h"

#include "orc/util/log.h"

namespace orc {
namespace wf {

void Graph::AddNode(std::unique_ptr<Node> node) {
  nodes_.emplace(std::move(node));
}

bool Graph::CheckNode(Node* node, std::set<Node*>* checked_nodes) const {
  if (node->true_edge() == nullptr || node->false_edge() == nullptr) {
    ORC_ERROR("Node %s has not two edge.", node->name().c_str());
    return false;
  }

  if (checked_nodes->find(node) != checked_nodes->end()) {
    return true;
  }

  checked_nodes->emplace(node);

  if ((node->true_edge() != Node::EndNode()) &&
      (!CheckNode(node->true_edge(), checked_nodes))) {
    return false;
  }

  if ((node->false_edge() != Node::EndNode()) &&
      (!CheckNode(node->false_edge(), checked_nodes))) {
    return false;
  }

  return true;
}

bool Graph::CheckEntry() const {
  for (const auto& n : nodes_) {
    if (n.get() == entry_) {
      return true;
    }
  }
  return false;
}

bool Graph::CheckValid() const {
  if ((entry_ == nullptr)) {
    ORC_ERROR("Graph %s entry is not set.", name_.c_str());
    return nodes_.empty();
  }

  std::set<Node*> checked_nodes;
  if (!CheckNode(entry_, &checked_nodes)) {
    ORC_ERROR("Graph %s has invalid nodes.", name_.c_str());
    return false;
  }

  // TODO(qingmeng.wyh): Strict verification should check if the elements in
  // each sets are the same.
  if (checked_nodes.size() != nodes_.size()) {
    ORC_ERROR("Graph %s has isolated nodes link: %zd, total: %zd.",
              name_.c_str(), checked_nodes.size(), nodes_.size());
    return false;
  }

  return CheckEntry();
}

void Graph::TraverseNode(Node* node, const std::function<void(Node*)>& f,
                         std::set<Node*>* checked_nodes) const {
  if (checked_nodes->find(node) != checked_nodes->end()) {
    return;
  }

  checked_nodes->emplace(node);

  f(node);

  if (node->true_edge() != Node::EndNode()) {
    TraverseNode(node->true_edge(), f, checked_nodes);
  }

  if (node->false_edge() != Node::EndNode()) {
    TraverseNode(node->false_edge(), f, checked_nodes);
  }
}

void Graph::Traverse(const std::function<void(Node*)>& f) const {
  std::set<Node*> checked_nodes;
  TraverseNode(entry_, f, &checked_nodes);
}

std::string Graph::DebugString() const {
  std::string out;
  Traverse([&out](Node* node) {
           out += node->name();
           out += "  T  ";
           out += node->true_edge()->name();
           out += "\n";

           out += node->name();
           out += "  F  ";
           out += node->false_edge()->name();
           out += "\n";
           });
  return out;
}

}  // namespace wf
}  // namespace orc
