#include "orc/framework/workflow_router_mgr.h"

#include "orc/util/log.h"

namespace orc {

WorkflowRouterMgr* WorkflowRouterMgr::Instance() {
  static WorkflowRouterMgr mgr;
  return &mgr;
}

void WorkflowRouterMgr::Register(const std::string& name,
                                std::unique_ptr<WorkflowRouter> router) {
  routers_.emplace(name, std::move(router));
}

WorkflowRouter* WorkflowRouterMgr::GetWorkflowRouter(const std::string& name) {
  auto it = routers_.find(name);
  if (it == routers_.end()) return nullptr;
  return it->second.get();
}

}  // namespace orc
