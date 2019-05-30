#include "orc/framework/server_base.h"

#include <thread>
#include <chrono>

#include "orc/framework/orc_mgr.h"
#include "orc/util/log.h"
#include "orc/util/scope_guard.h"

namespace orc {

ServerBase::ServerBase() : status_(ServerBase::Status::Stop), stop_(false) {}

bool ServerBase::SetupOrcLogger(const YAML::Node& config) {
  (void)config;
  return true;
}

bool ServerBase::BaseRun(const YAML::Node& config) {
  if (status_ == ServerBase::Status::Start) {
    ORC_INFO("Server: %s already started.", name().c_str());
    return true;
  }

  auto destory_guard = MakeScopeGuard([this]() {
    ServiceHide();
    StopServer();
    OrcMgr::Instance()->Stop();
  });

  if (!SetupOrcLogger(config)) {
    ORC_ERROR("SetupOrcLogger fail for Server: %s", name().c_str());
    return false;
  }

  if (!OrcMgr::Instance()->Setup(config)) {
    ORC_ERROR("OrcMgr Setup fail for Server: %s", name().c_str());
    return false;
  }

  if (!Init(config)) {
    ORC_ERROR("Init Server: %s fail.", name().c_str());
    return false;
  }

  if (!StartServer(config)) {
    ORC_ERROR("Start PbRpcServer: %s fail.", name().c_str());
    return false;
  }

  if (!ServicePublish(config)) {
    ORC_ERROR("Publish PbRpcServer: %s fail.", name().c_str());
    return false;
  }

  status_ = ServerBase::Status::Start;

  while (!stop_) std::this_thread::sleep_for(std::chrono::seconds(1));
  return true;
}

void ServerBase::BaseStop() { stop_ = true; }

}  // namespace orc
