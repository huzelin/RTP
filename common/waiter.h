/*
 * \file waiter.h 
 * \brief The waiter for synchronization.
 */
#ifndef COMMON_WAITER_H_
#define COMMON_WAITER_H_

#include <mutex>
#include <condition_variable>

namespace common {

class Waiter {
 public:
  explicit Waiter(int num_wait = 1) : num_wait_(num_wait) { }
  virtual ~Waiter() { }

  void Wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (num_wait_ > 0) cv_.wait(lock);
  }
  void Notify() {
    std::unique_lock<std::mutex> lock(mutex_);
    --num_wait_;
    cv_.notify_all();
  }
  void Reset(int num_wait) {
    std::unique_lock<std::mutex> lock(mutex_);
    num_wait_ = num_wait;
  }

 private:
  std::mutex mutex_;
  std::condition_variable cv_;
  volatile int num_wait_;
};

}  // namespace common

#endif  // COMMON_WAITER_H_
