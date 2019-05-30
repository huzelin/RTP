#ifndef ORC_UTIL_THREAD_POOL_H__
#define ORC_UTIL_THREAD_POOL_H__

#include <memory>
#include <vector>
#include <functional>
#include <atomic>

#include "orc/util/macros.h"

namespace orc {

class ThreadPool {
 public:
  struct Option {
    size_t worker_num;
    size_t queue_num;
    size_t queue_size;
    bool pin_cpu;
  };

  // The arguments need to statisfy this equation:
  //    'worker_num = queue_num * n' where 'n >= 1'.
  ThreadPool(size_t worker_num, size_t queue_num = 1, size_t queue_size = 1024);

  ThreadPool(const Option& option);

  ~ThreadPool();

  using Task = std::function<void()>;

  // When schedule failed, the input argument 'task' will be returned,
  // otherwise, 'Task()' will be returned.
  Task Schedule(Task task);

 private:
  void Loop(size_t idx);
  void PinThread(uint32_t cpu_id);

 private:
  class TaskQueue;
  class Worker;

  std::vector<TaskQueue*> queues_;
  std::vector<Worker*> workers_;

  std::atomic<bool> stop_;
  Option option_;

  ORC_DISALLOW_COPY_AND_ASSIGN(ThreadPool);
};


}  // namespace orc

#endif  // ORC_UTIL_THREAD_POOL_H__
