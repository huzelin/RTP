#include "orc/server/brpc_server.h"
#include "orc/util/log.h"
#include "orc/framework/configure.h"

#include "leader/brpc/brpc_server_register.h"

namespace orc {

bool BrpcServer::StartServer(const YAML::Node& config) {
  uint32_t io_thread_num;
  if (!GetOrcConfig(config, Options::SvrIoThreadNum, &io_thread_num)) {
    ORC_ERROR("Can't get option: %s for Server: %s",
              Options::SvrIoThreadNum.c_str(), name().c_str());
    return false;
  }
  server_.reset(new brpc::Server());
  if (server_->AddService(NewService(), brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
    ORC_ERROR("Failed to add service");
    return false;
  }
  auto brpc_server_register = new leader::BrpcServerRegister();
  if (!brpc_server_register->AddPingService(server_.get())) {
    ORC_ERROR("Failed AddPingService");
    return false;
  }
  service_publisher_.reset(brpc_server_register);
 
  if (!GetOrcConfig(config, Options::SvrPort, &port_)) {
    ORC_ERROR("Can't get option: %s for Server: %s",
              Options::SvrPort.c_str(), name().c_str());
    return false;
  }
  // Start service
  brpc::ServerOptions options;
  options.num_threads = io_thread_num;

  if (server_->Start(port_, &options) != 0) {
    ORC_ERROR("Failed to Start BrpcServer");
    return false;
  }
  ORC_INFO("Server: %s start success.", name().c_str());
  return true;
}

bool BrpcServer::StopServer() {
  if (status_ == ServerBase::Status::Start) {
    server_->Stop(0);
    server_->Join();
  }
  server_.reset(nullptr);
  return true;
}

bool BrpcServer::InitLeader(const std::string& service_discovery) {
  // get zkHost and path
  auto const pos = service_discovery.find_last_of('/');
  auto path = service_discovery.substr(pos);
  auto zkHost = service_discovery.substr(0, pos);
  
  if (!service_publisher_->Init(zkHost, port_)) {
    ORC_ERROR("leader RegisterServer fail for Server: %s, zkHost=%s path=%s",
              name().c_str(), zkHost.c_str(), path.c_str());
    return false;
  }
  service_publisher_->Start();
  service_publisher_->AddPath(path);
  return true; 
}

}  // namespace orc
