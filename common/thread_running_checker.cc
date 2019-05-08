/*!
 * \file thread_running_checker.cc
 */
#include "common/thread_running_checker.h"

namespace common {

ThreadRunningChecker::ThreadRunningChecker(bool running)
    : running_(running), wake_up_flag_(false), mutex_() { }

ThreadRunningChecker::~ThreadRunningChecker() {
  Stop();
  std::lock_guard<std::mutex> guard(mutex_);  // make sure cv_.wait_for quit
}

void ThreadRunningChecker::Start() {
  Set(true);
}

void ThreadRunningChecker::Stop() {
  Set(false);
}

void ThreadRunningChecker::Set(bool value) {
  running_ = value;
  cv_.notify_all();
}

void ThreadRunningChecker::WakeUp() {
  wake_up_flag_ = true;
  cv_.notify_all();
}

bool ThreadRunningChecker::Wait() {
  if (!running_) {
    return false;
  }
  wake_up_flag_ = false;
  std::unique_lock<std::mutex> lock(mutex_);
  cv_.wait(lock, [&]() { return !running_ || wake_up_flag_; });
  return running_;
}

void ThreadRunningChecker::Sleep(int timeout) {
  if (!running_) {
    return;
  }
  wake_up_flag_ = false;
  std::unique_lock<std::mutex> lock(mutex_);
  cv_.wait_for(lock, std::chrono::milliseconds(timeout),
               [&]() { return !running_ || wake_up_flag_; });
}

bool ThreadRunningChecker::IsRunning() {
  return running_;
}

ThreadRunningChecker& ThreadRunningChecker::operator=(bool value) {
  Set(value);
}

ThreadRunningChecker::operator bool() const {
  return running_.load();
}

}  // namespace common
