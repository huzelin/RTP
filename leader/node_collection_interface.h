/*!
 * \file node_collection_interface.h
 * \brief The node collection interface
 */
#ifndef LEADER_NODE_COLLECTION_INTERFACE_H_
#define LEADER_NODE_COLLECTION_INTERFACE_H_

#include <string>
#include <vector>
#include <set>
#include <map>

#include "heartbeat_message.pb.h"

namespace leader {

class NodeCollectionInterface {
 public:
  NodeCollectionInterface() = default;

  virtual ~NodeCollectionInterface() {}

  NodeCollectionInterface(const NodeCollectionInterface&) = delete;

  NodeCollectionInterface& operator=(const NodeCollectionInterface&) = delete;

  virtual void UpdateHeartbeat(const std::vector<std::string>& nodes,
                               const std::string& path) = 0;

  virtual void UpdateBadNode(std::set<std::string>& blackList) = 0;

  virtual void FindNodesBySpec(const std::set<std::string>& specLists,
                               std::map<std::string,
                               std::string>& nodes) = 0;

  virtual bool PickOneNode(std::string& node,
                           const std::string& path) = 0;

  virtual size_t GetNodeCount(const std::string& path) = 0;

  /**
   * @brief Remove path and all sub-paths
   * @param path
   */
  virtual void RemovePath(const std::string& path) = 0;

  /**
   * @brief Remove path and all sub-paths
   * @param path
   */
  virtual void RemovePath(const std::vector<std::string>& path) = 0;
};

}  // namespace leader

#endif  // LEADER_NODE_COLLECTION_INTERFACE_H_
