/*!
 * \file node_collection_impl.h
 * \brief The node collection implementation
 */
#ifndef LEADER_NODE_COLLECTION_IMPL_H_
#define LEADER_NODE_COLLECTION_IMPL_H_

#include <string>
#include <map>
#include <set>
#include <vector>
#include <mutex>
#include <memory>

#include "leader/node_collection_interface.h"
#include "common/lock.h"

namespace leader {

class NodeCollectionImpl : public NodeCollectionInterface {
 public:
  NodeCollectionImpl() = default;

  ~NodeCollectionImpl() override;

  NodeCollectionImpl(const NodeCollectionImpl&) = delete;

  NodeCollectionImpl& operator=(const NodeCollectionImpl&) = delete;

  void UpdateHeartbeat(const std::vector<std::string>& nodes,
                       const std::string& path) override;

  void UpdateBadNode(std::set<std::string>& blackList) override;

  void FindNodesBySpec(const std::set<std::string>& specLists,
                       std::map<std::string,
                       std::string>& nodes) override;

  bool PickOneNode(std::string& node, const std::string& path) override;

  size_t GetNodeCount(const std::string& path) override;

  /**
   * @brief Remove path and all sub-paths
   * @param path
   */
  void RemovePath(const std::string& path) override;

  /**
   * @brief Remove path and all sub-paths
   * @param path
   */
  void RemovePath(const std::vector<std::string>& path) override;

 private:
  common::ReadWriteLock node_lock_;
  std::mutex update_mutex_;

  std::map<std::string, std::shared_ptr<std::set<std::string>>>
      nodes_iindex_;
  std::set<std::string> black_list_;
  std::map<std::string, std::shared_ptr<std::vector<std::string>>>
      valid_nodes_;

  class RoundRobinIndex {
   public:
    size_t next(size_t base);
   private:
    std::mutex mutex_;
    size_t index_;
  };
  // only implement round robin lb for now
  mutable std::map<std::string, RoundRobinIndex*> round_robin_index_map_;
  common::ReadWriteLock round_robin_map_lock_;
};

}  // namespace leader

#endif  // LEADER_NODE_COLLECTION_IMPL_H_
