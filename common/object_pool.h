/*!
 * \file object_pool.h
 * \brief The object pool
 */
#ifndef COMMON_OBJECT_POOL_H_
#define COMMON_OBJECT_POOL_H_

#include <condition_variable>
#include <mutex>
#include <vector>
#include <unordered_map>

namespace common {

template <typename T>
class ObjectPool {
 public:
  ObjectPool(size_t capacity = 0) {
    for (size_t i = 0; i < capacity; ++i) {
      T* d = new T();
      objects_.push_back(d);
    }
  }
  virtual ~ObjectPool() {
    typename std::vector<T*>::iterator iter = objects_.begin();
    for (; iter != objects_.end(); ++iter) {
      if (*iter) delete *iter;
    }
  }
  /// Acquire one object
  virtual T* Acquire() {
    std::unique_lock<std::mutex> lck(mutex_);
    if (objects_.empty()) return  new T();
    T* object = objects_.back();
    objects_.pop_back();
    return object;
  }
  /// Release one object
  virtual void Release(T* object) {
    std::unique_lock<std::mutex> lck(mutex_);
    objects_.push_back(object);
  }
  /// Release the objects vector
  virtual void Release(const std::vector<T*>& objects) {
    std::unique_lock<std::mutex> lck(mutex_);
    typename std::vector<T*>::const_iterator iter = objects.begin();
    for (; iter != objects.end(); ++iter) {
      objects_.push_back(*iter);
    }
  }
  /// Return size of current available objects
  size_t Size() {
    std::unique_lock<std::mutex> lck(mutex_);
    return objects_.size();
  }

 protected:
  std::vector<T*> objects_;
  std::mutex mutex_;
  size_t capacity_;
};

}  // namespace common

#endif  // COMMON_OBJECT_POOL_H_
