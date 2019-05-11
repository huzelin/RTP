/*!
 * \file grpc_call.cc
 * \brief The GRPC Call
 */
#include "leader/grpc/grpc_call.h"

namespace leader {

CompletionQueue GrpcCall::cq; 

void GrpcCall::CompleteRPCLoop() {
  void* got_tag;
  bool ok = false;

  // Block until the next result is available in the
  // completion queue "cq".
  while (cq.Next(&got_tag, &ok)) {
    CHECK(ok) << "Verify that the request was completed successfully failed";
    GrpcCall* call = static_cast<GrpcCall*>(got_tag);
    call->AsyncCompleteRpc();
    delete call;
  }
}

void GrpcCall::CompleteQueueShutDown() {
  cq.Shutdown();
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
