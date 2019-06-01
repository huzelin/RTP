#include "orc/com/component_mgr.h"
#include "orc/com/rpc_client/rpc_client_factory.h"
#include "orc/util/log.h"

namespace orc {

class RpcClientCom : public orc::Component {
 public:
  bool Setup(const YAML::Node& config) override {
    if (!RpcClientFactory::Instance()->Init(config)) {
      ORC_ERROR("RpcClient Com Setup fail.");
      return false;
    }

    ORC_INFO("RpcClient Com Setup success.");
    return true;
  }

  void Release() override {}
};

ORC_REGISTER_COM(RpcClientCom);

}  // namespace orc
