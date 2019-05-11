/*!
 * \file node_collection_impl.cc
 * \brief The node collection implementation
 */
#include "leader/node_collection_impl.h"

#include <set>
#include <map>
#include <vector>
#include <string>

#include "leader/heartbeat_message_helper.h"
#include "common/logging.h"

namespace leader {

NodeCollectionImpl::~NodeCollectionImpl() {
  round_robin_map_lock_.wrlock();
  for (const auto& pair : round_robin_index_map_) {
    delete pair.second;
  }
  round_robin_map_lock_.unlock();
}

void NodeCollectionImpl::UpdateHeartbeat(const std::vector<std::string>& nodes,
                                         const std::string& path) {
  auto validNodes = std::make_shared<std::vector<std::string>>();
  std::lock_guard<std::mutex> updateLock(update_mutex_);
  for (const auto& node : nodes) {
    if (black_list_.find(node) == black_list_.end()) {
      validNodes->push_back(node);
    }
  }
  {
    common::ScopedWriteLock writeLock(node_lock_);
    for (const auto& node : *validNodes) {
      auto ii = nodes_iindex_.find(node);
      if (ii == nodes_iindex_.end()) {
        auto s = std::make_shared<std::set<std::string>>();
        s->insert(path);
        nodes_iindex_[node] = s;
      } else {
        ii->second->insert(path);
      }
    }
    valid_nodes_[path] = validNodes;
  }
  {
    common::ScopedWriteLock writeLock(round_robin_map_lock_);
    if (round_robin_index_map_.find(path) == round_robin_index_map_.end()) {
      round_robin_index_map_.insert(std::make_pair(path,
                                                   new RoundRobinIndex()));
    }
  }
}

void NodeCollectionImpl::RemovePath(const std::string& path) {
  std::vector<std::string> removePaths;
  std::lock_guard<std::mutex> updateLock(update_mutex_);
  for (const auto& pair : valid_nodes_) {
    if (strncmp(pair.first.c_str(), path.c_str(), path.size()) == 0
        && (pair.first.size() == path.size()
            || pair.first[path.size()] == '/')) {
      removePaths.push_back(pair.first);
    }
  }
  if (removePaths.empty()) {
    return;
  }
  round_robin_map_lock_.wrlock();
  for (const auto& remove : removePaths) {
    auto iter = round_robin_index_map_.find(remove);
    if (iter == round_robin_index_map_.end()) {
      LOG(WARNING) << remove << "not found in index map";
      continue;
    }
    delete iter->second;
    round_robin_index_map_.erase(iter);
  }
  round_robin_map_lock_.unlock();

  std::vector<std::string> removedNodes;
  common::ScopedWriteLock guard(node_lock_);
  for (const auto& remove : removePaths) {
    auto i = valid_nodes_.find(remove);
    if (i == valid_nodes_.end()) {
      continue;
    }
    for (const auto& node : *(i->second)) {
      auto ii = nodes_iindex_.find(node);
      if (ii == nodes_iindex_.end()) {
        removedNodes.push_back(node);
        continue;
      }
      (ii->second)->erase(remove);
      if ((ii->second)->empty()) {
        removedNodes.push_back(node);
        nodes_iindex_.erase(ii);
      }
    }
    valid_nodes_.erase(i);
  }
  for (const auto& node : removedNodes) {
    black_list_.erase(node);
  }
}

void NodeCollectionImpl::RemovePath(const std::vector<std::string>& paths) {
  std::set<std::string> removePaths;
  std::lock_guard<std::mutex> updateLock(update_mutex_);
  for (const auto& pair : valid_nodes_) {
    for (const auto& path : paths) {
      if (strncmp(pair.first.c_str(), path.c_str(), path.size()) == 0
          && (pair.first.size() == path.size()
              || pair.first[path.size()] == '/')) {
        removePaths.insert(pair.first);
      }
    }
  }
  if (removePaths.empty()) {
    return;
  }
  round_robin_map_lock_.wrlock();
  for (const auto& remove : removePaths) {
    auto iter = round_robin_index_map_.find(remove);
    if (iter == round_robin_index_map_.end()) {
      LOG(WARNING) << remove << "not found in index map";
      continue;
    }
    delete iter->second;
    round_robin_index_map_.erase(iter);
  }
  round_robin_map_lock_.unlock();

  std::vector<std::string> removedNodes;
  common::ScopedWriteLock guard(node_lock_);
  for (const auto& remove : removePaths) {
    auto i = valid_nodes_.find(remove);
    if (i == valid_nodes_.end()) {
      continue;
    }
    for (const auto& node : *(i->second)) {
      auto ii = nodes_iindex_.find(node);
      if (ii == nodes_iindex_.end()) {
        removedNodes.push_back(node);
        continue;
      }
      (ii->second)->erase(remove);
      if ((ii->second)->empty()) {
        removedNodes.push_back(node);
        nodes_iindex_.erase(ii);
      }
    }
    valid_nodes_.erase(i);
  }
  for (const auto& node : removedNodes) {
    black_list_.erase(node);
  }
}

void NodeCollectionImpl::UpdateBadNode(std::set<std::string>& blackList) {
  std::vector<std::string> validNodes;
  std::lock_guard<std::mutex> updateLock(update_mutex_);
  common::ScopedWriteLock writeLock(node_lock_);
  black_list_ = blackList;
  for (const auto& blackNode : blackList) {
    auto ii =
        // find all paths contains the bad node
        nodes_iindex_.find(blackNode);
    if (ii == nodes_iindex_.end()) {
      continue;
    }
    // iterate all paths contains the node
    for (const auto& path : *(ii->second)) {
      auto i = valid_nodes_.find(path);
      if (i == valid_nodes_.end()) {
        continue;
      }
      for (auto vi = (i->second)->begin(); vi != (i->second)->end(); vi++) {
        if (*vi == blackNode) {
          vi = (i->second)->erase(vi);
          // don't have to check all nodes, break after erased
          break;
        }
      }
    }
    nodes_iindex_.erase(ii);
  }
}

void NodeCollectionImpl::FindNodesBySpec(const std::set<std::string>& specLists,
                                         std::map<std::string, std::string>& nodes) {
  common::ScopedReadLock guard(node_lock_);
  for (const auto& msg : nodes_iindex_) {
    std::string spec = msg.first;
    if (specLists.find(spec) != specLists.end()) {
      nodes[spec] = msg.first;
    }
  }
}

size_t NodeCollectionImpl::RoundRobinIndex::next(size_t base) {
  std::lock_guard<std::mutex> guard(mutex_);
  index_ = (index_ + 1) % base;
  return index_;
}

bool NodeCollectionImpl::PickOneNode(std::string& node,
                                     const std::string& path) {
  common::ScopedReadLock rrmGuard(round_robin_map_lock_);
  auto rri = round_robin_index_map_.find(path);
  if (rri == round_robin_index_map_.end()) {
    return false;
  }
  common::ScopedReadLock guard(node_lock_);
  auto iValid = valid_nodes_.find(path);
  if (iValid == valid_nodes_.end() || iValid->second->empty()) {
    return false;
  }
  node = iValid->second->at(rri->second->next(iValid->second->size()));
  return true;
}

size_t NodeCollectionImpl::GetNodeCount(const std::string& path) {
  common::ScopedReadLock guard(node_lock_);
  auto iValid = valid_nodes_.find(path);
  if (iValid == valid_nodes_.end()) {
    return 0;
  }
  return iValid->second->size();
}

}  // namespace leader
