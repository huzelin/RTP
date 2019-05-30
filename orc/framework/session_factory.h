#ifndef ORC_FRAMEWORK_SESSION_FACTORY_H_
#define ORC_FRAMEWORK_SESSION_FACTORY_H_

#include "orc/framework/session_base.h"
#include "yaml-cpp/yaml.h"

namespace orc {

class SessionFactory {
 public:
  SessionFactory() = default;
  virtual ~SessionFactory() = default;

  virtual bool Init(const YAML::Node& config) = 0;

  virtual SessionBase* Acquire() = 0;
  virtual void Release(SessionBase* session) = 0;
};

}  // namespace orc

#endif  // ORC_FRAMEWORK_SESSION_FACTORY_H_
