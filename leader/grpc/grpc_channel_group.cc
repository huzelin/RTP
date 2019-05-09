/*!
 * \file grpc_channel_group.cc
 * \brief The GRPC channel group.
 */
#include "leader/grpc/grpc_channel_group.h"

#include "grpc/grpc.h"
#include "common/logging.h"
#include "common/waiter.h"

namespace leader {

/*
class GrpcChannelGroupPingClosure : public RPCClosure {
 public:
  GrpcChannelGroupPingClosure() = default;
  ~GrpcChannelGroupPingClosure() override = default;

  void DoAsyncPing(GRPCChannel* channel, WaitGroup* wg, int timeout) {
    wg_ = wg;
    wg_->Add();
    arpc::EasyRpcPing_Stub stub(channel);
    time_stamp_ = time(NULL);
    request_.set_id(time_stamp_);
    ctrl_._timeout = timeout;
    stub.Query(&ctrl_, &request_, &response_, this);
  }

  void Run() override {
    wg_->Done();
  }

  bool Success() {
    return response_.id() == time_stamp_ + 1;
  }

 private:
  arpc::EasyRPCController ctrl_;
  arpc::EasyRpcPingQuery request_;
  arpc::EasyRpcPingQuery response_;
  WaitGroup* wg_;
  uint64_t time_stamp_;
};
*/

std::atomic<GRPCChannel*>* GrpcChannelGroup::AddChannel(GRPCChannel* channel) {
  if (IsFull()) {
    return nullptr;
  }
  int index = channel_count_++;
  // channel_count_ may have changed after last check
  // it's ok to let channel_count_ go beyond channel_capacity_
  if (index >= channel_capacity_) {
    return nullptr;
  }
  auto& pair = channels_[index];
  pair.first = channel;
  return &pair.first;
}

int GrpcChannelGroup::GetIndexOfSlot(std::atomic<GRPCChannel*>* slot) {
  for (int i = 0; i < channels_.size(); i++) {
    if (slot == &channels_[i].first) {
      return i;
    }
  }
  return -1;
}

GRPCChannel* GrpcChannelGroup::PickChannel() {
  auto index = index_++ % channel_capacity_;
  for (int i = 0; i < channel_capacity_; i++) {
    if (channels_[index].second) {
      return channels_[index].first;
    }
    index = (index + 1) % channel_capacity_;
  }
  return nullptr;
}

void GrpcChannelGroup::MoveChannelToBad(GRPCChannel* channel) {
  for (auto& pair : channels_) {
    if (channel == pair.first) {
      pair.second = false;
    }
  }
}

void GrpcChannelGroup::MoveChannelToGood(GRPCChannel* channel) {
  for (auto& pair : channels_) {
    if (channel == pair.first) {
      pair.second = true;
    }
  }
}

bool GrpcChannelGroup::CheckChannels(int timeout) {
#if 0
  WaitGroup wg;
  std::vector<GrpcChannelGroupPingClosure*> closures;
  for (auto& pair : channels_) {
    if (pair.first != nullptr && !pair.second) {
      // only check bad channel
      auto closure = new GrpcChannelGroupPingClosure();
      closures.push_back(closure);
      closure->DoAsyncPing(pair.first.load(), &wg, timeout);
    }
  }
  wg.Wait();
  int badCount = 0;
  for (auto closure : closures) {
    badCount += closure->Success() ? 0 : 1;
    delete closure;
  }
  return badCount < channel_capacity_;
#endif
  return true;
}

}  // namespace leader
