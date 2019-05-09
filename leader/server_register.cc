/*!
 * \file server_register.cc
 * \brief The basic server resgiter impls
 */
#include <string>
#include <vector>

#include "common/logging.h"

#include "leader/server_register.h"
#include "leader/heartbeat_reporter.h"
#include "leader/net_util.h"

namespace leader {

ServerRegister::ServerRegister()
    : heartbeat_reporter_(nullptr) { }

ServerRegister::~ServerRegister() {
  Close();
}

void ServerRegister::AddPath(const std::string &path) {
  if (heartbeat_reporter_ == nullptr) {
    LOG(ERROR) << "HB reporter not initialized";
    return;
  }
  heartbeat_reporter_->AddPathToReport(path);
}

void ServerRegister::RemovePath(const std::string& path, bool blocking) {
  if (heartbeat_reporter_ == nullptr) {
    LOG(ERROR) << "HB reporter not initialized";
    return;
  }
  if (blocking) {
    std::shared_future<void> removed;
    heartbeat_reporter_->RemovePath(path, &removed);
    if (removed.valid()) {
      removed.get();
    }
  } else {
    heartbeat_reporter_->RemovePath(path);
  }
}

void ServerRegister::RemovePath(const std::vector<std::string>& paths,
                                bool blocking) {
  std::vector<std::shared_future<void>> removedFutures(
      blocking ? paths.size() : 0);
  for (int i = 0; i < paths.size(); i++) {
    heartbeat_reporter_->RemovePath(paths[i],
                                    blocking ? &removedFutures[i] : nullptr);
  }
  for (const auto& removed : removedFutures) {
    if (removed.valid()) {
      removed.get();
    }
  }
}

bool ServerRegister::Init(const std::string& zkHost, uint32_t port,
                          const std::string& ip, int32_t hbInterval) {
  if (!SharedInitializer(port, ip)) {
    return false;
  }
  HeartbeatMessage heartbeatMessage;
  heartbeatMessage.set_ip(ip_);
  heartbeatMessage.set_port(port_);
  if (!heartbeat_reporter_->Init(zkHost, heartbeatMessage, hbInterval)) {
    LOG(ERROR) << "Init HeartbeatReporter failed.";
    return false;
  }
  return true;
}

bool ServerRegister::Init(ZKWrapper* zk,
                          uint32_t port,
                          const std::string& ip,
                          int32_t hbInterval) {
  if (!SharedInitializer(port, ip)) {
    return false;
  }
  HeartbeatMessage heartbeatMessage;
  heartbeatMessage.set_ip(ip_);
  heartbeatMessage.set_port(port_);
  if (!heartbeat_reporter_->Init(zk, heartbeatMessage, hbInterval)) {
    LOG(ERROR) << "Init HeartbeatReporter failed.";
    return false;
  }
  return true;
}

bool ServerRegister::SharedInitializer(uint32_t port, const std::string& ip) {
  port_ = port;
  ip_ = ip;
  delete heartbeat_reporter_;
  heartbeat_reporter_ = new HeartbeatReporter();

  if (ip_.empty()) {
    if (!AutomaticGetIP()) {
      LOG(ERROR) << "Automatic Get Ip failed. ";
      return false;
    }
  }
  if (ip_.empty() || port_ > 65535) {
    LOG(ERROR) << "Invalid IP[" << ip_.c_str() << "] or Port[" << port_ << "] ";
    return false;
  }
  // don't support hostname for now
  //  if (mHost.empty()) {
  //    if (AutomaticGetHost()) {
  //      mHeartbeatMsg.set_host(mHost);
  //    }
  //  }
  return true;
}

bool ServerRegister::Start() {
  if (!heartbeat_reporter_->StartReport()) {
    LOG(ERROR) << "Start heartbeat failed.";
    return false;
  }
  return true;
}

void ServerRegister::Close() {
  if (heartbeat_reporter_) {
    heartbeat_reporter_->Close();
    delete heartbeat_reporter_;
    heartbeat_reporter_ = nullptr;
  }
}

bool ServerRegister::AutomaticGetIP() {
  std::vector<std::string> ipList;
  if (NetUtil::GetIP(ipList) && !ipList.empty()) {
    ip_ = ipList[0];
    return true;
  }
  return false;
}

bool ServerRegister::AutomaticGetHost() {
  return NetUtil::GetHostName(host_);
}

}  // namespace leader
