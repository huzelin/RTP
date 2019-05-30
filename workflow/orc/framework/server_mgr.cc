#include "orc/framework/server_mgr.h"
#include "orc/util/log.h"
#include "orc/framework/configure.h"

namespace orc {

ServerMgr* ServerMgr::Instance() {
  static ServerMgr mgr;
  return &mgr;
}

void ServerMgr::Register(const std::string& name, std::unique_ptr<ServerBase> server) {
  server->set_name(name);
  servers_.emplace(name, std::move(server));
}

bool ServerMgr::StartServer(const std::string& name, const YAML::Node& config) {
  auto it = servers_.find(name);
  if (it == servers_.end()) {
    ORC_ERROR("Server: %s isn't registered.", name.c_str());
    return false;
  }

  started_server_ = name;
  return it->second->BaseRun(config);
}

void ServerMgr::StopServer() {
  auto it = servers_.find(started_server_);
  if (it == servers_.end()) {
    ORC_ERROR("Server: %s isn't registered.", started_server_.c_str());
    return;
  }

  it->second->BaseStop();
}

}  // namespace orc
