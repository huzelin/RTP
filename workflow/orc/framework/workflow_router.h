#ifndef ORC_FRAMEWORK_WORKFLOW_ROUTER_H_
#define ORC_FRAMEWORK_WORKFLOW_ROUTER_H_

#include <string>

#include "orc/framework/session_base.h"
#include "yaml-cpp/yaml.h"

namespace orc {

class WorkflowRouter {
 public:
  WorkflowRouter() = default;
  virtual ~WorkflowRouter() = default;

  virtual bool Init(const YAML::Node& config) = 0;

  virtual std::string Route(SessionBase* session) = 0;
};

}  // namespace orc

#endif  // ORC_FRAMEWORK_WORKFLOW_ROUTER_H_
