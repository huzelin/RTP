#ifndef ORC_FRAMEWORK_SERVICE_CLOSURE_H_
#define ORC_FRAMEWORK_SERVICE_CLOSURE_H_

#include "orc/util/closure.h"

namespace orc {

class ServiceClosure : public Closure {
 public:
  enum class RetCode {Success, NoSession, ReqError, AppError};

  ServiceClosure() : ret_code_(RetCode::Success) {}

  RetCode ret_code() const { return ret_code_; }
  void set_ret_code(RetCode ret_code) { ret_code_ = ret_code; }

  std::string message() const { return message_; }
  void set_message(const std::string& message) { message_ = message; }

 private:
  RetCode ret_code_;
  std::string message_;
};

}  // namespace orc

#endif  // ORC_FRAMEWORK_SERVICE_CLOSURE_H_
