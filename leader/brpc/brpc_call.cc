/*!
 * \file brpc_call.cc
 * \brief The BRPC Call
 */
#include "leader/brpc/brpc_call.h"

namespace leader {

void BrpcPingCall::Ping(BrpcChannelPool* channel_pool,
                        BRPCChannel* channel,
                        const std::string& spec) {
  BrpcPingCall* call = new BrpcPingCall(channel_pool, channel, spec);
  call->PingImpl(channel);
}

void BrpcCheckerPingCall::Ping(BrpcChannelPool* channel_pool,
                               BRPCChannel* channel,
                               const std::string& spec,
                               common::Waiter* waiter,
                               std::atomic<uint32_t>* bad) {
  BrpcCheckerPingCall* call = new BrpcCheckerPingCall(channel_pool, channel, spec, waiter, bad);
  call->PingImpl(channel);
}

}  // namespace leader
