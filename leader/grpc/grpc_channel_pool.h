/*!
 * \file grpc_channel_pool.h
 * \brief The GRPC channel pool
 */
#ifndef LEADER_GRPC_GRPC_CHANNEL_POOL_H_
#define LEADER_GRPC_GRPC_CHANNEL_POOL_H_

#include <set>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <mutex>

#include "common/lock.h"
#include "leader/grpc/grpc_channel_group.h"

namespace leader {

/**
 * @brief Pool of grpc channels
 */
class GrpcChannelPool {
 public:
  GrpcChannelPool(uint32_t comm_timeout = 200,
                  uint64_t channel_queue_size = 64ul,
                  uint32_t build_conn_timeout = 200);

  ~GrpcChannelPool();

  GrpcChannelPool(const GrpcChannelPool&) = delete;

  GrpcChannelPool& operator=(const GrpcChannelPool&) = delete;

  /**
   * @brief initialize channel pool
   * @param work_thread_num grpc work thread num for callback, default as cpu_num
   * @param io_thread_num grpc io thread num for communication, default as cpu_num
   * @return true if initialize success
   */
  bool Init(int32_t work_thread_num = -1,
            int32_t io_thread_num = 0,
            uint32_t channel_count = 1);

  GRPCChannel* GetChannel(const std::string& spec);

  void MoveFromGood2Bad(const std::string& spec, GRPCChannel* channel);

  void MoveFromBad2Good(const std::string& spec, GRPCChannel* channel);

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

  GrpcChannelGroup* FindChannelGroup(const std::string& spec,
                                     GRPCChannel* channel);

  void OpenNewChannel(const std::string& spec,
                      GrpcChannelGroup* channelGroup);

  /// communicate timeout
  uint32_t timeout_;
  /// channel queue size
  uint64_t queue_size_;
  /// build connet timeout
  uint32_t build_conn_timeout_;
  uint32_t channel_count_;

  common::ReadWriteLock channel_lock_;
  std::map<std::string, GrpcChannelGroup*> channel_pool_;
  /// this object is used for increase channel group lookup speed
  std::map<GRPCChannel*, GrpcChannelGroup*> channel_group_cache_;
};

}  // namespace leader

#endif  // LEADER_GRPC_GRPC_CHANNEL_POOL_H_
