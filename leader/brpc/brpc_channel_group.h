/*!
 * \file brpc_channel_group.h
 * \brief The BRPC channel group
 */
#ifndef LEADER_BRPC_BRPC_CHANNEL_GROUP_H_
#define LEADER_BRPC_BRPC_CHANNEL_GROUP_H_

#include <cstdint>
#include <atomic>
#include <vector>
#include <utility>

#include <brpc/channel.h>

namespace leader {

typedef brpc::Channel BRPCChannel;
class BrpcChannelPool;

/**
 * @brief Manage channels for the same remote spec
 */
class BrpcChannelGroup {
 public:
  explicit BrpcChannelGroup(const uint32_t channelCount,
                            const std::string& spec,
                            BrpcChannelPool* channel_pool)
      : channel_capacity_(channelCount),
        channel_count_(0),
        index_(0),
        channels_(channelCount),
        spec_(spec),
        channel_pool_(channel_pool) {
    for (auto& pair : channels_) {
      pair.second = false;
    }
  }

  /**
   * @brief Don't release channel until we found a way to
   *        make sure channel stop being used
   */
  ~BrpcChannelGroup() {}

  /**
   * @brief add channel to this group
   * @param channel takes ownership of this channel if return not nullptr
   * @return a slot in which channel can be stored, the slot should not be
   *         used unless a nullptr is passed to this interface,
   *         nullptr if group is full
   */
  std::shared_ptr<BRPCChannel>* AddChannel();

  /**
   * @brief Get index of slot provided
   * @param slot
   * @return -1 if slot not belong to this group
   */
  int GetIndexOfSlot(std::shared_ptr<BRPCChannel>* slot);

  void MoveChannelToGood(BRPCChannel* channel);

  void MoveChannelToBad(BRPCChannel* channel);

  BRPCChannel* PickChannel();

  bool IsFull() {
    return channel_count_ >= channel_capacity_;
  }
  const std::string& spec() const { return spec_; }

  /**
   * @brief Do blocking parallel ping to all holding channels
   * @param timeout
   * @return true if at least one channel is living
   */
  bool CheckChannels(int timeout);

 private:
  std::string spec_;
  BrpcChannelPool *channel_pool_;

  const uint32_t channel_capacity_;
  std::atomic<uint32_t> channel_count_;
  std::atomic<uint32_t> index_;
  /// we don't need a lock for this one since only its value can change
  mutable std::vector<std::pair<std::shared_ptr<BRPCChannel>,
          std::atomic<bool>>> channels_;
};

}  // namespace leader

#endif  // LEADER_BRPC_BRPC_CHANNEL_GROUP_H_
