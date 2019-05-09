/*!
 * \file heartbeat_receiver.h
 * \brief The heartbeat receiver
 */
#ifndef LEADER_HEARTBEAT_RECEIVER_H_
#define LEADER_HEARTBEAT_RECEIVER_H_

#include <map>
#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>

#include "common/thread_running_checker.h"
#include "leader/node_collection_interface.h"
#include "leader/zkwrapper.h"

namespace leader {

/**
 * @brief heartbeat receiver, detect heartbeats
 */
class HeartbeatReceiver {
 public:
  HeartbeatReceiver();

  ~HeartbeatReceiver();

  HeartbeatReceiver(const HeartbeatReceiver&) = delete;

  HeartbeatReceiver& operator=(const HeartbeatReceiver&) = delete;

  /**
   * @brief init receiver
   * @param host zk host address
   * @param nodeCollection node collection instance
   * @param hbInterval interval between checks in seconds
   * @return true if success
   */
  bool Init(const std::string& host, NodeCollectionInterface* nodeCollection,
            uint32_t hbInterval = 5);

  /**
   * @brief init receiver with a shared zk object
   * @param zk shared zk object, receiver does not take ownership of this object,
   *           but global watcher will be set to a new one
   * @param nodeCollection node collection instance
   * @param hbInterval interval between checks in seconds
   * @return true if success
   */
  bool Init(ZKWrapper* zk, NodeCollectionInterface* nodeCollection,
            uint32_t hbInterval = 5);

  /**
   * @brief Add path to receive list
   * @param path
   */
  void AddPath(const std::string& path);

  /**
   * @brief Add paths to receive list
   * @param paths
   */
  void AddPaths(const std::vector<std::string>& paths);

  /**
   * @brief remove all path and sub-paths
   * @param path
   */
  void RemovePath(const std::string& path);

  /**
   * @brief remove all path and sub-paths
   * @param paths
   */
  void RemovePath(const std::vector<std::string>& paths);

  /**
   * @brief Start heartbeat receiving
   * @return true if success, false if already started
   */
  bool StartReceive();

  /**
   * @brief Close receiver, stop receive and shutdown zk client(if not shared),
   *        StartReceive can be called after this call
   */
  void Close();

 private:
  void CommonWatcher(ZKWrapper::ZKEvent event,
                     ZKWrapper::ZKState state,
                     const char* path);

  void WakeUp();

  bool StopReceive();

  class NodeStatus {
   public:
    NodeStatus(int32_t cversion) : cversion(cversion), next_scan_time(0) {}
    std::atomic<int32_t> cversion;
    std::atomic<uint64_t> next_scan_time;
  };

  void Receive(const std::string& path, NodeStatus& status);

  NodeCollectionInterface* node_collection_;
  ZKWrapper* zk_;
  bool shared_zk_;

  common::ReadWriteLock paths_lock_;

  std::map<std::string, std::shared_ptr<NodeStatus>> paths_;

  common::ThreadRunningChecker running_checker_;
  std::atomic<bool> skip_sleep_;
  std::mutex update_lock_;
  std::thread heartbeat_thread_;
  uint32_t heartbeat_interval_;
};

}  // namespace leader

#endif  // LEADER_HEARTBEAT_RECEIVER_H_
