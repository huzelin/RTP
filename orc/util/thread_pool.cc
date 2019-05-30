#include "orc/util/thread_pool.h"

#include <pthread.h>

#include <vector>
#include <thread>
#include <atomic>

#include "orc/util/utils.h"

namespace orc {

class ThreadPool::Worker {
 public:
  Worker(std::function<void()> fn) : thread_(fn) {}
  ~Worker() { thread_.join(); }

 private:
  std::thread thread_;
};

class ThreadPool::TaskQueue {
 public:
  explicit TaskQueue(size_t capacity) {
    data_ = new Task[capacity];
    end_ = data_ + capacity;
    front_ = data_;
    back_ = data_;
    size_ = 0;
    capacity_ = capacity;

    pthread_mutex_init(&mutex_, nullptr);
    pthread_cond_init(&empty_cv_, nullptr);
    pthread_cond_init(&full_cv_, nullptr);
  }

  ~TaskQueue() {
    delete[] data_;
    pthread_mutex_destroy(&mutex_);
    pthread_cond_destroy(&empty_cv_);
    pthread_cond_destroy(&full_cv_);
  }

  void Push(ThreadPool::Task task) {
    pthread_mutex_lock(&mutex_);
    while (size_ >= capacity_) pthread_cond_wait(&full_cv_, &mutex_);

    *back_++ = std::move(task);
    if (back_ == end_) back_ = data_;
    ++size_;
    pthread_mutex_unlock(&mutex_);
    pthread_cond_signal(&empty_cv_);
  }

  ThreadPool::Task Pop() {
    pthread_mutex_lock(&mutex_);
    while (size_ == 0) pthread_cond_wait(&empty_cv_, &mutex_);

    auto task = std::move(*front_++);
    if (front_ == end_) front_ = data_;
    --size_;
    pthread_mutex_unlock(&mutex_);
    pthread_cond_signal(&full_cv_);
    return task;
  }

  Task TryPush(ThreadPool::Task task) {
    pthread_mutex_lock(&mutex_);
    if (size_ >= capacity_) {
      pthread_mutex_unlock(&mutex_);
      return task;
    }

    *back_++ = std::move(task);
    if (back_ == end_) back_ = data_;
    ++size_;
    pthread_mutex_unlock(&mutex_);
    pthread_cond_signal(&empty_cv_);

    return ThreadPool::Task();
  }

  Task TryPop() {
    pthread_mutex_lock(&mutex_);
    if (size_ == 0) {
      pthread_mutex_unlock(&mutex_);
      return ThreadPool::Task();
    }

    auto task = std::move(*front_++);
    if (front_ == end_) front_ = data_;
    --size_;
    pthread_mutex_unlock(&mutex_);
    pthread_cond_signal(&full_cv_);

    return task;
  }

 private:
  pthread_mutex_t mutex_;
  pthread_cond_t empty_cv_;
  pthread_cond_t full_cv_;

  ThreadPool::Task* data_;
  ThreadPool::Task* end_;
  ThreadPool::Task* front_;
  ThreadPool::Task* back_;
  size_t size_;
  size_t capacity_;
};

ThreadPool::ThreadPool(size_t worker_num, size_t queue_num, size_t queue_size)
  : ThreadPool(Option{worker_num, queue_num, queue_size, false}) {}

ThreadPool::ThreadPool(const Option& option) : stop_(false), option_(option) {
  for (size_t i = 0; i < option.queue_num; ++i) {
    queues_.emplace_back(new TaskQueue(option.queue_size));
  }

  for (size_t i = 0; i < option.worker_num; ++i) {
    workers_.emplace_back(new Worker([i, this](){ Loop(i); }));
  }
}

ThreadPool::~ThreadPool() {
  for (auto& q : queues_) {
    for (auto i = 0ul; i < (workers_.size() / queues_.size() + 1); ++i) {
      q->Push([this](){ stop_ = true; });
    }
  }

  for (auto& w : workers_) delete w;
  for (auto& q : queues_) delete q;
}

ThreadPool::Task ThreadPool::Schedule(ThreadPool::Task task) {
  auto q = queues_[Random() % queues_.size()];
  return q->TryPush(std::move(task));
}

void ThreadPool::Loop(size_t idx) {
  ThreadMeta* th = GetThreadMeta();
  th->pool = this;
  th->id = idx;
  if (option_.pin_cpu) {
    PinThread(th->id);
  }

  auto q = queues_[idx % queues_.size()];

  while (true) {
    Task task = q->Pop();
    task();
    if (stop_) return;
  }
}

void ThreadPool::PinThread(uint32_t cpu_id) {
  cpu_set_t cs;
  CPU_ZERO(&cs);
  CPU_SET(cpu_id, &cs);
  pthread_setaffinity_np(pthread_self(), sizeof(cs), &cs);
}

}  // namespace orc
