#include "orc/server/pb_rpc_server.h"

#include <sys/types.h>
#include <unistd.h>

#include <thread>
#include <chrono>
#include <fstream>  // NOLINT

#include "orc/framework/configure.h"
#include "orc/util/log.h"

#include "leader/grpc/grpc_server_register.h"

namespace orc {

bool PbRpcServer::ServicePublish(const YAML::Node& config) {
  std::string service_discovery;
  if (!GetOrcConfig(config, Options::SvrServiceDiscovery, &service_discovery)) {
    ORC_ERROR("Can't get option: %s for Server: %s.",
              Options::SvrServiceDiscovery.c_str(), name().c_str());
    return false;
  }

  if (service_discovery == "local") {
    ORC_INFO("ServicePublish use local mode.");
  } else {
    int32_t port;
    if (!GetOrcConfig(config, Options::SvrPort, &port)) {
      ORC_ERROR("Can't get option: %s for Server: %s.",
                Options::SvrPort.c_str(), name().c_str());
      return false;
    }

    // get zkHost and path
    auto const pos = service_discovery.find_last_of('/');
    auto path = service_discovery.substr(pos + 1);
    auto zkHost = service_discovery.substr(0, pos);

    service_publisher_.reset(new leader::GrpcServerRegister());
    if (!service_publisher_->Init(zkHost, port)) {
      ORC_ERROR("leader RegisterServer fail for Server: %s, zkHost=%s path=%s",
                name().c_str(), zkHost.c_str(), path.c_str());
      return false;
    }
    service_publisher_->Start();
    service_publisher_->AddPath(path);
    ORC_INFO("ServicePublish use zk mode.");
  }

  // Touch a local flag file to index that service is published.
  pid_t pid = getpid();
  char file[128];
  snprintf(file, sizeof(file), "/tmp/%lu.done", static_cast<uint64_t>(pid));
  std::ofstream ofs(file);
  return true;
}

bool PbRpcServer::ServiceHide() {
  if (service_publisher_) {
    service_publisher_->Close();
    service_publisher_.reset(nullptr);
    std::this_thread::sleep_for(std::chrono::seconds(1));  // wait skeeper to update status
  }
  return true;
}

}  // namespace orc
