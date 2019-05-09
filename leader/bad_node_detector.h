/*!
 * \file bad_node_detector.h
 * \brief The bad node detector
 */
#ifndef LEADER_BAD_NODE_DETECTOR_H_
#define LEADER_BAD_NODE_DETECTOR_H_

#include "leader/node_collection_interface.h"

namespace leader {

class BadNodeDetector {
 public:
  BadNodeDetector(NodeCollectionInterface* node_collection) :
      node_collection_(node_collection) { }
  virtual ~BadNodeDetector() { }
  
  virtual void StartDetect() = 0;
  virtual void StopDetect() = 0;

 protected:
  BadNodeDetector(const BadNodeDetector &);
  BadNodeDetector& operator=(const BadNodeDetector &);
  
  NodeCollectionInterface* node_collection_;
};

}  // namespace leader

#endif  // LEADER_BAD_NODE_DETECTOR_H_
