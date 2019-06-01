#ifndef ORC_SERVER_BRPC_RPC_SERVER_H_
#define ORC_SERVER_BRPC_RPC_SERVER_H_

#include <memory>

#include "orc/server/pb_rpc_server.h"

#include "brpc/server.h"

namespace orc {

class BrpcServer : public PbRpcServer {
 public:
  BrpcServer() = default;
  virtual ~BrpcServer() = default;

  virtual bool Init(const YAML::Node& config) override = 0;
  virtual google::protobuf::Service* NewService() override = 0;

  bool StartServer(const YAML::Node& config) override;
  bool StopServer() override;

 protected:
  bool InitLeader(const std::string& service_discovery) override;

  uint32_t port_;
  std::unique_ptr<brpc::Server> server_;
  std::unique_ptr<google::protobuf::Service> service_;
  ORC_DISALLOW_COPY_AND_ASSIGN(BrpcServer);
};

}  // namespace orc

#endif  // ORC_SERVER_BRPC_RPC_SERVER_H_
