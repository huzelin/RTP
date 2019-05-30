#include "orc/framework/workflow_router.h"
#include "orc/framework/workflow_router_mgr.h"

namespace orc {

class DefaultWorkflowRouter : public WorkflowRouter {
 public:
  DefaultWorkflowRouter() = default;
  virtual ~DefaultWorkflowRouter() = default;

  bool Init(const YAML::Node& config) override { return true; }
  std::string Route(SessionBase* session) override { return "default"; }
};

ORC_REGISTER_WORKFLOW_ROUTER(DefaultWorkflowRouter);

}  // namespace orc
