/*! 
 * \file queue.h
 * \brief Defines a thread safe queue
 */
#ifndef COMMON_QUEUE_H_
#define COMMON_QUEUE_H_

#include <atomic>
#include <list>
#include <mutex>
#include <condition_variable>

namespace common {

/*!
 * \brief A thread safe queue support multithread push and pop concurrently
 *        The queue is based on move semantics.
 */
template<typename T>
class Queue {
 public:
  /*! \brief Constructor */
  Queue() { exit_.store(false); }

  /*!
   * \brief Push an element into the queue. the function is based on
   *        move semantics. After you pushed, the item would be an
   *        uninitialized variable.
   * \param item item to be pushed
   */
  void Push(T& item);
  void PushFront(T& item);

  /*!
   * \brief Pop an element from the queue, if the queue is empty, thread
   *        call pop would be blocked
   * \param result the returned result
   * \return true when pop successfully; false when the queue is exited
   */
  bool Pop(T& result);

  /*! \brief thread will not be blocked. Return false if queue is empty */
  bool TryPop(T& result);

  /*!
   * \brief Get the front element from the queue, if the queue is empty,
   *        threat who call front would be blocked. Not move semantics.
   * \param result the returned result
   * \return true when pop successfully; false when the queue is exited
   */
  bool Front(T& result);

  /*!
   * \brief Gets the number of elements in the queue
   * \return size of queue
   */
  int Size() const;

  /*!
   * \brief Whether queue is empty or not
   * \return true if queue is empty; false otherwise
   */
  bool Empty() const;

  /*! \brief Exit queue, awake all threads blocked by the queue */
  void Exit();

  bool Alive();

 private:
  /*! the underlying container of queue */
  std::list<T> buffer_;
  mutable std::mutex mutex_;
  std::condition_variable empty_condition_;
  /*! whether the queue is still work */
  std::atomic_bool exit_;
  // bool exit_;

  // No copying allowed
  Queue(const Queue&);
  void operator=(const Queue&);
};

template <typename T>
void Queue<T>::Push(T& item) {
  std::lock_guard<std::mutex> lock(mutex_);
  buffer_.push_back(std::move(item));
  empty_condition_.notify_one();
}

template <typename T>
void Queue<T>::PushFront(T& item) {
  std::lock_guard<std::mutex> lock(mutex_);
  buffer_.push_front(std::move(item));
  empty_condition_.notify_one();
}

template <typename T>
bool Queue<T>::Pop(T& result) {
  std::unique_lock<std::mutex> lock(mutex_);
  // empty_condition_.wait(lock,
  //  [this]{ return !buffer_.empty() || exit_; });
  while (buffer_.empty() && !exit_) {
     empty_condition_.wait(lock);
  }
  if (buffer_.empty()) return false;
  result = std::move(buffer_.front());
  buffer_.pop_front();
  return true;
}

template <typename T>
bool Queue<T>::TryPop(T& result) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (buffer_.empty()) return false;
  result = std::move(buffer_.front());
  buffer_.pop_front();
  return true;
}

template <typename T>
bool Queue<T>::Front(T& result) {
  std::unique_lock<std::mutex> lock(mutex_);
  empty_condition_.wait(lock,
    [this]{ return !buffer_.empty() || exit_; });
  if (buffer_.empty()) return false;
  result = buffer_.front();
  return true;
}

template <typename T>
int Queue<T>::Size() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return static_cast<int>(buffer_.size());
}

template <typename T>
bool Queue<T>::Empty() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return buffer_.empty();
}

template <typename T>
void Queue<T>::Exit() {
  std::lock_guard<std::mutex> lock(mutex_);
  exit_.store(true);
  empty_condition_.notify_all();
}

template <typename T>
bool Queue<T>::Alive() {
  std::lock_guard<std::mutex> lock(mutex_);
  return exit_ == false;
}

}  // namespace common

#endif  // COMMON_QUEUE_H_
