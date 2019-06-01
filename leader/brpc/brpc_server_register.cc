/*!
 *  \file brpc_server_register.cc
 *  \brief The brpc server register impl
 */
#include "leader/brpc/brpc_server_register.h"

namespace leader {

void PingServiceImpl::Ping(google::protobuf::RpcController* controller,
                           const PingRequest* request,
                           PingResponse* reponse,
                           google::protobuf::Closure* done) {
  brpc::ClosureGuard done_gruad(done);
  reponse->set_id(request->id());
}

BrpcServerRegister::BrpcServerRegister() { }

BrpcServerRegister::~BrpcServerRegister() { }

bool BrpcServerRegister::Start() {
  auto ret = ServerRegister::Start();
  if (!ret) return ret;

  server_.reset(new Server());
  if (server_->AddService(&service_, brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
    LOG(FATAL) << "Faied to add PingService";
    return false;
  }

  // Start the server
  brpc::ServerOptions options;
  if (server_->Start(port_, &options) != 0) {
    LOG(FATAL) << "Failed to start PingServer";
    return false;
  }
  return true;
}

void BrpcServerRegister::Close() {
  ServerRegister::Close();
  if (server_) {
    server_->Stop(0);
    server_->Join();
  }
}

}  // namespace leader
