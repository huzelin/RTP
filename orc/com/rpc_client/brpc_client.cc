#include "orc/com/rpc_client/brpc_client.h"

#include "orc/util/utils.h"
#include "orc/util/log.h"

namespace orc {

BrpcClient::BrpcClient(const std::string& name, int32_t id)
  : RpcClient(name, RpcClient::Type::Brpc, id) {}

BrpcClient::~BrpcClient() {}

bool BrpcClient::Init(const YAML::Node& config) {
  if (!RpcClient::Init(config)) {
    ORC_ERROR("RpcClient Init fail.");
    return false;
  }

  brpc_server_subscriber_.reset(new leader::BrpcServerSubscriber());

  auto const pos = leader_server_path_.find_last_of('/');
  path_ = leader_server_path_.substr(pos);
  auto zkHost = leader_server_path_.substr(0, pos);

  if (!brpc_server_subscriber_->Init(zkHost,
                                     leader_request_timeout_ms_,
                                     leader_channel_count_,
                                     leader_hb_interval_s_)) {
    ORC_ERROR("BrpcServerSubscriber Init fail.");
    return false;
  }
  brpc_server_subscriber_->Start();
  brpc_server_subscriber_->AddPath(path_);
  ORC_INFO("brpc subscriber: %s %s inited", zkHost.c_str(), path_.c_str());
  return true;
}

google::protobuf::RpcChannel* BrpcClient::GetChannel() {
  return brpc_server_subscriber_->GetChannel(path_);
}

void BrpcClient::FreeChannel(google::protobuf::RpcChannel* channel) {
  // erpc don't need to free channel.
  (void) channel;
}

google::protobuf::RpcController* BrpcClient::GetController() {
  auto controller = new brpc::Controller();
  controller->set_timeout_ms(leader_request_timeout_ms_);
  controller->set_max_retry(3);
  return controller;
}

void BrpcClient::FreeController(google::protobuf::RpcController* controller) {
  delete controller;
}

}  // namespace orc
