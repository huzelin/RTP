/*!
 * \file heartbeat_reporter.h
 * \brief The heartbeat reporter
 */
#ifndef LEADER_HEARTBEAT_REPORTER_H_
#define LEADER_HEARTBEAT_REPORTER_H_

#include <string>
#include <memory>
#include <utility>
#include <set>
#include <map>
#include <atomic>
#include <thread>
#include <future>
#include <condition_variable>

#include "common/thread_running_checker.h"
#include "common/lock.h"
#include "heartbeat_message.pb.h"
#include "leader/zkwrapper.h"

namespace leader {

using std::shared_ptr;

/**
 * @brief Heartbeat reporter, can report to multiple path
 */
class HeartbeatReporter {
 public:
  HeartbeatReporter();

  ~HeartbeatReporter();

  HeartbeatReporter(const HeartbeatReporter&) = delete;

  HeartbeatReporter& operator=(const HeartbeatReporter&) = delete;

  /**
   * @brief init reporter
   * @param zkHost zk host address
   * @param msg message to report
   * @param hbInterval beat interval
   * @return true if init success
   */
  bool Init(const std::string& zkHost,
            const HeartbeatMessage& msg, uint32_t hbInterval = 5);

  /**
   * @brief init reporter
   * @param zk shared zk object, reporter does not take ownership of this object
   * @param msg message to report
   * @param hbInterval beat interval
   * @return true if init success
   */
  bool Init(ZKWrapper* zk,
            const HeartbeatMessage& msg, uint32_t hbInterval = 5);

  /**
   * @brief Add a zk path to report to, heartbeat node will be created under this path
   * @param path zk path started by "/"
   */
  void AddPathToReport(const std::string& path);

  /**
   * @brief Remove registered path
   * @param path
   * @param removeFuture future of the remove action, caller should check valid()
   *                     of removeFuture
   */
  void RemovePath(const std::string& path,
                  std::shared_future<void>* removeFuture = nullptr);

  /**
   * @brief Start report
   * @return
   */
  bool StartReport();

  /**
   * @brief Close reporter, and shutdown zk client(if not shared), StartReport can be called after this call
   */
  void Close();

 private:
  void WakeUp();

  bool StopReport();

  bool Report(const std::string& path,
              const shared_ptr<std::atomic<bool>>& force);

  HeartbeatMessage heartbeat_message_;

  common::ReadWriteLock node_path_lock_;
  /// path : force report
  std::map<std::string, shared_ptr<std::atomic<bool>>> node_path_map_;
  std::mutex node_path_removed_mutex_;
  std::map<std::string, shared_ptr<std::pair<shared_ptr<std::promise<void>>,
      std::shared_future<void>>>>
          node_path_to_remove_;

  ZKWrapper* zk_;
  bool shared_zk_;

  uint32_t heartbeat_interval_seconds_;
  std::thread heartbeat_thread_;
  std::atomic<bool> skip_sleep_;
  common::ThreadRunningChecker running_checker_;
};

}  // namespace leader

#endif  // LEADER_HEARTBEAT_REPORTER_H_
