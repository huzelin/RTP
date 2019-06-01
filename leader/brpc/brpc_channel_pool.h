/*!
 * \file brpc_channel_pool.h
 * \brief The BRPC channel pool
 */
#ifndef LEADER_BRPC_BRPC_CHANNEL_POOL_H_
#define LEADER_BRPC_BRPC_CHANNEL_POOL_H_

#include <set>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <mutex>

#include "common/lock.h"
#include "leader/brpc/brpc_channel_group.h"

namespace leader {

/**
 * @brief Pool of brpc channels
 */
class BrpcChannelPool {
 public:
  BrpcChannelPool(uint32_t comm_timeout = 200,
                  uint32_t build_conn_timeout = 200);

  ~BrpcChannelPool();

  BrpcChannelPool(const BrpcChannelPool&) = delete;

  BrpcChannelPool& operator=(const BrpcChannelPool&) = delete;

  /**
   * @brief initialize channel pool
   * @param work_thread_num brpc work thread num for callback, default as cpu_num
   * @param io_thread_num brpc io thread num for communication, default as cpu_num
   * @return true if initialize success
   */
  bool Init(int32_t work_thread_num = -1,
            int32_t io_thread_num = 0,
            uint32_t channel_count = 1);

  BRPCChannel* GetChannel(const std::string& spec);

  void MoveFromGood2Bad(const std::string& spec, BRPCChannel* channel);

  void MoveFromBad2Good(const std::string& spec, BRPCChannel* channel);

  /**
   * @brief blocking check for bad nodes,
   *        if all channels in a channel group are bad,
   *        we say it is a bad node.
   *        This is not safe if called concurrently with destructor.
   * @param badList
   */
  void CheckAndGetBadNodeSpecs(std::set<std::string>& badList);

 private:
  uint32_t GetLogicCpuNum();

  BrpcChannelGroup* FindChannelGroup(const std::string& spec,
                                     BRPCChannel* channel);

  void OpenNewChannel(const std::string& spec,
                      BrpcChannelGroup* channelGroup);

  /// communicate timeout
  uint32_t timeout_;
  /// build connet timeout
  uint32_t build_conn_timeout_;
  uint32_t channel_count_;

  common::ReadWriteLock channel_lock_;
  mutable std::map<std::string, BrpcChannelGroup*> channel_pool_;
  /// this object is used for increase channel group lookup speed
  std::map<BRPCChannel*, BrpcChannelGroup*> channel_group_cache_;
};

}  // namespace leader

#endif  // LEADER_BRPC_BRPC_CHANNEL_POOL_H_
