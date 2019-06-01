/*!
 * \file brpc_bad_node_detector.h
 * \brief The BRPC bad node detector
 */
#ifndef LEADER_BRPC_BRPC_BAD_NODE_DETECTOR_H_
#define LEADER_BRPC_BRPC_BAD_NODE_DETECTOR_H_

#include <thread>

#include "common/thread_running_checker.h"
#include "leader/bad_node_detector.h"
#include "leader/brpc/brpc_channel_pool.h"

namespace leader {

class BrpcBadNodeDetector : public BadNodeDetector {
 public:
  BrpcBadNodeDetector(NodeCollectionInterface* node_collection_interface,
                      BrpcChannelPool* channel_pool);
  ~BrpcBadNodeDetector();

  void StartDetect() override;
  void StopDetect() override;
 
 private:
  BrpcBadNodeDetector(const BrpcBadNodeDetector &) = delete;
  BrpcBadNodeDetector& operator=(const BrpcBadNodeDetector &) = delete;

  void DoDetect(std::set<std::string>& specList);

  BrpcChannelPool* channel_pool_;
  std::thread detect_thread_;
  common::ThreadRunningChecker running_;
};

}  // namespace leader

#endif // LEADER_BRPC_BRPC_BAD_NODE_DETECTOR_H_
