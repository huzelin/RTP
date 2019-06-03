#include "hello.pb.h"

#include "common/monitor/monitor_status.h"
#include "orc/com/rpc_client/rpc_client_factory.h"
#include "orc/util/log.h"

using namespace orc;

int main(int argc, char** argv) {
  MONITOR_STATUS_INIT("/tmp/monitor_client");

  YAML::Node config;
  try {
    config = YAML::LoadFile(argv[1]);
  } catch (const YAML::Exception& e) {
    printf("failed\n");
    return false;
  }
  printf("I amd doing\n");

  if (!RpcClientFactory::Instance()->Init(config)) {
    ORC_ERROR("RpcClient init failed");
  }

  auto method_name = "hello";
  auto method = orc::example::hello::HelloService::descriptor()->FindMethodByName(method_name);
 
  auto rpc_client_group = RpcClientFactory::Instance()->GetRpcClient("HelloServer");
  while (true) {
    for (auto& rpc_client : *rpc_client_group) {
      bool success;
      orc::example::hello::Request request;
      orc::example::hello::Response response;

      rpc_client->CallMethod(method,
                             &request,
                             &response,
                             nullptr,
                             &success);
      ORC_INFO("response=%s", response.DebugString().c_str());
    }
  }
  return 0;
}
