#ifndef ORC_FRAMEWORK_SESSION_BASE_H_
#define ORC_FRAMEWORK_SESSION_BASE_H_

#include "orc/util/macros.h"
#include "orc/framework/context.h"
#include "orc/framework/service_closure.h"

namespace orc {

class SessionBase {
 public:
  SessionBase() : orc_ctx_(this) {}
  virtual ~SessionBase() = default;

  virtual bool Init() { return true; }
  virtual void Clear() {}

  ServiceClosure* service_closure() const { return service_closure_; }
  void set_service_closure(ServiceClosure* closure) { service_closure_ = closure; }

  // Only called by orc
  Context* orc_ctx() { return &orc_ctx_; }

 private:
  Context orc_ctx_;
  ServiceClosure* service_closure_;
  ORC_DISALLOW_COPY_AND_ASSIGN(SessionBase);
};

}  // namespace orc

#endif  // ORC_FRAMEWORK_SESSION_BASE_H_
