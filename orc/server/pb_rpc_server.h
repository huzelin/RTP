#ifndef ORC_SERVER_PB_RPC_SERVER_H_
#define ORC_SERVER_PB_RPC_SERVER_H_

#include <atomic>

#include "orc/framework/server_base.h"
#include "orc/framework/service_closure.h"

#include "leader/server_register.h"
#include "google/protobuf/service.h"

namespace orc {

class PbRpcClosure : public ServiceClosure {
 public:
  PbRpcClosure(::google::protobuf::RpcController* controller,
               const ::google::protobuf::Message* request,
               ::google::protobuf::Message* response,
               ::google::protobuf::Closure* done)
    : controller_(controller),
      request_(request),
      response_(response),
      done_(done) {}

  ::google::protobuf::RpcController* controller() const { return controller_; }
  const ::google::protobuf::Message* request() const { return request_; }
  ::google::protobuf::Message* response() const { return response_; }
  ::google::protobuf::Closure* done() const { return done_; }

  void Done() override {
    if (ret_code() != ServiceClosure::RetCode::Success) {
      controller_->SetFailed(message());
    }
    done_->Run();
    delete this;
  }

 private:
  ::google::protobuf::RpcController* controller_;
  const ::google::protobuf::Message* request_;
  ::google::protobuf::Message* response_;
  ::google::protobuf::Closure* done_;
};

class PbRpcServer : public ServerBase {
 public:
  PbRpcServer() = default;
  virtual ~PbRpcServer() = default;

  virtual google::protobuf::Service* NewService() = 0;

  bool ServicePublish(const YAML::Node& config) override;
  bool ServiceHide() override;

 private:
  std::unique_ptr<leader::ServerRegister> service_publisher_;

  ORC_DISALLOW_COPY_AND_ASSIGN(PbRpcServer);
};

}  // namespace orc

#endif  // ORC_SERVER_PB_RPC_SERVER_H_
