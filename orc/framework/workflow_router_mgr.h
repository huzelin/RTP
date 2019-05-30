#ifndef ORC_FRAMEWORK_WORKFLOW_ROUTER_MGR_H_
#define ORC_FRAMEWORK_WORKFLOW_ROUTER_MGR_H_

#include <memory>
#include <map>
#include <vector>
#include <string>

#include "orc/util/macros.h"
#include "orc/framework/workflow_router.h"

namespace orc {

class WorkflowRouterMgr {
 public:
  ~WorkflowRouterMgr() = default;

  static WorkflowRouterMgr* Instance();

  void Register(const std::string& name, std::unique_ptr<WorkflowRouter> router);

  WorkflowRouter* GetWorkflowRouter(const std::string& name);

 private:
  WorkflowRouterMgr() = default;

 private:
  std::map<std::string, std::unique_ptr<WorkflowRouter>> routers_;

  ORC_DISALLOW_COPY_AND_ASSIGN(WorkflowRouterMgr);
};

}  // namespace orc

#define ORC_REGISTER_WORKFLOW_ROUTER(Router) \
__attribute__((constructor)) void orc_register_workflow_router_##Router() { \
  orc::WorkflowRouterMgr::Instance()->Register( \
      #Router, std::unique_ptr<orc::WorkflowRouter>(new Router())); \
}

#endif  // ORC_FRAMEWORK_WORKFLOW_ROUTER_MGR_H_
