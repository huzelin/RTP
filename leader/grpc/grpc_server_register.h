/*!
 *  \file grpc_server_register.h
 *  \brief The grpc server register
 */
#ifndef LEADER_GRPC_GRPC_SERVER_REGISTER_H_
#define LEADER_GRPC_GRPC_SERVER_REGISTER_H_

#include <thread>
#include <grpcpp/grpcpp.h>

#include "leader/server_register.h"
#include "ping.grpc.pb.h"

using grpc::Server;
using grpc::ServerCompletionQueue;

namespace leader {

class GrpcServerRegister : public ServerRegister {
 public:
  GrpcServerRegister();
  virtual ~GrpcServerRegister();

  virtual bool Start() override;

  virtual void Close() override;

 protected:
  PingService::AsyncService service_;
  std::unique_ptr<ServerCompletionQueue> cq_;
  std::unique_ptr<Server> server_;
  std::thread complete_consume_thread_;
};

}  // namespace leader

#endif  // LEADER_GRPC_GRPC_SERVER_REGISTER_H_
