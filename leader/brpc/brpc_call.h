/*!
 * \file brpc_call.h
 * \brief The BRPC call service for ping/pong and checker
 */
#ifndef LEADER_BRPC_CALL_H_
#define LEADER_BRPC_CALL_H_

#include <memory>
#include <string>
#include <thread>

#include "common/common_defines.h"
#include "common/logging.h"
#include "common/singleton.h"
#include "common/waiter.h"
#include "ping.pb.h"
#include "leader/brpc/brpc_channel_pool.h"

namespace leader {

class BrpcPingCall {
 public:
  explicit BrpcPingCall(BrpcChannelPool* channel_pool,
                        BRPCChannel* channel,
                        const std::string& spec) :
      channel_pool_(channel_pool),
      channel_(channel),
      spec_(spec),
      status_(false) { }

  static void Ping(BrpcChannelPool* channel_pool,
                   BRPCChannel* channel,
                   const std::string& spec);

 protected:
  // Clieng Ping implementation.
  void PingImpl(BRPCChannel* channel) {
    auto cntl = new brpc::Controller();

    PingRequest request;
    request.set_id(kMaskCode);
    PingResponse* response = new PingResponse();

    PingService_Stub stub(channel);
    auto done = brpc::NewCallback(this, &BrpcPingCall::HandleResponse, cntl, response);
    stub.Ping(cntl, &request, response, done);
  }

  virtual void HandleResponse(brpc::Controller* cntl, PingResponse* response) {
    HandleResponseImpl(cntl, response);
    delete this;
  }

  void HandleResponseImpl(brpc::Controller* cntl, PingResponse* response) {
    std::unique_ptr<brpc::Controller> cntl_guard(cntl);
    std::unique_ptr<PingResponse> response_guard(response);

    if (!cntl->Failed() && response->id() == kMaskCode) {
      channel_pool_->MoveFromBad2Good(spec_, channel_);
      status_ = true;
    } else {
      channel_pool_->MoveFromGood2Bad(spec_, channel_);
    }
  }

  BrpcChannelPool *channel_pool_;
  BRPCChannel *channel_;
  std::string spec_;
  bool status_;
  const int kMaskCode = 0x08;
};

class BrpcCheckerPingCall : public BrpcPingCall {
 public:
  explicit BrpcCheckerPingCall(BrpcChannelPool* channel_pool,
                               BRPCChannel* channel,
                               const std::string& spec,
                               common::Waiter* waiter,
                               std::atomic<uint32_t>* bad) :
      BrpcPingCall(channel_pool, channel, spec),
      waiter_(waiter),
      bad_(bad) { }

  virtual void HandleResponse(brpc::Controller* cntl, PingResponse* response) override {
    BrpcPingCall::HandleResponseImpl(cntl, response);
    if (!status_) {
      (*bad_)++;
    }
    waiter_->Notify();
    delete this;
  }

  static void Ping(BrpcChannelPool* channel_pool,
                   BRPCChannel* channel,
                   const std::string& spec,
                   common::Waiter* waiter,
                   std::atomic<uint32_t>* bad);
 
 protected:
  common::Waiter* waiter_;
  std::atomic<uint32_t>* bad_;
};

}  // namespace leader

#endif // LEADER_BRPC_CALL_H_
