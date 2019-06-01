/*!
 * \file brpc_channel_group.cc
 * \brief The BRPC channel group.
 */
#include "leader/brpc/brpc_channel_group.h"

#include "common/logging.h"
#include "common/waiter.h"

#include "leader/brpc/brpc_call.h"
#include "leader/brpc/brpc_channel_pool.h"

namespace leader {

std::shared_ptr<BRPCChannel>* BrpcChannelGroup::AddChannel() {
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
  return &pair.first;
}

int BrpcChannelGroup::GetIndexOfSlot(std::shared_ptr<BRPCChannel>* slot) {
  for (int i = 0; i < channels_.size(); i++) {
    if (slot == &channels_[i].first) {
      return i;
    }
  }
  return -1;
}

BRPCChannel* BrpcChannelGroup::PickChannel() {
  auto index = index_++ % channel_capacity_;
  for (int i = 0; i < channel_capacity_; i++) {
    if (channels_[index].second) {
      return channels_[index].first.get();
    }
    index = (index + 1) % channel_capacity_;
  }
  return nullptr;
}

void BrpcChannelGroup::MoveChannelToBad(BRPCChannel* channel) {
  for (auto& pair : channels_) {
    if (channel == pair.first.get()) {
      pair.second = false;
      DLOG(INFO) << "Set channel: " << channel << " Bad";
    }
  }
}

void BrpcChannelGroup::MoveChannelToGood(BRPCChannel* channel) {
  for (auto& pair : channels_) {
    if (channel == pair.first.get()) {
      pair.second = true;
      DLOG(INFO) << "Set channel: " << channel << " Good";
    }
  }
}

bool BrpcChannelGroup::CheckChannels(int timeout) {
  std::vector<BRPCChannel*> channels;
  for (auto& pair : channels_) {
    if (pair.first != nullptr && !pair.second) {
      channels.push_back(pair.first.get());
    }
  } 
  common::Waiter waiter(channels.size());
  std::atomic<uint32_t> bad(0);
  for (auto& channel : channels) {
    BrpcCheckerPingCall::Ping(channel_pool_,
                              channel,
                              spec_,
                              &waiter,
                              &bad);
  }
  waiter.Wait();
  return bad.load() < channel_capacity_;
}

}  // namespace leader
