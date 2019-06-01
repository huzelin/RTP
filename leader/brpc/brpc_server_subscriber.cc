/*!
 * \file brpc_server_subscriber.cc
 * \brief The BRPC Server subscriber
 */
#include "leader/brpc/brpc_server_subscriber.h"

#include "leader/node_collection_impl.h"
#include "leader/brpc/brpc_bad_node_detector.h"
#include "leader/brpc/brpc_call.h"

namespace {
const int GET_CHANNEL_RETRY_TIMES = 3;
}  // namespace

namespace leader {

BrpcServerSubscriber::BrpcServerSubscriber() :
    ServerSubscriber(),
    channel_pool_(nullptr) { }

BrpcServerSubscriber::~BrpcServerSubscriber() {
}

BRPCChannel* BrpcServerSubscriber::GetChannel(const std::string& path) {
  std::string msg;
  for (uint32_t i = 0; i < GET_CHANNEL_RETRY_TIMES; i++) {
    if (!node_collection_->PickOneNode(msg, path)) {
      LOG(ERROR) << "No valid server spec for " << path;
      return nullptr;
    }
    BRPCChannel* channel = channel_pool_->GetChannel(msg);
    if (channel != nullptr) {
      return channel;
    }
  }
  return nullptr;
}

bool BrpcServerSubscriber::SharedInitializer(uint32_t timeout,
                                             int32_t work_thread_num,
                                             int32_t io_thread_num,
                                             uint32_t channel_count) {
  // init channel pool
  channel_pool_.reset(new BrpcChannelPool(timeout));
  if (!channel_pool_->Init(work_thread_num, io_thread_num, channel_count)) {
    LOG(ERROR) << ("Init channel pool failed.");
    return false;
  }
  bad_node_detector_.reset(new BrpcBadNodeDetector(node_collection_.get(),
                                                   channel_pool_.get()));
  return true;
}

}  // namespace leader
