/*!
 * \file heartbeat_receiver.cc
 * \brief The heartbeat receiver
 */
#include "leader/heartbeat_receiver.h"

#include <set>
#include <vector>
#include <string>
#include <mutex>
#include <chrono>

#include "common/logging.h"

namespace leader {

#define INVALID_CHILDVERSION  -1

static uint64_t CurrentUTCEpoch() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()
  ).count();
}

HeartbeatReceiver::HeartbeatReceiver()
    : zk_(nullptr), shared_zk_(false),
      node_collection_(nullptr), running_checker_(false),
      heartbeat_interval_(0), skip_sleep_(false) {
}

HeartbeatReceiver::~HeartbeatReceiver() {
  Close();
  if (!shared_zk_) {
    delete (zk_);
  }
  zk_ = nullptr;
}

void HeartbeatReceiver::CommonWatcher(ZKWrapper::ZKEvent event,
                                      ZKWrapper::ZKState state,
                                      const char* path) {
  if (state != ZKWrapper::ZKSTATE_CONNECTED) {
    return;
  }
  switch (event) {
    case ZKWrapper::ZKEVENT_CREATED:
    case ZKWrapper::ZKEVENT_DELETED:
    case ZKWrapper::ZKEVENT_CHILD: {
      paths_lock_.rdlock();
      auto iter = paths_.find(path);
      if (iter != paths_.end()) {
        // set next_scan_time to 0 to force a update
        iter->second->next_scan_time = 0;
      }
      paths_lock_.unlock();
      WakeUp();
      break;
    }
    default:
      return;
  }
}

bool HeartbeatReceiver::Init(const std::string& zkHost,
                             NodeCollectionInterface* nodeCollection,
                             uint32_t interval) {
  if (zk_ != nullptr) {
    LOG(ERROR) << "Already initialized";
    return false;
  }
  shared_zk_ = false;
  zk_ = new ZKWrapper(zkHost,
                      [this](ZKWrapper::ZKEvent event,
                             ZKWrapper::ZKState state,
                             const char* path) {
                        CommonWatcher(event, state, path);
                      });
  node_collection_ = nodeCollection;
  heartbeat_interval_ = interval;
  return true;
}

bool HeartbeatReceiver::Init(ZKWrapper* zk,
                             NodeCollectionInterface* nodeCollection,
                             uint32_t interval) {
  if (zk_ != nullptr) {
    LOG(ERROR) << "Already initialized";
    return false;
  }
  if (zk == nullptr) {
    LOG(ERROR) << "shared zk object can't be null";
    return false;
  }
  shared_zk_ = true;
  zk_ = zk;
  zk_->SetWatcher([this](ZKWrapper::ZKEvent event,
                         ZKWrapper::ZKState state,
                         const char* path) {
    CommonWatcher(event, state, path);
  });
  node_collection_ = nodeCollection;
  heartbeat_interval_ = interval;
  return true;
}

void HeartbeatReceiver::AddPath(const std::string& path) {
  AddPaths(std::vector<std::string>({path}));
}

void HeartbeatReceiver::AddPaths(const std::vector<std::string>& paths) {
  {
    common::ScopedWriteLock guard(paths_lock_);
    for (const auto& path : paths) {
      auto pairI = paths_.find(path);
      if (pairI == paths_.end()) {
        paths_[path] = std::make_shared<NodeStatus>(INVALID_CHILDVERSION);
      } else {
        (pairI->second)->next_scan_time = 0;
      }
    }
  }
  WakeUp();
}

void HeartbeatReceiver::RemovePath(const std::string& path) {
  RemovePath(std::vector<std::string>({path}));
}

void HeartbeatReceiver::RemovePath(const std::vector<std::string>& paths) {
  std::set<std::string> removeList;
  {
    common::ScopedWriteLock guard(paths_lock_);
    for (const auto& pair : paths_) {
      for (const auto& path : paths) {
        if (strncmp(pair.first.c_str(), path.c_str(), path.size()) == 0
            && (pair.first.size() == path.size()
                || pair.first[path.size()] == '/')) {
          removeList.insert(pair.first);
        }
      }
    }
    for (const auto& remove : removeList) {
      auto iter = paths_.find(remove);
      if (iter == paths_.end()) {
        continue;
      }
      // path removed, don't need to scan ever;
      iter->second->next_scan_time = UINT64_MAX;
      paths_.erase(iter);
    }
  }
  WakeUp();
  // use this lock to make sure remove will take effect after return
  std::lock_guard<std::mutex> guard(update_lock_);
}

void HeartbeatReceiver::Close() {
  StopReceive();
  if (zk_ != nullptr) {
    if (shared_zk_) {
      zk_->SetWatcher(nullptr);
    } else {
      zk_->Shutdown();
    }
  }
}

void HeartbeatReceiver::Receive(const std::string& path,
                                NodeStatus& node_status) {
  if (node_status.next_scan_time > CurrentUTCEpoch()) {
    return;
  }
  // call sync Exists first to make sure ZK init complete
  ZKWrapper::ZKStatus status;
  auto rc = zk_->Exists(path, &status, true);
  switch (zk_->GetState()) {
    case ZKWrapper::ZKSTATE_CONNECTED:
      // zk correctly connected, continue
      break;
    case ZKWrapper::ZKSTATE_CONNECTING:
    case ZKWrapper::ZKSTATE_ASSOCIATING:
      // zk connect in progress, this is not normal since sync Exists() has returned,
      // log and reconnect
      LOG(ERROR)
          << "ZK should not be connecting after sync api call, reconnect";
    default:
      try {
        zk_->Connect();
      } catch (const ZKWrapper::ZKException& exception) {
        // zk connection issue, recheck after next scan
        LOG(ERROR) << exception.what();
        return;
      }
      break;
  }

  if (rc == ZKWrapper::ZK_NONODE) {
    LOG(WARNING) << path << " doesn't exist, rescan in 30 secs";
    node_status.next_scan_time = CurrentUTCEpoch() + 30 * 1000;
    return;
  } else if (rc != ZKWrapper::ZK_OK) {
    // zk connection issue, recheck after next scan
    LOG(ERROR) << "Check " << path << " failed:" << ZKWrapper::GetMessage(rc)
               << "(" << rc << ").";
    return;
  }
  int32_t cversion = node_status.cversion;
  uint64_t next_scan_time = node_status.next_scan_time;
  if (cversion == INVALID_CHILDVERSION || cversion != status.cversion) {
    // New path or watcher triggered, need update children and re-register watcher
    thread_local static std::vector<std::string> children;
    rc = zk_->GetChildren(path, &children, &status, true);
    if (rc != ZKWrapper::ZK_OK) {
      // unexpected error, recheck after next scan
      LOG(ERROR) << "Get children failed:" << ZKWrapper::GetMessage(rc) << "("
                 << rc << ").";
      return;
    }
    node_collection_->UpdateHeartbeat(children, path);
    if (node_status.cversion.compare_exchange_strong(cversion,
                                                     status.cversion)) {
      // update success, cversion not changed during update,
      // check again after 5 secs
      bool next_scan_time_changed = !node_status.next_scan_time
          .compare_exchange_strong(next_scan_time,
                                   CurrentUTCEpoch() + 5 * 1000);
      // if next scan time modified to not forever, watcher must be triggered
      // do skip sleep and rescan ASAP
      if (next_scan_time_changed
          && node_status.next_scan_time != UINT64_MAX
          && !skip_sleep_) {
        skip_sleep_ = true;
      }
    } else {
      // cversion changed unexpectedly
      if (node_status.next_scan_time != UINT64_MAX) {
        node_status.next_scan_time = 0;
        skip_sleep_ = true;
      }
    }
  }
}

bool HeartbeatReceiver::StartReceive() {
  if (running_checker_) {
    LOG(WARNING) << "Heartbeat Receive thread is already running.";
    return false;
  }
  if (zk_->GetState() == ZKWrapper::ZKSTATE_UNDEFINED) {
    zk_->Connect();
  }
  running_checker_.Start();
  heartbeat_thread_ = std::thread([this]() {
    while (running_checker_) {
      decltype(paths_) pathMap;
      std::vector<std::string> pathsToRemove;
      {
        std::lock_guard<std::mutex> updateLockGuard(update_lock_);
        {
          common::ScopedReadLock guard(paths_lock_);
          pathMap = paths_;
        }
        for (const auto& pair : pathMap) {
          Receive(pair.first, *pair.second);
        }
        auto skipSleep = skip_sleep_.exchange(false);
        if (skipSleep) {
          continue;
        }
      }
      // TODO: do random length sleep to prevent zk request flooding
      running_checker_.Sleep(heartbeat_interval_ * 1000);
    }
  });
  LOG(INFO) << "Start Heartbeat Receive thread successfully.";
  return true;
}

void HeartbeatReceiver::WakeUp() {
  skip_sleep_ = true;
  running_checker_.WakeUp();
}

bool HeartbeatReceiver::StopReceive() {
  if (running_checker_) {
    running_checker_.Stop();
    if (heartbeat_thread_.joinable()) {
      heartbeat_thread_.join();
    }
  }
  return true;
}

}  // namespace leader
