/*!
 * \file brpc_bad_node_detector.cc
 * \brief GRPC bad node detector
 */
#include "leader/brpc/brpc_bad_node_detector.h"

#include <map>
#include <set>
#include <string>
#include <vector>

#include "common/logging.h"
#include "leader/heartbeat_message_helper.h"

namespace {
const static uint32_t BAD_NODE_DETECT_INTERVAL = 3;
}  // namespace

namespace leader {

BrpcBadNodeDetector::BrpcBadNodeDetector(NodeCollectionInterface* node_collection,
                                         BrpcChannelPool* channel_pool) :
    BadNodeDetector(node_collection),
    channel_pool_(channel_pool),
    running_(false) { }

BrpcBadNodeDetector::~BrpcBadNodeDetector() {
  StopDetect();
}

void BrpcBadNodeDetector::StartDetect() {
  if (running_) {
    LOG(WARNING) << "Bad node detector already started";
    return;
  }
  running_.Start();
  detect_thread_ = std::thread([=]() {
    std::set<std::string> blacklist;
    while (running_.IsRunning()) {
      DoDetect(blacklist);
      running_.Sleep(BAD_NODE_DETECT_INTERVAL * 1000);
    }
  });
  LOG(INFO) << "Start bad node detect successfully.";
}

void BrpcBadNodeDetector::StopDetect() {
  running_.Stop();
  if (detect_thread_.joinable()) {
    detect_thread_.join();
  }
}

void BrpcBadNodeDetector::DoDetect(std::set<std::string>& specList) {
  std::set<std::string> nodeBlackList;
  // check and get channel-pool blacklist
  std::set<std::string> specBlackList;
  channel_pool_->CheckAndGetBadNodeSpecs(specBlackList);
  if (specList.empty() && specBlackList.empty()) {
     return;
  }
  // get zk blacklist
  std::map<std::string, std::string> zkBlackListInfo;
  node_collection_->FindNodesBySpec(specBlackList,
                                    zkBlackListInfo);
  for (const auto& pair : zkBlackListInfo) {
    nodeBlackList.insert(pair.second);
  }
  node_collection_->UpdateBadNode(nodeBlackList); // refresh zk blacklist
  specList.swap(specBlackList);
}

}  // namespace leader
