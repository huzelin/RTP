/*!
 * \file grpc_server_subscriber.cc
 * \brief The GRPC Server subscriber
 */
#include "leader/grpc/grpc_server_subscriber.h"

#include "leader/node_collection_impl.h"
#include "leader/grpc/grpc_bad_node_detector.h"
#include "leader/grpc/grpc_call.h"

namespace {
const int GET_CHANNEL_RETRY_TIMES = 3;
}  // namespace

namespace leader {

GrpcServerSubscriber::GrpcServerSubscriber() :
    ServerSubscriber(),
    channel_pool_(nullptr) {
  complete_queue_consume_thread_ = std::thread([=]() {
    GrpcCall::CompleteRPCLoop(); 
  });
}

GRPCChannel* GrpcServerSubscriber::GetChannel(const std::string& path) {
  std::string msg;
  for (uint32_t i = 0; i < GET_CHANNEL_RETRY_TIMES; i++) {
    if (!node_collection_->PickOneNode(msg, path)) {
      LOG(ERROR) << "No valid server spec for " << path;
      return nullptr;
    }
    GRPCChannel* channel = channel_pool_->GetChannel(msg);
    if (channel != nullptr) {
      return channel;
    }
  }
  return nullptr;
}

void GrpcServerSubscriber::Close() {
  GrpcCall::CompleteQueueShutDown();
  complete_queue_consume_thread_.join();
}

bool GrpcServerSubscriber::SharedInitializer(uint32_t timeout,
                                             int32_t work_thread_num,
                                             int32_t io_thread_num,
                                             uint32_t channel_count,
                                             size_t channel_queue_size) {
  // init channel pool
  channel_pool_.reset(new GrpcChannelPool(timeout, channel_queue_size));
  if (!channel_pool_->Init(work_thread_num, io_thread_num, channel_count)) {
    LOG(ERROR) << ("Init channel pool failed.");
    return false;
  }
  bad_node_detector_.reset(new GrpcBadNodeDetector(node_collection_.get(),
                                                   channel_pool_.get()));
  return true;
}

}  // namespace leader
