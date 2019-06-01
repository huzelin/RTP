/*!
 * \file brpc_server_subscriber.h
 * \brief The BRPC Server subscriber
 */
#ifndef LEADER_BRPC_BRPC_SERVER_SUBSCRIBER_H_
#define LEADER_BRPC_BRPC_SERVER_SUBSCRIBER_H_

#include "leader/server_subscriber.h"
#include "leader/brpc/brpc_channel_pool.h"

namespace leader {

class BrpcServerSubscriber : public ServerSubscriber {
 public:
  BrpcServerSubscriber(); 
  virtual ~BrpcServerSubscriber();

  /**
   * @brief Get RPC channel from listening path
   * @param path
   * @return nullptr if no matching channel found
   */
  BRPCChannel* GetChannel(const std::string& path);

 protected:
  bool SharedInitializer(uint32_t timeout,
                         int32_t work_thread_num,
                         int32_t io_thread_num,
                         uint32_t channel_count) override;

  std::unique_ptr<BrpcChannelPool> channel_pool_;
};

}  // namespace leader

#endif  // LEADER_BRPC_BRPC_SERVER_SUBSCRIBER_H_
