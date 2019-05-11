/*!
 * \file grpc_call.cc
 * \brief The GRPC Call
 */
#include "leader/grpc/grpc_call.h"

namespace leader {

GrpcCompleteQueueScheduler::GrpcCompleteQueueScheduler() {
  thread_ = std::thread([=]() {
    CompleteRPCLoop();                   
  });
}

GrpcCompleteQueueScheduler::~GrpcCompleteQueueScheduler() {
  if (thread_.joinable()) {
    thread_.join();
  }
}

void GrpcCompleteQueueScheduler::CompleteRPCLoop() {
  void* got_tag;
  bool ok = false;

  // Block until the next result is available in the
  // completion queue "cq".
  while (cq_.Next(&got_tag, &ok)) {
    if (!ok) break;
    GrpcCall* call = static_cast<GrpcCall*>(got_tag);
    call->AsyncCompleteRpc();
    delete call;
  }
}

void GrpcCompleteQueueScheduler::Close() {
  cq_.Shutdown();
}

void GrpcPingCall::Ping(GrpcChannelPool* channel_pool,
                        GRPCChannel* channel,
                        const std::string& spec,
                        int timeout) {
  GrpcPingCall* call = new GrpcPingCall(channel_pool, channel, spec);
  GrpcCall::PingImpl(call, channel, timeout);
}

void GrpcCheckerPingCall::Ping(GrpcChannelPool* channel_pool,
                               GRPCChannel* channel,
                               const std::string& spec,
                               int timeout,
                               common::Waiter* waiter,
                               std::atomic<uint32_t>* bad) {
  GrpcCheckerPingCall* call = new GrpcCheckerPingCall(channel_pool, channel, spec, waiter, bad);
  GrpcCall::PingImpl(call, channel, timeout);
}

}  // namespace leader
