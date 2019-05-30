#ifndef ORC_WORKFLOW_GRAPH_H_
#define ORC_WORKFLOW_GRAPH_H_

#include <string>
#include <set>
#include <memory>

#include "orc/workflow/node.h"
#include "orc/util/macros.h"

namespace orc {
namespace wf {

class Graph {
 public:
  explicit Graph(const std::string& name) : name_(name), entry_(nullptr) {}
  ~Graph() = default;

  void AddNode(std::unique_ptr<Node> node);

  // Node must be added by 'AddNode' already.
  void set_entry(Node* node) { entry_ = node; }
  Node* entry() const { return entry_; }

  const std::set<std::unique_ptr<Node>>& nodes() const { return nodes_; }

  // Check if all added nodes have 'true&false' edges.
  bool CheckValid() const;

  const std::string& name() const { return name_; }

  std::string DebugString() const;

  void Traverse(const std::function<void(Node*)>& f) const;

 private:
  void TraverseNode(Node* node, const std::function<void(Node*)>& f,
                    std::set<Node*>* checked_nodes) const;
  bool CheckNode(Node* node, std::set<Node*>* checked_nodes) const;
  bool CheckEntry() const;

 private:
  std::string name_;
  Node* entry_;
  std::set<std::unique_ptr<Node>> nodes_;

  ORC_DISALLOW_COPY_AND_ASSIGN(Graph);
};

}  // namespace wf
}  // namespace orc


#endif  // ORC_WORKFLOW_GRAPH_H_
