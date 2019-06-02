#include "hello_server.h"

#include "orc/framework/server_mgr.h"
#include "orc/framework/orc_mgr.h"
#include "hello.pb.h"

namespace orc {
namespace example {
namespace hello {

ORC_REGISTER_SERVER(HelloServer);

class HelloServerServiceImpl : public HelloService {
 public:
  HelloServerServiceImpl() = default;
  virtual ~HelloServerServiceImpl() = default;

  void hello(google::protobuf::RpcController* controller,
             const orc::example::hello::Request* request,
             orc::example::hello::Response* response,
             google::protobuf::Closure* done) override {
    orc::OrcMgr::Instance()->Run(new orc::PbRpcClosure(controller, request, response, done));
  }
};

google::protobuf::Service* HelloServer::NewService() {
  return new HelloServerServiceImpl();
}

}  // namespace example
}  // namespace hello
}  // namespace orc

