#ifndef EXAMPLE_ORC_SERVER_HELLO_SESSION_H_
#define EXAMPLE_ORC_SERVER_HELLO_SESSION_H_

#include "orc/framework/session_base.h"
#include "orc/server/pb_rpc_server.h"

#include "hello.pb.h"

namespace orc {
namespace example {
namespace hello {

class HelloSession : public orc::SessionBase {
 public:
  HelloSession() = default;
  virtual ~HelloSession() = default;

  const Request* request() const {
    return static_cast<const Request*>(static_cast<orc::PbRpcClosure*>(service_closure())->request());
  }

  Response* response() const {
    return static_cast<Response*>(static_cast<orc::PbRpcClosure*>(service_closure())->response());
  }

  bool Init() override {
    return true;
  }
};

}  // namespace hello
}  // namespace example
}  // namespace orc


#endif  // EXAMPLE_ORC_SERVER_HELLO_SESSION_H_

