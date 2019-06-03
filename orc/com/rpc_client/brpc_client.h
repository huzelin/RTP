#ifndef ORC_COM_RPC_CLIENT_BRPC_RPC_CLIENT_H_
#define ORC_COM_RPC_CLIENT_BRPC_RPC_CLIENT_H_

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "leader/brpc/brpc_server_subscriber.h"
#include "orc/com/rpc_client/rpc_client.h"

namespace orc {

class BrpcClient : public RpcClient {
 public:
  explicit BrpcClient(const std::string& name, int32_t id);
  virtual ~BrpcClient();

  bool Init(const YAML::Node& config) override;

  google::protobuf::RpcChannel* GetChannel() override;
  void FreeChannel(google::protobuf::RpcChannel* channel) override;

  google::protobuf::RpcController* GetController() override;
  void FreeController(google::protobuf::RpcController* controller) override;

 private:
  std::unique_ptr<leader::BrpcServerSubscriber> brpc_server_subscriber_;

 private:
  ORC_DISALLOW_COPY_AND_ASSIGN(BrpcClient);
};

}  // namespace orc

#endif  // ORC_COM_RPC_CLIENT_BRPC_RPC_CLIENT_H_
