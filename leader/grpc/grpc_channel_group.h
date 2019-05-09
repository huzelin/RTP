/*!
 * \file grpc_channel_group.h
 * \brief The GRPC channel group
 */
#ifndef LEADER_GRPC_GRPC_CHANNEL_GROUP_H_
#define LEADER_GRPC_GRPC_CHANNEL_GROUP_H_

#include <cstdint>
#include <atomic>
#include <vector>
#include <utility>

#include <grpcpp/grpcpp.h>

namespace leader {

typedef grpc::Channel GRPCChannel;

/**
 * @brief Manage channels for the same remote spec
 */
class GrpcChannelGroup {
 public:
  explicit GrpcChannelGroup(const uint32_t channelCount)
      : channel_capacity_(channelCount),
        channel_count_(0),
        index_(0),
        channels_(channelCount) {
    for (auto& pair : channels_) {
      pair.first = nullptr;
      pair.second = false;
    }
  }

  /**
   * @brief Don't release channel until we found a way to
   *        make sure channel stop being used
   */
  ~GrpcChannelGroup() {}

  /**
   * @brief add channel to this group
   * @param channel takes ownership of this channel if return not nullptr
   * @return a slot in which channel can be stored, the slot should not be
   *         used unless a nullptr is passed to this interface,
   *         nullptr if group is full
   */
  std::atomic<GRPCChannel*>* AddChannel(GRPCChannel* channel);

  /**
   * @brief Get index of slot provided
   * @param slot
   * @return -1 if slot not belong to this group
   */
  int GetIndexOfSlot(std::atomic<GRPCChannel*>* slot);

  void MoveChannelToGood(GRPCChannel* channel);

  void MoveChannelToBad(GRPCChannel* channel);

  GRPCChannel* PickChannel();

  bool IsFull() {
    return channel_count_ >= channel_capacity_;
  }

  /**
   * @brief Do blocking parallel ping to all holding channels
   * @param timeout
   * @return true if at least one channel is living
   */
  bool CheckChannels(int timeout);

 private:
  const uint32_t channel_capacity_;
  std::atomic<uint32_t> channel_count_;
  std::atomic<uint32_t> index_;
  /// we don't need a lock for this one since only its value can change
  std::vector<std::pair<std::atomic<GRPCChannel*>,
                        std::atomic<bool>>> channels_;
};

}  // namespace leader

#endif  // LEADER_GRPC_GRPC_CHANNEL_GROUP_H_
