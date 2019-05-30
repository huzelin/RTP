#include "orc/workflow/node.h"

namespace orc {
namespace wf {

Node* Node::EndNode() {
  static Node end_node{"end_node"};
  return &end_node;
}

}  // namespace wf
}  // namespace orc
