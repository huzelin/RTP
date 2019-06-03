#include "orc/com/rpc_client/rpc_client_factory.h"

#include <strings.h>

#include "orc/util/log.h"
#include "orc/framework/configure.h"

#include "orc/com/rpc_client/brpc_client.h"

namespace orc {

RpcClientFactory* RpcClientFactory::Instance() {
  static RpcClientFactory factory;
  return &factory;
}

bool RpcClientFactory::Init(const YAML::Node& config) {
  std::string rpc_file;
  if (!GetOrcConfig(config, Options::ComRpcClientFile, &rpc_file)) {
    ORC_INFO("can't get option: %s for RpcClient com.",
             Options::ComRpcClientFile.c_str());
    return false;
  }

  return LoadConfigFile(rpc_file);
}

bool RpcClientFactory::LoadConfigFile(const std::string& file) {
  YAML::Node config;
  try {
    config = YAML::LoadFile(file);
  } catch (const YAML::Exception& e) {
    ORC_ERROR("YAML load file: %s fail. for: %s.", file.c_str(), e.what());
    return false;
  }

  if (!config["service_name"]) {
    ORC_ERROR("no [service_name] section in the config.");
    return false;
  }

  for (auto it = config["service_name"].begin();
       it != config["service_name"].end(); ++it) {
    auto name = it->first.as<std::string>();
    auto type = it->second.as<std::string>();

    ORC_INFO("Init service: %s, type: %s.", name.c_str(), type.c_str());

    if (!CreateRpcClient(config, name, type)) {
      ORC_ERROR("Init service: %s fail.", name.c_str());
      return false;
    }

    auto rpc_clients_group = GetRpcClient(name);

    // sanity check.
    if (rpc_clients_group == nullptr) {
      ORC_ERROR("Service: %s isn't init.", name.c_str());
      return false;
    }

    for (const auto& client : *rpc_clients_group) {
      client->WarmUpChannel();
    }

    ORC_DEBUG("Init service: %s success.", name.c_str());
  }

  return true;
}

bool RpcClientFactory::AddRpcClient(const YAML::Node& config,
                                    const std::string& name,
                                    const std::string& type,
                                    int32_t id) {
  std::unique_ptr<RpcClient> client;

  if (strncasecmp(type.c_str(), "brpc", sizeof("brpc")-1) == 0) {
    client.reset(new BrpcClient(name, id));
  } else {
    ORC_ERROR("Unknown service type: %s for service: %s",
              type.c_str(), name.c_str());
    return false;
  }

  if (!client->Init(config)) {
    ORC_ERROR("Init RpcClient: %s, id: %d fail.", name.c_str(), id);
    return false;
  }

  rpc_clients_[name].emplace_back(std::move(client));
  return true;
}

bool RpcClientFactory::CreateRpcClient(const YAML::Node& config,
                                       const std::string& name,
                                       const std::string& type) {
  if (config[name].IsSequence()) {
    for (size_t i = 0; i < config[name].size(); ++i) {
      if (!AddRpcClient(config[name][i], name, type, i)) {
        ORC_ERROR("AddRpcClient name: %s, id: %ld fail.", name.c_str(), i);
        return false;
      }
    }
  } else {
    if (!AddRpcClient(config[name], name, type, 0)) {
      ORC_ERROR("AddRpcClient name: %s, id: %d fail.", name.c_str(), 0);
      return false;
    }
  }
  return true;
}

RpcClientGroup* RpcClientFactory::GetRpcClient(const std::string& service_name) {
  auto it = rpc_clients_.find(service_name);
  if (it != rpc_clients_.end()) return &(it->second);
  return nullptr;
}

}  // namespace orc
