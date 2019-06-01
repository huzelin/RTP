#ifndef ORC_COM_RPC_CLIENT_RPC_CLIENT_FACTORY_H_
#define ORC_COM_RPC_CLIENT_RPC_CLIENT_FACTORY_H_

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "orc/util/macros.h"
#include "orc/com/rpc_client/rpc_client.h"

#include "yaml-cpp/yaml.h"

namespace orc {

using RpcClientGroup = std::vector<std::unique_ptr<RpcClient>>;

class RpcClientFactory {
 public:
  ~RpcClientFactory() = default;

  static RpcClientFactory* Instance();

  bool Init(const YAML::Node& config);

  RpcClientGroup* GetRpcClient(const std::string& service_name);

 private:
  RpcClientFactory() = default;

  bool AddRpcClient(const YAML::Node& config,
                    const std::string& name,
                    const std::string& type,
                    int32_t id);

  bool CreateRpcClient(const YAML::Node& config,
                       const std::string& name,
                       const std::string& type);

  bool LoadConfigFile(const std::string& file);

 private:
  std::map<std::string, RpcClientGroup> rpc_clients_;
  ORC_DISALLOW_COPY_AND_ASSIGN(RpcClientFactory);
};

}  // namespace orc

#endif  // ORC_COM_RPC_CLIENT_RPC_CLIENT_FACTORY_H_
