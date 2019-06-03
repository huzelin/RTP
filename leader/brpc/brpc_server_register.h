/*!
 *  \file brpc_server_register.h
 *  \brief The brpc server register
 */
#ifndef LEADER_BRPC_BRPC_SERVER_REGISTER_H_
#define LEADER_BRPC_BRPC_SERVER_REGISTER_H_

#include <thread>
#include <brpc/server.h>

#include "leader/server_register.h"
#include "ping.pb.h"

namespace leader {

class PingServiceImpl : public PingService {
 public:
  virtual void Ping(google::protobuf::RpcController* controller,
                    const PingRequest* request,
                    PingResponse* reponse,
                    google::protobuf::Closure* done); 
};

class BrpcServerRegister : public ServerRegister {
 public:
  BrpcServerRegister();
  virtual ~BrpcServerRegister();

  bool AddPingService(brpc::Server* server);

  virtual bool Start() override;
  virtual void Close() override;

 protected:
  PingServiceImpl service_;
  std::unique_ptr<brpc::Server> server_;
  bool ping_service_added_;
};

}  // namespace leader

#endif  // LEADER_BRPC_BRPC_SERVER_REGISTER_H_
