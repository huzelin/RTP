#ifndef ORC_WORKFLOW_NODE_H_
#define ORC_WORKFLOW_NODE_H_

#include <string>

#include "orc/util/macros.h"

namespace orc {
namespace wf {

class Node {
 public:
  explicit Node(const std::string& name)
      : name_(name), true_edge_(nullptr), false_edge_(nullptr) {}

  ~Node() = default;

  static Node* EndNode();

  void set_true_edge(Node* node) { true_edge_ = node; }
  Node* true_edge() const { return true_edge_; }

  void set_false_edge(Node* node) { false_edge_ = node; }
  Node* false_edge() const { return false_edge_; }

  const std::string& name() const { return name_; }

 private:
  std::string name_;
  Node* true_edge_;
  Node* false_edge_;

  ORC_DISALLOW_COPY_AND_ASSIGN(Node);
};

}  // namespace wf
}  // namespace orc

#endif  // ORC_WORKFLOW_NODE_H_
