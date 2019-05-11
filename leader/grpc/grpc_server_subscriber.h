/*!
 * \file grpc_server_subscriber.h
 * \brief The GRPC Server subscriber
 */
#ifndef LEADER_GRPC_GRPC_SERVER_SUBSCRIBER_H_
#define LEADER_GRPC_GRPC_SERVER_SUBSCRIBER_H_

#include "leader/server_subscriber.h"
#include "leader/grpc/grpc_channel_pool.h"

namespace leader {

class GrpcServerSubscriber : public ServerSubscriber {
 public:
  GrpcServerSubscriber(); 
  virtual ~GrpcServerSubscriber();

  /**
   * @brief Get RPC channel from listening path
   * @param path
   * @return nullptr if no matching channel found
   */
  GRPCChannel* GetChannel(const std::string& path);

 protected:
  bool SharedInitializer(uint32_t timeout,
                         int32_t work_thread_num,
                         int32_t io_thread_num,
                         uint32_t channel_count,
                         size_t channelQueueSize) override;

  std::unique_ptr<GrpcChannelPool> channel_pool_;
};

}  // namespace leader

#endif  // LEADER_GRPC_GRPC_SERVER_SUBSCRIBER_H_
