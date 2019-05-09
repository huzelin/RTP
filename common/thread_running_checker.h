/*!
 * \file thread_running_checker.h
 * \brief The thread running checker
 */
#ifndef COMMON_THREAD_RUNNING_CHECKER_H_
#define COMMON_THREAD_RUNNING_CHECKER_H_

#include <condition_variable>
#include <mutex>
#include <atomic>

namespace common {

/**
 * @brief Util class provided for single loop running thread
 */
class ThreadRunningChecker {
 public:
  explicit ThreadRunningChecker(bool running = false);

  ~ThreadRunningChecker();

  ThreadRunningChecker &operator=(const ThreadRunningChecker &) = delete;

  ThreadRunningChecker(const ThreadRunningChecker &) = delete;

  ThreadRunningChecker &operator=(bool value);

  explicit operator bool() const;

  bool IsRunning();

  /**
   * @brief Called on looping thread, "smart" sleep, can be interrupted by WakeUp
   * @param timeout timeout in milliseconds
   */
  void Sleep(int timeout);

  /**
   * @brief Called on looping thread, Block until WakeUp() or Stop() is called or not started
   * @return current IsRunning status
   */
  bool Wait();

  /**
   * @brief Set status to running
   */
  void Start();

  /**
   * @brief Set status to stop and wake up
   */
  void Stop();

  /**
   * @brief Wake up thread blocking on Sleep and Wait
   */
  void WakeUp();

 private:
  inline void Set(bool value);

  std::atomic<bool> running_;
  std::atomic<bool> wake_up_flag_;
  std::condition_variable cv_;
  std::mutex mutex_;
};

}  // namespace common

#endif  // COMMON_THREAD_RUNNING_CHECKER_H_
