/*!
 * \file grpc_call.cc
 * \brief The GRPC Call
 */
#include "leader/grpc/grpc_call.h"

#include "common/logging.h"

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
                        const std::string& spec) {
  std::shared_ptr<GRPCChannel> s_channel(channel);
  std::unique_ptr<PingService::Stub> stub(PingService::NewStub(s_channel));

  Request request;
  GrpcPingCall* call = new GrpcPingCall(channel_pool, channel, spec);
  call->response_reader_ = stub->PrepareAsyncPing(&call->context_, request, &cq);
  call->response_reader_->StartCall();
  call->response_reader_->Finish(&call->reply_, &call->status_, (void*)call);
}

void GrpcCheckerPingCall::Ping(GrpcChannelPool* channel_pool,
                               GRPCChannel* channel,
                               const std::string& spec,
                               common::Waiter* waiter,
                               std::atomic<uint32_t>* bad) {
  std::shared_ptr<GRPCChannel> s_channel(channel);
  std::unique_ptr<PingService::Stub> stub(PingService::NewStub(s_channel));

  Request request;
  GrpcCheckerPingCall* call = new GrpcCheckerPingCall(channel_pool, channel, spec, waiter, bad);
  call->response_reader_ = stub->PrepareAsyncPing(&call->context_, request, &cq);
  call->response_reader_->StartCall();
  call->response_reader_->Finish(&call->reply_, &call->status_, (void*)call);
}

}  // namespace leader
