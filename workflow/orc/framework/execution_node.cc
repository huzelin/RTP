#include "orc/framework/execution_node.h"
#include "orc/framework/handler_mgr.h"

#include "orc/util/log.h"

namespace orc {

ExecutionNode* ExecutionNode::EndNode() {
  static ExecutionNode end_node("end_node");
  return &end_node;
}

std::unique_ptr<ExecutionNode> ExecutionNode::Clone() const {
  auto node = std::unique_ptr<ExecutionNode>(new ExecutionNode(name_));
  auto handler = HandlerMgr::Instance()->GetHandler(name_);
  if (!handler) {
    ORC_ERROR("Clone fail for can't get handler by name: %s", name_.c_str());
    return nullptr;
  }
  node->set_handler(std::move(handler));
  return node;
}

void ExecutionNode::Run(SessionBase* session, Context* ctx) {
  ctx->set_next_edge(handler_->BaseRun(session, ctx));
}

}  // namespace orc
