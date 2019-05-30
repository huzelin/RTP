#ifndef ORC_FRAMEWORK_POOL_SESSION_FACTORY_H_
#define ORC_FRAMEWORK_POOL_SESSION_FACTORY_H_

#include <mutex>
#include <stack>
#include <memory>

#include "orc/framework/session_factory.h"
#include "orc/framework/configure.h"
#include "yaml-cpp/yaml.h"

#include "common/monitor/monitor_status_impl.h"

namespace orc {

template<typename T>
class PoolSessionFactory : public SessionFactory {
 public:
  PoolSessionFactory();
  virtual ~PoolSessionFactory();

  bool Init(const YAML::Node& config) override;

  SessionBase* Acquire() override;
  void Release(SessionBase* session) override;

 private:
  std::mutex mutex_;
  std::stack<std::unique_ptr<SessionBase>> sessions_;
  size_t total_size_;

  thread_local static std::stack<std::unique_ptr<SessionBase>> tls_sessions_;
  size_t tls_size_;
};

template<typename T>
thread_local std::stack<std::unique_ptr<SessionBase>> PoolSessionFactory<T>::tls_sessions_;

template<typename T>
PoolSessionFactory<T>::PoolSessionFactory() {}

template<typename T>
PoolSessionFactory<T>::~PoolSessionFactory() {}

template<typename T>
bool PoolSessionFactory<T>::Init(const YAML::Node& config) {
  static_assert(std::is_convertible<T*, SessionBase*>::value,
                "template parameter must be child of SessionBase");
  if (!GetOrcConfig(config, Options::SvrSessionPoolSize, &total_size_)) {
    if (!GetOrcConfig(config, Options::SvrWorkflowPoolSize, &total_size_)) {
      ORC_ERROR("Not found config item: %s.", Options::SvrSessionPoolSize.c_str());
      return false;
    }
  }

  size_t worker_num;
  ORC_CONFIG_OR_FAIL(config, Options::SvrWorkerNum, worker_num);

  tls_size_  = total_size_ / worker_num;
  if (tls_size_ == 0) {
    ORC_ERROR("session size: %zu less than worker num: %zu", total_size_, worker_num);
    return false;
  }

  for (uint32_t i = 0; i < total_size_; ++i) {
    sessions_.emplace(new T());
  }
  return true;
}

template<typename T>
SessionBase* PoolSessionFactory<T>::Acquire() {
  if (tls_sessions_.empty()) {
    size_t ssize;
    SessionBase* s;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      ssize = sessions_.size();

      if (sessions_.empty()) {
        s = nullptr;
      } else {
        s = sessions_.top().release();
        sessions_.pop();
      }
    }

    // MONITOR_STATUS_NORMAL_TIMER_BY("GlobalSessionPoolSize", "0", ssize * 1000, 1);
    return s;
  }

  // MONITOR_STATUS_NORMAL_TIMER_BY("TlsSessionPoolSize", tid_str(), tls_sessions_.size() * 1000, 1);
  auto s = tls_sessions_.top().release();
  tls_sessions_.pop();
  return s;
}

template<typename T>
void PoolSessionFactory<T>::Release(SessionBase* session) {
  session->Clear();

  if (tls_sessions_.size() < tls_size_) {
    tls_sessions_.emplace(session);
  } else {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.emplace(session);
  }
}

}  // namespace orc

#endif  // ORC_FRAMEWORK_POOL_SESSION_FACTORY_H_
