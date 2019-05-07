/*!
 * \file thread_pool.h
 * \brief The thread pool for Task Scheduling.
 */

#ifndef COMMON_THREAD_POOL_H_
#define COMMON_THREAD_POOL_H_

namespace common {

class ThreadPool {
 public:
  using Task = std::function<void()>;
  
  ThreadPool(size_t size = 10): stop_ {false} {
    size = size < 1 ? 1 : size;
    for (size_t i = 0; i< size; ++i) {
      pool_.emplace_back(&ThreadPool::Run, this);
    }
  }

  ~ThreadPool() {
    for(std::thread& thread : pool_){
      thread.join();
    }
  }

  void ShutDown() {
    this->stop_.store(true);
  }

  void ReStart() {
    this->stop_.store(false);
  }

  template<class F, class... Args>
  auto Schedule(F&& f, Args&&... args) ->std::future<decltype(f(args...))> {
    if (stop_.load()) {
      throw std::runtime_error("task executor have closed commit.");
    }
    
    using ResType =  decltype(f(args...));
    auto task = std::make_shared<std::packaged_task<ResType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
    {
      std::lock_guard<std::mutex> lock { mtx_task_ };
      tasks_.emplace([task](){
                     (*task)();
                     });
    }
    cv_task_.notify_one();
    
    std::future<ResType> future = task->get_future();
    return future;
  }
 
 private:
  Task GetTask() {
    std::unique_lock<std::mutex> lock { mtx_task_ };
    cv_task_.wait(lock, [this](){ return !tasks_.empty(); });
    Task task {std::move(tasks_.front())};
    tasks_.pop();
    return task;
  }

  void Run() {
    while (true) {
      if (Task task = GetTask()) {
        task();
      } else {
        return;
      }
    }
  }
 
 private:
  std::vector<std::thread> pool_;
  std::queue<Task> tasks_;
  std::mutex mtx_task_;
  std::condition_variable cv_task_;
  std::atomic<bool> stop_;
};

}  // namespace common

#endif  // COMMON_THREAD_POOL_H_
