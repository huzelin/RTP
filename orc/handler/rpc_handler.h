#ifndef ORC_HANDLER_RPC_HANDLER_H__
#define ORC_HANDLER_RPC_HANDLER_H__

#include <vector>
#include <string>

#include "orc/framework/handler_base.h"

#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/stubs/common.h"

namespace orc {

struct RpcParam {
  const google::protobuf::Message* request;

  struct Resp {
    google::protobuf::Message* response;
    bool success;

    Resp() : response(nullptr), success(false) {}
    Resp(google::protobuf::Message* resp, bool suc) : response(resp), success(suc) {}
    Resp(google::protobuf::Message* resp) : response(resp), success(false) {}
  };

  std::vector<Resp> resps;

  RpcParam() : request(nullptr) {}
};

class RpcHandlerBase : public HandlerBase {
 public:
  virtual bool Init(const YAML::Node& config) = 0;

  virtual bool BuildRequest(SessionBase* session_base,
                            std::vector<RpcParam>* rpc_params) = 0;

  virtual bool ProcessResponse(SessionBase* session_base,
                               const std::vector<RpcParam>& rpc_params) = 0;

  virtual const google::protobuf::ServiceDescriptor* service_descriptor() = 0;

 private:
  bool BaseInit(const YAML::Node& config) override;
  bool BaseRun(SessionBase* session_base, Context* context) override;

  std::string service_name_;
  const google::protobuf::MethodDescriptor* method_;
};

template<typename Service>
class RpcHandler : public RpcHandlerBase {
 public:
  const google::protobuf::ServiceDescriptor* service_descriptor() override {
    return Service::descriptor();
  }
};

}  // namespace orc

#endif  // ORC_HANDLER_RPC_HANDLER_H__
