#ifndef LEADER_SERVER_SUBSCRIBER_H_
#define LEADER_SERVER_SUBSCRIBER_H_

#include <string>
#include <vector>
#include <memory>

#include "leader/zkwrapper.h"

namespace leader {

class NodeCollectionImpl;
class BadNodeDetector;
class HeartbeatReceiver;

/**
 * @brief server subscriber
 */
class ServerSubscriber {
 public:
  ServerSubscriber();

  virtual ~ServerSubscriber();

  ServerSubscriber(const ServerSubscriber&) = delete;

  ServerSubscriber& operator=(const ServerSubscriber&) = delete;

  /**
   * @brief Initialize
   * @param host zookeeper host address(es)
   * @param timeout rpc timeout
   * @param workThreadNum rpc worker thread number, default as cpu number
   * @param ioThreadNum rpc io thread number, default as cpu number
   * @param channelQueueSize
   * @param hbInterval
   * @return
   */
  bool Init(const std::string& host,
            uint32_t timeout = 5000,
            int32_t workThreadNum = -1,
            int32_t ioThreadNum = 0,
            uint32_t channelCount = 1,
            size_t channelQueueSize = 64,
            uint32_t hbInterval = 5);

  /**
   * @brief Initialize
   * @param zk shared ZKWrapper, subscriber does not take ownership of this object,
   *        but global watcher will be set to a new one
   * @param timeout rpc timeout
   * @param work_thread_num rpc worker thread number, default as cpu number
   * @param io_thread_num rpc io thread number, default as cpu number
   * @param channelQueueSize
   * @param hbInterval
   * @return
   */
  bool Init(ZKWrapper* zk,
            uint32_t timeout = 5000,
            int32_t work_thread_num = -1,
            int32_t io_thread_num = 0,
            uint32_t channel_count = 1,
            size_t channelQueueSize = 64,
            uint32_t hbInterval = 5);

  bool Start();

  virtual void Close();

  /**
   * @brief Add zk path to subscribe from
   * @param path
   */
  void AddPath(const std::string& path);

  /**
   * @brief Add zk paths to subscribe from
   * @param paths
   */
  void AddPaths(const std::vector<std::string>& paths);

  /**
   * @brief Remove zk path and all of its sub-paths from subscribe list
   * @param path
   */
  void RemovePath(const std::string& path);

  /**
   * @brief Remove zk paths and all of their sub-paths from subscribe list
   * @param paths
   */
  void RemovePath(const std::vector<std::string>& paths);

  /**
   * @brief Get alive server count under specified path
   * @param path
   * @return server count
   */
  size_t GetServerCount(const std::string& path);

 protected:
  virtual bool SharedInitializer(uint32_t timeout,
                                 int32_t work_thread_num,
                                 int32_t io_thread_num,
                                 uint32_t channel_count,
                                 size_t channelQueueSize) = 0;

  std::unique_ptr<NodeCollectionImpl> node_collection_;
  std::unique_ptr<BadNodeDetector> bad_node_detector_;
  std::unique_ptr<HeartbeatReceiver> heartbeat_receiver_;
};

}  // namespace leader

#endif  // LEADER_SERVER_SUBSCRIBER_H_
