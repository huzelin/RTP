/*!
 * \file hearbeat_reporter.cc
 * \brief Heartbeat reporter
 */
#include "leader/heartbeat_reporter.h"

#include <utility>
#include <string>

#include "common/logging.h"

namespace leader {

HeartbeatReporter::HeartbeatReporter()
    : zk_(nullptr), shared_zk_(false), heartbeat_interval_seconds_(0) { }

HeartbeatReporter::~HeartbeatReporter() {
  DLOG(INFO) << "Quit heartbeat.";
  Close();
  if (!shared_zk_) {
    delete zk_;
  }
  zk_ = nullptr;
}

bool HeartbeatReporter::Init(const std::string& zkHost,
                             const HeartbeatMessage& msg, uint32_t hbInterval) {
  if (zk_ != nullptr) {
    LOG(ERROR) << "Already initialized";
    return false;
  }
  try {
    shared_zk_ = false;
    zk_ = new ZKWrapper(zkHost);
  } catch (const std::exception& exception) {
    LOG(ERROR) << exception.what();
    return false;
  }

  heartbeat_message_ = msg;
  if (!msg.has_ip() || !msg.has_port()) {
    LOG(ERROR) << "Invalid heartbeat message.";
    return false;
  }

  heartbeat_interval_seconds_ = hbInterval;
  return true;
}

bool HeartbeatReporter::Init(ZKWrapper* zk,
                             const HeartbeatMessage& msg,
                             uint32_t hbInterval) {
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

  heartbeat_message_ = msg;
  if (!msg.has_ip() || !msg.has_port()) {
    LOG(ERROR) << "Invalid heartbeat message.";
    return false;
  }

  heartbeat_interval_seconds_ = hbInterval;
  return true;
}

void HeartbeatReporter::AddPathToReport(const std::string& p) {
  // for skeeper subscriber compability, we still write pb to node content
  // between leader instances, we only use node name
  auto path = ZKWrapper::JoinPath(p, heartbeat_message_.ip()
                                  + ":" + std::to_string(heartbeat_message_.port()));
  {
    common::ScopedWriteLock guard(node_path_lock_);
    auto iter = node_path_map_.find(path);
    if (iter == node_path_map_.end()) {
      node_path_map_[path] = std::make_shared<std::atomic<bool>>(false);
    } else {
      *(iter->second) = false;
    }
  }
  DLOG(INFO) << "Wake up to add " << path;
  WakeUp();
}

void HeartbeatReporter::RemovePath(const std::string& remove,
                                   std::shared_future<void>* removeFuture) {
  auto path =
      ZKWrapper::JoinPath(remove,
                          heartbeat_message_.ip() + ":"
                          + std::to_string(heartbeat_message_.port()));
  bool exist;
  {
    common::ScopedWriteLock guard(node_path_lock_);
    exist = node_path_map_.erase(path) > 0;
  }
  node_path_removed_mutex_.lock();
  std::shared_ptr<std::promise<void>> p;
  std::shared_future<void> f;
  auto iter = node_path_to_remove_.find(path);
  if (iter == node_path_to_remove_.end()) {
    // if path is not removed before and don't need a future,
    // put a placeholder in the path to remove map
    if (removeFuture == nullptr) {
      node_path_to_remove_[path] = nullptr;
    }
  } else if (iter->second != nullptr && removeFuture != nullptr) {
    // path is in progress of removal, get shared future and promise
    p = iter->second->first;
    f = iter->second->second;
  }
  // if path not existed, there's no need to create promise and future for it
  if (exist && removeFuture != nullptr && p == nullptr) {
    p = std::make_shared<std::promise<void >>();
    f = p->get_future().share();
    node_path_to_remove_[path] =
        std::make_shared<std::pair<std::shared_ptr<std::promise<void>>,
        std::shared_future<void>>>(p, f);
  }
  node_path_removed_mutex_.unlock();
  DLOG(INFO) << "Wake up to remove " << path;
  WakeUp();
  if (removeFuture != nullptr) {
    *removeFuture = f;
  }
}

bool HeartbeatReporter::Report(const std::string& path,
                               const std::shared_ptr<std::atomic<bool>>& force) {
  ZKWrapper::ZKStatus status;
  // call sync Exists first to make sure ZK init complete
  auto rc = zk_->Exists(path, &status);
  switch (zk_->GetState()) {
    case
        ZKWrapper::ZKSTATE_CONNECTED:
        // zk correctly connected, continue
        break;
    case
        ZKWrapper::ZKSTATE_CONNECTING:
        case
        ZKWrapper::ZKSTATE_ASSOCIATING:
        // zk connect in progress, this is not normal
        // since sync Exists() has returned, log and reconnect
        LOG(ERROR)
        << "ZK should not be connecting after sync api call, reconnect";
    default:
    try {
      zk_->Connect();
    }
    catch (const ZKWrapper::ZKException& exception) {
      LOG(ERROR) << exception.what();
      return false;
    }
    break;
  }
  bool isNewSession = true;
  if (rc == ZKWrapper::ZK_OK) {
    if (status.ephemeralOwner != zk_->GetID()) {
      LOG(WARNING)
          << "node " << path << " is created by session "
          << status.ephemeralOwner << " not current session("
          << zk_->GetID() << ").";
      rc = zk_->Delete(path);
      if (rc != ZKWrapper::ZK_OK) {
        LOG(ERROR)
            << "failed delete foreign node " << path
            << "created by " << status.ephemeralOwner
            << ":" <<
            ZKWrapper::GetMessage(rc)
            << "(" << rc << ")";
      }
    } else {
      if (!*force) {  // everything looks good, return if not forced
        return true;
      }
      isNewSession = false;
    }
  } else if (rc != ZKWrapper::ZK_NONODE) {
    LOG(WARNING)
        << "check node " << path << " failed "
        <<
        ZKWrapper::GetMessage(rc)
        << "(" << rc << ")";
    return false;
  }

  std::string msg;
  if (!heartbeat_message_.SerializeToString(&msg)) {
    LOG(ERROR) << "Serialize hb message failed.";
    return false;
  }
  if (isNewSession) {
    rc = zk_->Create(path, msg.c_str(), msg.size(), true);
  } else {
    rc = zk_->Set(path, msg.c_str(), msg.size());
  }
  if (rc != ZKWrapper::ZK_OK) {
    LOG(WARNING)
        << (isNewSession ? "create" : "set") << " node " << path
        << " failed " <<
        ZKWrapper::GetMessage(rc)
        << "(" << rc << ")";
    return false;
  }
  rc = zk_->Exists(path, &status);
  if (rc == ZKWrapper::ZK_NONODE) {
    LOG(WARNING)
        << "HB node " << path << " not exist, report again!";
    return false;
  } else if (rc != ZKWrapper::ZK_OK) {
    LOG(WARNING)
        << "HB node " << path << " existence check failed:"
        <<
        ZKWrapper::GetMessage(rc)
        << "(" << rc << ")";
    return false;
  }

  if (status.ephemeralOwner != zk_->GetID()) {
    LOG(WARNING)
        << "node " << path << " is not created by current session";
    return false;
  }
  return true;
}

void HeartbeatReporter::Close() {
  StopReport();
  if (zk_ != nullptr && !shared_zk_) {
    zk_->Shutdown();
  }
}

bool HeartbeatReporter::StartReport() {
  if (running_checker_) {
    LOG(WARNING) << "Heartbeat thread is already running.";
    return false;
  }
  if (zk_->GetState() == ZKWrapper::ZKSTATE_UNDEFINED) {
    zk_->Connect();
  }
  running_checker_.Start();
  heartbeat_thread_ = std::thread([this]() {
                                  while (running_checker_.IsRunning()) {
                                  DLOG(INFO) << "Do a heartbeat check.";
                                  decltype(node_path_map_) pathMap;
                                  decltype(node_path_to_remove_) pathsToRemove;
                                  {
                                  common::ScopedReadLock guard(node_path_lock_);
                                  pathMap = node_path_map_;
                                  }
                                  {
                                  std::lock_guard<std::mutex> guard(node_path_removed_mutex_);
                                  pathsToRemove.swap(node_path_to_remove_);
                                  }
                                  for (const auto& remove : pathsToRemove) {
                                  zk_->Delete(remove.first, [=](ZKWrapper::ZKCode rc) {
                                              if (rc != ZKWrapper::ZK_OK && rc != ZKWrapper::ZK_NONODE) {
                                              LOG(ERROR) << "Remove " << remove.first << " failed:"
                                              << ZKWrapper::GetMessage(rc) << "(" << rc << ").";
                                              std::lock_guard<std::mutex> guard(node_path_removed_mutex_);
                                              node_path_to_remove_.insert(remove);
                                              return;
                                              }
                                              if (remove.second != nullptr) {
                                              try {
                                              remove.second->first->set_value();
                                              } catch (const std::exception& e) {
                                              LOG(ERROR) << e.what();
                                              }
                                              }
                                              });
                                  }
                                  for (const auto& pair : pathMap) {
                                    if(!Report(pair.first, pair.second)) {
                                      pair.second->store(true);
                                    }
                                  }
                                  if (!skip_sleep_) {
                                    // TODO: do random length sleep to prevent zk request flooding
                                    running_checker_.Sleep(heartbeat_interval_seconds_ * 1000);
                                  }
                                  skip_sleep_.store(false);
                                  }
  });
  LOG(INFO) << "Start heartbeat thread successfully. ";
  return true;
}

void HeartbeatReporter::WakeUp() {
  skip_sleep_ = true;
  running_checker_.WakeUp();
}

bool HeartbeatReporter::StopReport() {
  if (running_checker_.IsRunning()) {
    running_checker_.Stop();
    if (heartbeat_thread_.joinable()) {
      heartbeat_thread_.join();
    }
  }
  return true;
}

}  // namespace leader
