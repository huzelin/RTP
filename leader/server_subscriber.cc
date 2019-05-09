
#include "common/lock.h"
#include "common/logging.h"

#include "leader/server_subscriber.h"
#include "leader/heartbeat_receiver.h"
#include "leader/node_collection_impl.h"
#include "leader/bad_node_detector.h"

#include <vector>
#include <string>

namespace leader {

const int GET_CHANNEL_RETRY_TIMES = 3;

ServerSubscriber::ServerSubscriber()
    : bad_node_detector_(nullptr),
    node_collection_(new NodeCollectionImpl()),
    heartbeat_receiver_(new HeartbeatReceiver()) { }

ServerSubscriber::~ServerSubscriber() {
  Close();
}

bool ServerSubscriber::Start() {
  if (bad_node_detector_ == nullptr) {
    return false;
  }
  bad_node_detector_->StartDetect();
  heartbeat_receiver_->StartReceive();
  return true;
}

void ServerSubscriber::Close() {
  heartbeat_receiver_->Close();
  if (bad_node_detector_ != nullptr) {
    bad_node_detector_->StopDetect();
  }
}

bool ServerSubscriber::Init(const std::string& host,
                            uint32_t timeout,
                            int32_t workThreadNum,
                            int32_t ioThreadNum,
                            uint32_t channelCount,
                            size_t channelQueueSize,
                            uint32_t hbInterval) {
  return SharedInitializer(timeout,
                           workThreadNum,
                           ioThreadNum,
                           channelCount,
                           channelQueueSize)
      && heartbeat_receiver_->Init(host, node_collection_.get(), hbInterval);
}

bool ServerSubscriber::Init(ZKWrapper* zk,
                            uint32_t timeout,
                            int32_t workThreadNum,
                            int32_t ioThreadNum,
                            uint32_t channelCount,
                            size_t channelQueueSize,
                            uint32_t hbInterval) {
  return SharedInitializer(timeout,
                           workThreadNum,
                           ioThreadNum,
                           channelCount,
                           channelQueueSize)
      && heartbeat_receiver_->Init(zk, node_collection_.get(), hbInterval);
}

void ServerSubscriber::AddPath(const std::string& path) {
  heartbeat_receiver_->AddPath(path);
}

void ServerSubscriber::AddPaths(const std::vector<std::string>& paths) {
  heartbeat_receiver_->AddPaths(paths);
}

void ServerSubscriber::RemovePath(const std::string& path) {
  heartbeat_receiver_->RemovePath(path);
  node_collection_->RemovePath(path);
}

void ServerSubscriber::RemovePath(const std::vector<std::string>& paths) {
  heartbeat_receiver_->RemovePath(paths);
  node_collection_->RemovePath(paths);
}

size_t ServerSubscriber::GetServerCount(const std::string& path) {
  return node_collection_->GetNodeCount(path);
}

}  // namespace leader
