/*!
 * \file grpc_bad_node_detector.h
 * \brief The GRPC bad node detector
 */
#ifndef LEADER_GRPC_GRPC_BAD_NODE_DETECTOR_H_
#define LEADER_GRPC_GRPC_BAD_NODE_DETECTOR_H_

#include <thread>

#include "common/thread_running_checker.h"
#include "leader/bad_node_detector.h"
#include "leader/grpc/grpc_channel_pool.h"

namespace leader {

class GrpcBadNodeDetector : public BadNodeDetector {
 public:
  GrpcBadNodeDetector(NodeCollectionInterface* node_collection_interface,
                      GrpcChannelPool* channel_pool);
  ~GrpcBadNodeDetector();

  void StartDetect() override;
  void StopDetect() override;
 
 private:
  GrpcBadNodeDetector(const GrpcBadNodeDetector &) = delete;
  GrpcBadNodeDetector& operator=(const GrpcBadNodeDetector &) = delete;

  void DoDetect(std::set<std::string>& specList);

  GrpcChannelPool* channel_pool_;
  std::thread detect_thread_;
  common::ThreadRunningChecker running_;
};

}  // namespace leader

#endif // LEADER_GRPC_GRPC_BAD_NODE_DETECTOR_H_
