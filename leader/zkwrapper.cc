/*!
 * \file zkwrapper.cc
 * \brief zk wrapper
 */
#include "leader/zkwrapper.h"

#include <utility>
#include <string>
#include <vector>

#include "common/logging.h"
#include "zookeeper.h"

namespace leader {

ZKWrapper::ZKEvent ZKWrapper_zooEventToZKEvent(int event) {
#define ZOOEVENT2ZK(a) if (event == ZOO_##a##_EVENT) {return ZKWrapper::ZKEVENT_##a;}
  ZOOEVENT2ZK(CREATED);
  ZOOEVENT2ZK(DELETED);
  ZOOEVENT2ZK(CHANGED);
  ZOOEVENT2ZK(CHILD);
  ZOOEVENT2ZK(SESSION);
  ZOOEVENT2ZK(NOTWATCHING);
#undef ZOOEVENT2ZK
  return ZKWrapper::ZKEVENT_UNDEFINED;
}

ZKWrapper::ZKState ZKWrapper_zooStateToZKState(int state) {
#define ZOOSTATE2ZK(a) if (state == ZOO_##a##_STATE) {return ZKWrapper::ZKSTATE_##a;}
  ZOOSTATE2ZK(CONNECTING);
  ZOOSTATE2ZK(CONNECTED);
  ZOOSTATE2ZK(ASSOCIATING);
  ZOOSTATE2ZK(EXPIRED_SESSION);
  ZOOSTATE2ZK(AUTH_FAILED);
#undef ZOOSTATE2ZK
  return ZKWrapper::ZKSTATE_UNDEFINED;
}

ZKWrapper::ZKCode ZKWrapper_zooCodeToZKCode(int code) {
#define ZOOCODE2ZK(a) if (code == Z##a) {return ZKWrapper::ZK_##a;}
  ZOOCODE2ZK(OK);
  ZOOCODE2ZK(NONODE);
  ZOOCODE2ZK(NODEEXISTS);
  ZOOCODE2ZK(BADVERSION);
  ZOOCODE2ZK(CONNECTIONLOSS);
  ZOOCODE2ZK(OPERATIONTIMEOUT);
  ZOOCODE2ZK(BADARGUMENTS);
  ZOOCODE2ZK(INVALIDSTATE);
  ZOOCODE2ZK(SESSIONEXPIRED);
  ZOOCODE2ZK(SYSTEMERROR);
#undef ZOOCODE2ZK
  return (ZKWrapper::ZKCode) code;
}

int ZKWrapper_ZKCodeTozooCode(ZKWrapper::ZKCode code) {
  switch (code) {
#define ZKCODE2ZOO(a) case ZKWrapper::ZK_##a: return Z##a;
    ZKCODE2ZOO(OK);
    ZKCODE2ZOO(NONODE);
    ZKCODE2ZOO(NODEEXISTS);
    ZKCODE2ZOO(BADVERSION);
    ZKCODE2ZOO(CONNECTIONLOSS);
    ZKCODE2ZOO(OPERATIONTIMEOUT);
    ZKCODE2ZOO(BADARGUMENTS);
    ZKCODE2ZOO(INVALIDSTATE);
    ZKCODE2ZOO(SESSIONEXPIRED);
    ZKCODE2ZOO(SYSTEMERROR);
#undef ZKCODE2ZOO
    default:
    break;
  }
  return -255;
}

template<typename T>
class ZKWrapperContext {
 public:
  template<class... Args>
  void operator()(Args&& ... args) const {
    if (callback_ == nullptr) {
      return;
    }
    callback_(std::forward<Args>(args)...);
  }

  ZKWrapperContext(T&& callback, ZKWrapper* wrapper) :
      callback_(std::move(callback)), zkwrapper_(wrapper) {
  }

  ~ZKWrapperContext() = default;

  void Remove() {
    if (zkwrapper_ == nullptr) {
      return;
    }
    DLOG(INFO) << "remove context:" << this << ":" << id_;
    zkwrapper_->RemoveContextById(id_);
  }

  void SetId(uint64_t id) {
    id_ = id;
  }

  uint64_t id() {
    return id_;
  }

  static uint64_t AddContext(std::shared_ptr<ZKWrapperContext<T>>& ctx,
                             ZKWrapper* zkWrapper) {
    return zkWrapper->AddContext(ctx);
  }
 
 private:
  ZKWrapper* zkwrapper_;
  T callback_;
  uint64_t id_;
};

template<typename T>
static std::shared_ptr<ZKWrapperContext<T>> ZKWrapperContext_Create(
    T&& callback, ZKWrapper* wrapper) {
  auto context = std::make_shared<ZKWrapperContext<T>>(std::move(callback),
                                                       wrapper);
  context->SetId(ZKWrapperContext<T>::AddContext(context, wrapper));
  return context;
}

void ZKWrapper_LambdaStringCompletion(int rc,
                                      const char* value,
                                      const void* data) {
  if (data == nullptr) {
    return;
  }
  DLOG(INFO) << "will remove context:" << data;
  auto context = (ZKWrapperContext<ZKWrapper::StringCompleteLambda>*) data;
  (*context)(ZKWrapper_zooCodeToZKCode(rc), value);
  context->Remove();
}

void ZKWrapper_LambdaVoidCompletion(int rc, const void* data) {
  if (data == nullptr) {
    return;
  }
  DLOG(INFO) << "will remove context:" << data;
  auto context = (ZKWrapperContext<ZKWrapper::VoidCompleteLambda>*) data;
  (*context)(ZKWrapper_zooCodeToZKCode(rc));
  context->Remove();
}

void ZKWrapper_LambdaStatCompletion(int rc,
                                    const struct Stat* stat,
                                    const void* data) {
  if (data == nullptr) {
    return;
  }
  DLOG(INFO) << "will remove context:" << data;
  auto context = (ZKWrapperContext<ZKWrapper::StatCompleteLambda>*) data;
  (*context)(ZKWrapper_zooCodeToZKCode(rc), (ZKWrapper::ZKStatus*) stat);
  context->Remove();
}

void ZKWrapper_LambdaDataCompletion(int rc, const char* value, int value_len,
                                    const struct Stat* stat, const void* data) {
  if (data == nullptr) {
    return;
  }
  DLOG(INFO) << "will remove context:" << data;
  auto context = (ZKWrapperContext<ZKWrapper::DataCompleteLambda>*) data;
  (*context)(ZKWrapper_zooCodeToZKCode(rc),
             value,
             value_len,
             (ZKWrapper::ZKStatus*) stat);
  context->Remove();
}

void ZKWrapper_LambdaStringArrayStatCompletion(int rc,
                                               const struct String_vector* strings,
                                               const struct Stat* stat,
                                               const void* data) {
  if (data == nullptr) {
    return;
  }
  DLOG(INFO) << "will remove context:" << data;
  auto context =
      (ZKWrapperContext<ZKWrapper::StringArrayStatCompleteLambda>*) data;
  if (strings) {
    (*context)(ZKWrapper_zooCodeToZKCode(rc),
               const_cast<const char**>(strings->data),
               strings->count,
               (ZKWrapper::ZKStatus*) stat);
  } else {
    (*context)(ZKWrapper_zooCodeToZKCode(rc),
               nullptr,
               0,
               (ZKWrapper::ZKStatus*) stat);
  }
  context->Remove();
}

void ZKWrapper_LambdaWatcher(zhandle_t* zh, int type, int state,
                             const char* path, void* watcherCtx) {
  // we don't response to SESSION EVENT in lambda watcher
  // by api document, one shot watcher should only be called once.
  // but in the implementation of zookeeper c client, collect_session_watchers
  // called by collectWatchers will collect all watchers to inform a SESSION_EVENT
  if (watcherCtx == nullptr || type == ZOO_SESSION_EVENT) {
    return;
  }
  DLOG(INFO) << "will remove context:" << watcherCtx;
  auto context = (ZKWrapperContext<ZKWrapper::WatcherLambda>*) watcherCtx;
  (*context)(ZKWrapper_zooEventToZKEvent(type),
             ZKWrapper_zooStateToZKState(state),
             path);
  context->Remove();
}

template<typename T>
void ZKWrapper::RemoveContext(const std::shared_ptr<T>& context) {
#ifndef NDEBUG
  LOG(INFO) << "RemoveContext:" << context.get() << " id:" << context->id();
#endif
  RemoveContextById(context->id());
}

void ZKWrapper::RemoveContextById(uint64_t id) {
  std::lock_guard<std::mutex> guard(context_pool_mutex_);
#ifndef NDEBUG
  LOG(INFO) << this << ":" << zhandle_ << " remove context:" << id
      << ":" << context_pool_.size();
#endif
  context_pool_.erase(id);
}

template<typename T>
uint64_t ZKWrapper::AddContext(std::shared_ptr<T> context) {
  if (context == nullptr) {
    return 0;
  }
  std::lock_guard<std::mutex> guard(context_pool_mutex_);
  auto id = ++context_id_count_;
  while (context_pool_.find(id) != context_pool_.end()) {
    id = ++context_id_count_;
  }
  context_pool_[id] = context;
  return id;
}

void ZKWrapper::GlobalZooWatcher(zhandle_t* zh, int type, int state,
                                 const char* path, void* watcherCtx) {
  auto* pthis = (ZKWrapper*) watcherCtx;
  if (pthis == nullptr) {
    LOG(FATAL) << "zhandle:" << zh;
    return;
  }
  // guard to prevent callback called before Connect() complete
  pthis->zhandle_lock_.rdlock();
  pthis->zhandle_lock_.unlock();
  ZKEvent zkEvent = ZKWrapper_zooEventToZKEvent(type);
  ZKState zkState = ZKWrapper_zooStateToZKState(state);
  pthis->watcher_lock_.rdlock();
  auto watcher = pthis->watcher_;
  pthis->watcher_lock_.unlock();
  if (watcher != nullptr) {
    watcher(zkEvent, zkState, path);
  }
}

ZKWrapper::ZKWrapper(const std::string& remote,
                     WatcherLambda&& watcher,
                     int recv_timeout)
  : zremote_(remote),
    zrecv_timeout_(recv_timeout),
    watcher_(std::move(watcher)),
    zhandle_(nullptr),
    context_id_count_(0) {
      Connect();
}

ZKWrapper::~ZKWrapper() {
  {
    std::lock_guard<std::mutex> guard(finalize_thread_mutex_);
    if (finalize_thread_.joinable()) {
      finalize_thread_.join();
    }
  }
  try {
    Shutdown();
  } catch (const ZKException& exception) {
    LOG(ERROR) << "zk connection close failed, code:" << exception.code()
        << "\n";
  }
}

void ZKWrapper::AddSessionUpdateCallback(std::function<void()>&& callback) {
  std::lock_guard<std::mutex> guard(session_update_callback_mutex_);
  session_update_callbacks_.push_back(std::move(callback));
}

void ZKWrapper::Shutdown() {
  zhandle_lock_.wrlock();
  auto zhandle = zhandle_;
  zhandle_ = nullptr;
  decltype(context_pool_) old_pool;
  {
    std::lock_guard<std::mutex> guard(context_pool_mutex_);
    context_pool_.swap(old_pool);
  }
  zhandle_lock_.unlock();
  if (zhandle != nullptr) {
    DLOG(INFO) << "old zhandle " << zhandle << " closing...("
        << old_pool.size() << " ctxs)";
    // CAUTION:lock guard of this call will cause dead lock
    int rc = zookeeper_close(zhandle);
    DLOG(INFO) << "old zhandle " << zhandle << " closed";
    //DLOG_ASSERT(rc == ZOK) << "return code:" << rc << "\n";
    if (rc != ZOK) {
      throw ZKException(ZKWrapper_zooCodeToZKCode(rc));
    }
  }
}

void ZKWrapper::Connect() {
  // finish unfinished finalize thread to avoid dead lock
  {
    std::lock_guard<std::mutex> guard(finalize_thread_mutex_);
    if (finalize_thread_.joinable()) {
      finalize_thread_.join();
    }
  }
  // Lock the zhandle here to make sure no contexts will be set to the old
  // zhandle and prevent callback called before return
  zhandle_lock_.wrlock();
  auto zhandle = zhandle_;
  if (zhandle != nullptr) {
    // is_unrecoverable will ignore case like state == 0, but this can be caused
    // by a unrecoverable error like unable to connect to zk server due to vipserver
    // dns changed. according to implementation of is_unrecoverable,
    // checking zoo state directly is a better choice
    if (zoo_state(zhandle) > 0) {
      zhandle_lock_.unlock();
      LOG(WARNING) << "Connect should not be called on a living("
          << zoo_state(zhandle) <<
          ") ZKWrapper, ignore connect request";
      return;
    } else {
      std::vector<uint64_t> oldContexts;
      {
        std::lock_guard<std::mutex> guard(context_pool_mutex_);
        for (const auto& pair : context_pool_) {
          oldContexts.push_back(pair.first);
        }
      }
      // old zhandle will be closed from this point
      zhandle = zhandle_;
      zhandle_ = nullptr;
      if (zhandle != nullptr) {
        std::lock_guard<std::mutex> guard(finalize_thread_mutex_);
        finalize_thread_ = std::thread([=]() {
                                       DLOG(INFO) << "old zhandle " << zhandle << " closing...("
                                       << oldContexts.size() << " ctxs)";
                                       zookeeper_close(zhandle);
                                       DLOG(INFO) << "old zhandle " << zhandle << " closed";
                                       std::lock_guard<std::mutex> guard2(context_pool_mutex_);
                                       for (const auto& key : oldContexts) {
                                       context_pool_.erase(key);
                                       }
                                       });
      }
    }
  }
  zhandle = zookeeper_init(zremote_.c_str(), &ZKWrapper::GlobalZooWatcher,
                           zrecv_timeout_, nullptr, this, 0);
  if (zhandle == nullptr) {
    zhandle_lock_.unlock();
    int err = errno;
    if (err == EINVAL || err == ENOENT) {
      throw ZKException(ZK_BADARGUMENTS);
    } else if (err == ENOMEM) {
      throw ZKException(ZK_SYSTEMERROR);
    }
    throw ZKException(ZK_UNDEFINED);
  }
  zhandle_ = zhandle;
  zhandle_lock_.unlock();
  std::vector<std::function<void()>> callbacks;
  session_update_callback_mutex_.lock();
  callbacks.swap(session_update_callbacks_);
  session_update_callback_mutex_.unlock();
  for (const auto& callback : callbacks) {
    callback();
  }
}

ZKWrapper::ZKCode ZKWrapper::Create(const std::string& path,
                                    const char* value,
                                    int valuelen,
                                    bool ephemeral,
                                    bool sequence,
                                    std::string* createdPath) {
  common::ScopedReadLock guard(zhandle_lock_);
  auto zhandle = zhandle_;
  if (zhandle == nullptr) {
    LOG(ERROR) << "null zhandle";
    return ZK_INVALIDSTATE;
  }
  int flags = (ephemeral ? ZOO_EPHEMERAL : 0) | (sequence ? ZOO_SEQUENCE : 0);
  // zk path should not longer than 511 characters.
  // This value in zk python is 256, which will cause problems.
  char buffer[512] = {0};
  int rc = zoo_create(zhandle, path.c_str(), value, valuelen,
                      &ZOO_OPEN_ACL_UNSAFE, flags, buffer, 512);
  if (rc == ZOK && createdPath != nullptr) {
    *createdPath = buffer;
  }
  return ZKWrapper_zooCodeToZKCode(rc);
}

ZKWrapper::ZKCode ZKWrapper::Create(const std::string& path,
                                    const char* value,
                                    int valuelen,
                                    StringCompleteLambda&& complete,
                                    bool ephemeral,
                                    bool sequence) {
  common::ScopedReadLock guard(zhandle_lock_);
  auto zhandle = zhandle_;
  if (zhandle == nullptr) {
    LOG(ERROR) << "null zhandle";
    return ZK_INVALIDSTATE;
  }
  int flags = (ephemeral ? ZOO_EPHEMERAL : 0) | (sequence ? ZOO_SEQUENCE : 0);
  auto context =
      ZKWrapperContext_Create(std::move(complete), this);
  int rc = zoo_acreate(zhandle,
                       path.c_str(),
                       value,
                       valuelen,
                       &ZOO_OPEN_ACL_UNSAFE,
                       flags,
                       &ZKWrapper_LambdaStringCompletion,
                       context.get());
  if (rc != ZOK) {
    RemoveContext(context);
  }
  return ZKWrapper_zooCodeToZKCode(rc);
}

ZKWrapper::ZKCode ZKWrapper::Delete(const std::string& path, int64_t version) {
  common::ScopedReadLock guard(zhandle_lock_);
  auto zhandle = zhandle_;
  if (zhandle == nullptr) {
    LOG(ERROR) << "null zhandle";
    return ZK_INVALIDSTATE;
  }
  return ZKWrapper_zooCodeToZKCode(zoo_delete(zhandle, path.c_str(), version));
}

ZKWrapper::ZKCode ZKWrapper::Delete(const std::string& path,
                                    VoidCompleteLambda&& complete,
                                    int64_t version) {
  common::ScopedReadLock guard(zhandle_lock_);
  auto zhandle = zhandle_;
  if (zhandle == nullptr) {
    LOG(ERROR) << "null zhandle";
    return ZK_INVALIDSTATE;
  }
  auto context =
      ZKWrapperContext_Create(std::move(complete), this);
  int rc = zoo_adelete(zhandle,
                       path.c_str(),
                       version,
                       &ZKWrapper_LambdaVoidCompletion,
                       context.get());
  if (rc != ZOK) {
    RemoveContext(context);
  }
  return ZKWrapper_zooCodeToZKCode(rc);
}

ZKWrapper::ZKCode ZKWrapper::Set(const std::string& path,
                                 const char* buffer,
                                 int buflen,
                                 int64_t version,
                                 ZKStatus* stat) {
  common::ScopedReadLock guard(zhandle_lock_);
  auto zhandle = zhandle_;
  if (zhandle == nullptr) {
    LOG(ERROR) << "null zhandle";
    return ZK_INVALIDSTATE;
  }
  return ZKWrapper_zooCodeToZKCode(zoo_set2(zhandle,
                                            path.c_str(),
                                            buffer,
                                            buflen,
                                            version,
                                            (Stat*) stat));
}

ZKWrapper::ZKCode ZKWrapper::Set(const std::string& path,
                                 const char* buffer,
                                 int buflen,
                                 StatCompleteLambda&& complete,
                                 int64_t version) {
  common::ScopedReadLock guard(zhandle_lock_);
  auto zhandle = zhandle_;
  if (zhandle == nullptr) {
    LOG(ERROR) << "null zhandle";
    return ZK_INVALIDSTATE;
  }
  auto context =
      ZKWrapperContext_Create(std::move(complete), this);
  int rc = zoo_aset(zhandle, path.c_str(), buffer, buflen, version,
                    &ZKWrapper_LambdaStatCompletion, context.get());
  if (rc != ZOK) {
    RemoveContext(context);
  }
  return ZKWrapper_zooCodeToZKCode(rc);
}

ZKWrapper::ZKCode ZKWrapper::Get(const std::string& path,
                                 char* buffer,
                                 int* bufferLen,
                                 ZKStatus* stat,
                                 bool watch) {
  common::ScopedReadLock guard(zhandle_lock_);
  auto zhandle = zhandle_;
  if (zhandle == nullptr) {
    LOG(ERROR) << "null zhandle";
    return ZK_INVALIDSTATE;
  }
  return ZKWrapper_zooCodeToZKCode(zoo_get(zhandle,
                                           path.c_str(),
                                           watch ? 1 : 0,
                                           buffer,
                                           bufferLen,
                                           (Stat*) stat));
}

ZKWrapper::ZKCode
ZKWrapper::Get(const std::string& path,
               char* buffer,
               int* bufferLen,
               WatcherLambda&& watcher,
               ZKStatus* stat) {
  common::ScopedReadLock guard(zhandle_lock_);
  auto zhandle = zhandle_;
  if (zhandle == nullptr) {
    LOG(ERROR) << "null zhandle";
    return ZK_INVALIDSTATE;
  }
  auto context = ZKWrapperContext_Create(std::move(watcher), this);
  int rc = zoo_wget(zhandle, path.c_str(), &ZKWrapper_LambdaWatcher, context.get(),
                    buffer, bufferLen, (Stat*) stat);
  if (rc != ZOK) {
    RemoveContext(context);
  }
  return ZKWrapper_zooCodeToZKCode(rc);
}

ZKWrapper::ZKCode ZKWrapper::Get(const std::string& path,
                                 DataCompleteLambda&& complete,
                                 bool watch) {
  common::ScopedReadLock guard(zhandle_lock_);
  auto zhandle = zhandle_;
  if (zhandle == nullptr) {
    LOG(ERROR) << "null zhandle";
    return ZK_INVALIDSTATE;
  }
  auto context =
      ZKWrapperContext_Create(std::move(complete), this);
  int rc = zoo_aget(zhandle,
                    path.c_str(),
                    watch ? 1 : 0,
                    &ZKWrapper_LambdaDataCompletion,
                    context.get());
  if (rc != ZOK) {
    RemoveContext(context);
  }
  return ZKWrapper_zooCodeToZKCode(rc);
}

ZKWrapper::ZKCode ZKWrapper::Get(const std::string& path,
                                 DataCompleteLambda&& complete,
                                 WatcherLambda&& watcher) {
  common::ScopedReadLock guard(zhandle_lock_);
  auto zhandle = zhandle_;
  if (zhandle == nullptr) {
    LOG(ERROR) << "null zhandle";
    return ZK_INVALIDSTATE;
  }
  auto completeContext =
      ZKWrapperContext_Create(std::move(complete), this);
  auto watcherContext =
      ZKWrapperContext_Create(std::move(watcher), this);
  int rc =
      zoo_awget(zhandle, path.c_str(), &ZKWrapper_LambdaWatcher,
                watcherContext.get(),
                &ZKWrapper_LambdaDataCompletion, completeContext.get());
  if (rc != ZOK) {
    LOG(INFO) << "AsyncGetError:" << path << "[" << zhandle << "]:" << zerror(rc);
    RemoveContext(completeContext);
    RemoveContext(watcherContext);
  }
  return ZKWrapper_zooCodeToZKCode(rc);
}

ZKWrapper::ZKCode ZKWrapper::Exists(const std::string& path,
                                    ZKStatus* stat,
                                    bool watch) {
  common::ScopedReadLock guard(zhandle_lock_);
  auto zhandle = zhandle_;
  if (zhandle == nullptr) {
    LOG(ERROR) << "null zhandle";
    return ZK_INVALIDSTATE;
  }
  return ZKWrapper_zooCodeToZKCode(zoo_exists(zhandle,
                                              path.c_str(),
                                              watch ? 1 : 0,
                                              (Stat*) stat));
}

ZKWrapper::ZKCode ZKWrapper::Exists(const std::string& path,
                                    WatcherLambda&& watcher,
                                    ZKStatus* stat) {
  common::ScopedReadLock guard(zhandle_lock_);
  auto zhandle = zhandle_;
  if (zhandle == nullptr) {
    LOG(ERROR) << "null zhandle";
    return ZK_INVALIDSTATE;
  }
  auto context = ZKWrapperContext_Create(std::move(watcher), this);
  int rc = zoo_wexists(zhandle, path.c_str(), &ZKWrapper_LambdaWatcher,
                       context.get(), (Stat*) stat);
  if (rc != ZOK && rc != ZNONODE) {
    RemoveContext(context);
  }
  return ZKWrapper_zooCodeToZKCode(rc);
}

ZKWrapper::ZKCode ZKWrapper::Exists(const std::string& path,
                                    StatCompleteLambda&& complete,
                                    bool watch) {
  common::ScopedReadLock guard(zhandle_lock_);
  auto zhandle = zhandle_;
  if (zhandle == nullptr) {
    LOG(ERROR) << "null zhandle";
    return ZK_INVALIDSTATE;
  }
  auto context =
      ZKWrapperContext_Create(std::move(complete), this);
  int rc = zoo_aexists(zhandle,
                       path.c_str(),
                       watch ? 1 : 0,
                       &ZKWrapper_LambdaStatCompletion,
                       context.get());
  if (rc != ZOK) {
    RemoveContext(context);
  }
  return ZKWrapper_zooCodeToZKCode(rc);
}

ZKWrapper::ZKCode ZKWrapper::Exists(const std::string& path,
                                    StatCompleteLambda&& complete,
                                    WatcherLambda&& watcher) {
  common::ScopedReadLock guard(zhandle_lock_);
  auto zhandle = zhandle_;
  if (zhandle == nullptr) {
    LOG(ERROR) << "null zhandle";
    return ZK_INVALIDSTATE;
  }
  auto watcherContext =
      ZKWrapperContext_Create(std::move(watcher), this);
  auto completeContext =
      ZKWrapperContext_Create(std::move(complete), this);
  int rc = zoo_awexists(zhandle,
                        path.c_str(),
                        &ZKWrapper_LambdaWatcher,
                        watcherContext.get(),
                        &ZKWrapper_LambdaStatCompletion,
                        completeContext.get());
  if (rc != ZOK) {
    RemoveContext(watcherContext);
    RemoveContext(completeContext);
  }
  return ZKWrapper_zooCodeToZKCode(rc);
}

ZKWrapper::ZKCode
ZKWrapper::GetChildren(const std::string& path,
                       std::vector<std::string>* children,
                       ZKStatus* status,
                       bool watch) {
  common::ScopedReadLock guard(zhandle_lock_);
  auto zhandle = zhandle_;
  if (zhandle == nullptr) {
    LOG(ERROR) << "null zhandle";
    return ZK_INVALIDSTATE;
  }
  String_vector results{0};
  int rc;
  if (status != nullptr) {
    rc = zoo_get_children2(zhandle, path.c_str(), watch ? 1 : 0,
                           &results, (Stat*) status);
  } else {
    rc = zoo_get_children(zhandle, path.c_str(), watch ? 1 : 0,
                          &results);
  }
  if (children != nullptr) {
    children->clear();
    children->reserve(results.count);
    for (int i = 0; i < results.count; i++) {
      children->emplace_back(std::string(results.data[i]));
    }
  }
  deallocate_String_vector(&results);
  return ZKWrapper_zooCodeToZKCode(rc);
}

ZKWrapper::ZKCode ZKWrapper::GetChildren(const std::string& path,
                                         std::vector<std::string>* children,
                                         WatcherLambda&& watcher,
                                         ZKStatus* status) {
  common::ScopedReadLock guard(zhandle_lock_);
  auto zhandle = zhandle_;
  if (zhandle == nullptr) {
    LOG(ERROR) << "null zhandle";
    return ZK_INVALIDSTATE;
  }
  String_vector results{0};
  auto context = ZKWrapperContext_Create(std::move(watcher), this);
  int rc;
  if (status != nullptr) {
    rc = zoo_wget_children2(zhandle, path.c_str(), &ZKWrapper_LambdaWatcher,
                            context.get(), &results, (Stat*) status);
  } else {
    rc = zoo_wget_children(zhandle, path.c_str(), &ZKWrapper_LambdaWatcher,
                           context.get(), &results);
  }
  if (rc != ZOK) {
    RemoveContext(context);
  }
  if (children != nullptr) {
    children->clear();
    for (int i = 0; i < results.count; i++) {
      children->push_back(std::string(results.data[i]));
    }
  }
  deallocate_String_vector(&results);
  return ZKWrapper_zooCodeToZKCode(rc);
}

ZKWrapper::ZKCode ZKWrapper::GetChildren(const std::string& path,
                                         StringArrayStatCompleteLambda&& complete,
                                         WatcherLambda&& watcher) {
  common::ScopedReadLock guard(zhandle_lock_);
  auto zhandle = zhandle_;
  if (zhandle == nullptr) {
    LOG(ERROR) << "null zhandle";
    return ZK_INVALIDSTATE;
  }
  auto watcherContext =
      ZKWrapperContext_Create(std::move(watcher), this);
  auto completeContext =
      ZKWrapperContext_Create(std::move(complete), this);
  int rc = zoo_awget_children2(zhandle,
                               path.c_str(),
                               &ZKWrapper_LambdaWatcher,
                               watcherContext.get(),
                               &ZKWrapper_LambdaStringArrayStatCompletion,
                               completeContext.get());
  if (rc != ZOK) {
    RemoveContext(watcherContext);
    RemoveContext(completeContext);
  }
  return ZKWrapper_zooCodeToZKCode(rc);
}

void ZKWrapper::SetWatcher(WatcherLambda&& watcher) {
  watcher_lock_.wrlock();
  watcher_ = std::move(watcher);
  watcher_lock_.unlock();
}

int64_t ZKWrapper::GetID() {
  int64_t id = 0;
  zhandle_lock_.rdlock();
  if (zhandle_ != nullptr) {
    id = zoo_client_id(zhandle_)->client_id;
  }
  zhandle_lock_.unlock();
  return id;
}

ZKWrapper::ZKState ZKWrapper::GetState() {
  int state = 0;
  zhandle_lock_.rdlock();
  if (zhandle_ != nullptr) {
    state = zoo_state(zhandle_);
  }
  zhandle_lock_.unlock();
  return state == 0 ? ZKSTATE_UNDEFINED : ZKWrapper_zooStateToZKState(state);
}

const char* ZKWrapper::GetMessage(ZKCode code) {
  return zerror(ZKWrapper_ZKCodeTozooCode(code));
}

std::string ZKWrapper::JoinPath(const std::string& path1,
                                const std::string& path2) {
  if (path1.length() == 0 || path2.length() == 0) {
    return path1 + path2;
  }
  bool firstTail = path1[path1.length() - 1] == '/';
  bool secondHead = path2[0] == '/';
  int length = path1.length() - (firstTail ? 1 : 0)
      + path2.length() - (secondHead ? 1 : 0) + 1;
  std::string result(length, 0);
  char* mutable_data = const_cast<char*>(result.data());
  length = path1.length() - (firstTail ? 1 : 0);
  memcpy(mutable_data, path1.data(), length);
  mutable_data[length] = '/';
  length++;
  mutable_data += length;
  memcpy(mutable_data, path2.data() + (secondHead ? 1 : 0),
         path2.length() - (secondHead ? 1 : 0));
  return result;
}

}  // namespace leader
