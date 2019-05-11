/*!
 *  \file grpc_server_register.cc
 *  \brief The grpc server register impl
 */
#include "leader/grpc/grpc_server_register.h"

using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

namespace leader {

class CallData {
 public:
  CallData(PingService::AsyncService* service, ServerCompletionQueue* cq)
      : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {
    Proceed();
  }

  void Proceed() {
    if (status_ == CREATE) {
      status_ = PROCESS;
      service_->RequestPing(&ctx_, &request_, &responder_, cq_, cq_, this);
    } else if (status_ == PROCESS) {
      new CallData(service_, cq_);
      status_ = FINISH;
      responder_.Finish(reply_, Status::OK, this);
    } else {
      delete this;
    }
  }


 protected:
  PingService::AsyncService *service_;
  ServerCompletionQueue* cq_;
  ServerContext ctx_;
  Request request_;
  Reply reply_;
  ServerAsyncResponseWriter<Reply> responder_;

  enum CallStatus { CREATE, PROCESS, FINISH };
  CallStatus status_;
};

GrpcServerRegister::GrpcServerRegister() { }

GrpcServerRegister::~GrpcServerRegister() {
}

bool GrpcServerRegister::Start() {
  auto ret = ServerRegister::Start();
  if (!ret) return ret;
  
  std::string spec = ip_ + ":" + std::to_string(port_);
  ServerBuilder builder;
  builder.AddListeningPort(spec, grpc::InsecureServerCredentials());
  builder.RegisterService(&service_);
  cq_ = builder.AddCompletionQueue();
  server_ = builder.BuildAndStart();

  complete_consume_thread_ = std::thread([=]() {
    new CallData(&service_, cq_.get());
    void* got_tag;
    bool ok;
    while (cq_->Next(&got_tag, &ok)) {
      CHECK(ok) << "Verify that the request was completed successfully failed";
      static_cast<CallData*>(got_tag)->Proceed();
    }
  });
  return true;
}

void GrpcServerRegister::Close() {
  ServerRegister::Close();

  server_->Shutdown();
  cq_->Shutdown();
  complete_consume_thread_.join();
}

}  // namespace leader
