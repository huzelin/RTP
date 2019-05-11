/*!
 * \file grpc_call.h
 * \brief The GRPC call service for ping/pong and checker
 */
#ifndef LEADER_GRPC_CALL_H_
#define LEADER_GRPC_CALL_H_

#include <memory>
#include <string>

#include "common/waiter.h"
#include "ping.grpc.pb.h"
#include "leader/grpc/grpc_channel_pool.h"

namespace leader {

using grpc::CompletionQueue;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientAsyncResponseReader;

/**
 * @brief Grpc ping call servce
 */
class GrpcCall {
 public:
  virtual bool AsyncCompleteRpc() = 0;
  static void CompleteRPCLoop();
  static void CompleteQueueShutDown();

 protected:
  Status status_;
  ClientContext context_;
  Reply reply_;
  std::shared_ptr<ClientAsyncResponseReader<Reply>> response_reader_;

  static CompletionQueue cq;
};

class GrpcPingCall : public GrpcCall {
 public:
  explicit GrpcPingCall(GrpcChannelPool* channel_pool,
                        GRPCChannel* channel,
                        const std::string& spec) :
      channel_pool_(channel_pool),
      channel_(channel),
      spec_(spec) { }

  virtual bool AsyncCompleteRpc() {
    if (status_.ok()) {
      channel_pool_->MoveFromBad2Good(spec_, channel_);
    } else {
      channel_pool_->MoveFromGood2Bad(spec_, channel_);
    }
    return status_.ok();
  }

  static void Ping(GrpcChannelPool* channel_pool,
                   GRPCChannel* channel,
                   const std::string& spec);
  
 protected:
  GrpcChannelPool *channel_pool_;
  GRPCChannel *channel_;
  std::string spec_;
};

class GrpcCheckerPingCall : public GrpcPingCall {
 public:
  explicit GrpcCheckerPingCall(GrpcChannelPool* channel_pool,
                               GRPCChannel* channel,
                               const std::string& spec,
                               common::Waiter* waiter,
                               std::atomic<uint32_t>* bad) :
      GrpcPingCall(channel_pool, channel, spec),
      waiter_(waiter),
      bad_(bad) { }

  virtual bool AsyncCompleteRpc() {
    auto success = GrpcPingCall::AsyncCompleteRpc();
    if (!success) {
      bad_++;
    }
    return success;
  }

  static void Ping(GrpcChannelPool* channel_pool,
                   GRPCChannel* channel,
                   const std::string& spec,
                   common::Waiter* waiter,
                   std::atomic<uint32_t>* bad);
 
 protected:
  common::Waiter* waiter_;
  std::atomic<uint32_t>* bad_;
};

}  // namespace leader

#endif // LEADER_GRPC_CALL_H_
