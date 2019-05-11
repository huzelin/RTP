/*!
 * \file grpc_channel_group.cc
 * \brief The GRPC channel group.
 */
#include "leader/grpc/grpc_channel_group.h"

#include "grpc/grpc.h"
#include "common/logging.h"
#include "common/waiter.h"

#include "leader/grpc/grpc_call.h"
#include "leader/grpc/grpc_channel_pool.h"

namespace leader {

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
  std::vector<GRPCChannel*> channels;
  for (auto& pair : channels_) {
    if (pair.first != nullptr) {
      channels.push_back(pair.first.load());
    }
  } 
  common::Waiter waiter(channels.size());
  std::atomic<uint32_t> bad(0);
  for (auto& channel : channels) {
    GrpcCheckerPingCall::Ping(channel_pool_,
                              channel,
                              spec_,
                              &waiter,
                              &bad);
  }
  waiter.Wait();
  return bad.load() < channel_capacity_;
}

}  // namespace leader
