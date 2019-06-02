#include "orc/framework/workflow_router.h"
#include "orc/framework/workflow_router_mgr.h"

namespace orc {
namespace example {
namespace hello {

class HelloWorkflowRouter : public WorkflowRouter {
 public:
  HelloWorkflowRouter() = default;
  virtual ~HelloWorkflowRouter() = default;

  bool Init(const YAML::Node& config) override { return true; }
  std::string Route(SessionBase* session) override { return "hello"; }
};

ORC_REGISTER_WORKFLOW_ROUTER(HelloWorkflowRouter);

}  // namespace hello
}  // namespace example
}  // namespace orc
