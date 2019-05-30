#ifndef ORC_FRAMEWORK_HANDLER_BASE_H_
#define ORC_FRAMEWORK_HANDLER_BASE_H_

#include <string>

#include "orc/framework/session_base.h"
#include "orc/framework/context.h"
#include "yaml-cpp/yaml.h"

namespace orc {

class HandlerBase {
 public:
  HandlerBase() = default;
  virtual ~HandlerBase() = default;

  virtual bool BaseInit(const YAML::Node& config) = 0;
  virtual bool BaseRun(SessionBase* session_base, Context* context) = 0;

  const std::string& name() const { return name_; }
  void set_name(const std::string& name) { name_ = name; }

 private:
  std::string name_;
};

}  // namespace orc

#endif  // ORC_FRAMEWORK_HANDLER_BASE_H_
