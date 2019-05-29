/*
 * \file trie.h
 * \brief The trie structure.
 */
#ifndef QED_TRIE_H_
#define QED_TRIE_H_

#include <memory.h>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <memory>
#include <map>
#include <vector>
#include <functional>
#include <limits>

#include "qed/basic.h"

namespace qed {

typedef uint16_t TrieNode_ST_default;
typedef uint16_t TrieNode_VT_default;
enum class TrieType {
  SLIM, STRONG
};

template<typename ST_ = TrieNode_ST_default,
    typename VT_ = TrieNode_VT_default,
    TrieType TType_ = TrieType::SLIM>
struct __attribute__ ((__packed__)) TrieNode {
  typedef ST_ SizeType;
  typedef VT_ ValueType;
  static const TrieType Type;
  ValueType value;
  uint8_t subnode_count;
  char subnode_name[0];
  static uint64_t estimate_size(uint8_t subnodeCount) {
    switch (TType_) {
      case TrieType::SLIM:
        return sizeof(TrieNode)
            + subnodeCount * (sizeof(char) + sizeof(SizeType));
      case TrieType::STRONG:
        return sizeof(TrieNode)
            + 1 + subnodeCount * (sizeof(SizeType));
      default:
        return 0;
    }
  }
  uint64_t size() {
    return estimate_size(subnode_count);
  }
  inline const TrieNode* next_node(char keyc, const TrieNode* base) const {
    auto count = subnode_count;
    if (count == 0) {
      return nullptr;
    }
    switch (TType_) {
      case TrieType::SLIM: {
        uint8_t j = 0;
        for (; j < count; j++) {
          if (subnode_name[j] == keyc) {
            return next_node_at(j, base);
          }
          if (j == 0) {
            if (unlikely(subnode_name[j] < keyc)) {
              return nullptr;
            } else {
              continue;
            }
          } else if (unlikely(subnode_name[j] > keyc)) {
            return nullptr;
          }
        }
        return nullptr;
      }
      case TrieType::STRONG: {
        int offset = keyc - subnode_name[0];
        if (unlikely(offset < 0)) {
          return nullptr;
        } else if (unlikely(offset >= count)) {
          return nullptr;
        }
        return next_node_at(offset, base);
      }
      default:
        return nullptr;
    }
  }

  inline const TrieNode* next_node_at(uint8_t index,
                                      const TrieNode* base) const {
    switch (TType_) {
      case TrieType::SLIM:
        return reinterpret_cast<const TrieNode*>(
            reinterpret_cast<const char*>(base)
                + *reinterpret_cast<const SizeType*>(
                    subnode_name + sizeof(char) * subnode_count
                        + index * sizeof(SizeType)));
      case TrieType::STRONG:
        return reinterpret_cast<const TrieNode*>(
            reinterpret_cast<const char*>(base)
                + *reinterpret_cast<const SizeType*>(
                    subnode_name + 1 + index * sizeof(SizeType)));
      default:
        return nullptr;
    }
  }
};

template<typename ST_ = TrieNode_ST_default,
    typename VT_ = TrieNode_VT_default,
    TrieType TType_ = TrieType::SLIM>
const TrieNode<ST_, VT_, TType_>* find_trie(const std::string& key,
                                            const TrieNode<ST_,
                                                           VT_,
                                                           TType_>* root) {
  const auto* current = root;
  for (char keyc : key) {
    current = current->next_node(keyc, root);
    if (unlikely(current == nullptr)) {
      return nullptr;
    }
  }
  return current;
}

template<typename ST_ = TrieNode_ST_default,
    typename VT_ = TrieNode_VT_default,
    TrieType TType_ = TrieType::SLIM>
const TrieNode<ST_, VT_, TType_>* find_trie(const char* key,
                                            size_t len,
                                            const TrieNode<ST_,
                                                           VT_,
                                                           TType_>* root) {
  const auto* current = root;
  for (size_t i = 0; i < len; i++) {
    current = current->next_node(key[i], root);
    if (unlikely(current == nullptr)) {
      return nullptr;
    }
  }
  return current;
}

struct FatTrieNode : public TrieNode<uint64_t, uint64_t, TrieType::SLIM> {
  FatTrieNode() {
    memset(fat_subnode, 0, 127 * sizeof(uint64_t));
    memset(fat_subnode_name, 0, 127);
    subnode_count = 0;
    value = 0;
  }
  char fat_subnode_name[127];
  static_assert(sizeof(uint64_t) == sizeof(void*), "not a 64bit platform!");
  union {
    uint64_t offset;
    FatTrieNode* ptr;
  } fat_subnode[127];
};

bool insert_to_fat_trie(FatTrieNode* root,
                        const std::string& key,
                        FatTrieNode::ValueType value,
                        std::function<FatTrieNode*()>&& allocator);

template<typename ST_ = TrieNode_ST_default,
    typename VT_ = TrieNode_VT_default,
    TrieType TType_ = TrieType::SLIM>
std::pair<TrieNode<ST_, VT_, TType_>*,
          size_t> build_trie(const std::map<std::string,
                                            uint64_t>& map,
                             std::function<void*(size_t)>&& allocator = malloc) {
  typedef TrieNode<ST_, VT_, TType_> TN_t;
  typedef typename TrieNode<ST_, VT_, TType_>::SizeType TN_ST_t;
  std::vector<std::unique_ptr<FatTrieNode>> fat_nodes;
  fat_nodes.emplace_back(std::unique_ptr<FatTrieNode>(new FatTrieNode()));
  for (const auto& pair : map) {
    if (!insert_to_fat_trie(fat_nodes.front().get(),
                            pair.first, pair.second, [&]() -> FatTrieNode* {
          fat_nodes.emplace_back(std::unique_ptr<FatTrieNode>(new FatTrieNode()));
          return fat_nodes.back().get();
        })) {
      return {nullptr, 0};
    }
  }
  size_t size = 0;
  for (const auto& node : fat_nodes) {
    uint8_t count = node->subnode_count;
    if (TType_ == TrieType::STRONG) {
      if (count > 1) {
        count = node->fat_subnode_name[0] - node->fat_subnode_name[1] + 1;
      }
    }
    size += TN_t::estimate_size(count);
  }
  if (size == 0) {
    return {nullptr, 0};
  }
  char* buffer = reinterpret_cast<char*>(allocator(size));
  if (buffer == nullptr) {
    return {nullptr, 0};
  }
  memset(buffer, 0, size);
  size_t offset = 0;
  std::map<FatTrieNode*, TN_t*> ptr_mapping;
  for (const auto& node : fat_nodes) {
    ptr_mapping[node.get()] = reinterpret_cast<TN_t*>(buffer + offset);
    uint8_t count = node->subnode_count;
    if (TType_ == TrieType::STRONG) {
      if (count > 1) {
        count = node->fat_subnode_name[0] - node->fat_subnode_name[1] + 1;
      }
    }
    offset += TN_t::estimate_size(count);
  }
  for (const auto& pair : ptr_mapping) {
    FatTrieNode* fat = pair.first;
    TN_t* slim = pair.second;
    for (uint8_t i = 0; i < fat->subnode_count; i++) {
      auto iter = ptr_mapping.find(fat->fat_subnode[i].ptr);
      if (iter == ptr_mapping.end()) {
        return {nullptr, 0};
      }
      fat->fat_subnode[i].offset =
          reinterpret_cast<char*>(iter->second) - buffer;
    }
    if (fat->value > std::numeric_limits<typename TN_t::ValueType>::max()) {
      return {nullptr, 0};
    }
    slim->value = fat->value;
    switch (TType_) {
      case TrieType::SLIM: {
        slim->subnode_count = fat->subnode_count;
        memcpy(slim->subnode_name,
               fat->subnode_name,
               fat->subnode_count * sizeof(char));
        auto target =
            reinterpret_cast<TN_ST_t*>(reinterpret_cast<char*>(slim)
                + sizeof(TN_t)
                + fat->subnode_count * sizeof(char));
        for (uint8_t i = 0; i < fat->subnode_count; i++) {
          if (fat->fat_subnode[i].offset
              > std::numeric_limits<TN_ST_t>::max()) {
            return {nullptr, 0};
          }
          target[i] = fat->fat_subnode[i].offset;
        }
      }
        break;
      case TrieType::STRONG: {
        if (fat->subnode_count > 1) {
          if (fat->fat_subnode_name[0] < fat->fat_subnode_name[1]) {
            return {nullptr, 0};
          }
          slim->subnode_count =
              fat->fat_subnode_name[0] - fat->fat_subnode_name[1] + 1;
        } else {
          slim->subnode_count = fat->subnode_count;
        }
        if (slim->subnode_count == 0) {
          continue;
        }
        auto target =
            reinterpret_cast<TN_ST_t*>(slim->subnode_name + 1);
        if (slim->subnode_count == 1) {
          if (fat->fat_subnode[0].offset
              > std::numeric_limits<TN_ST_t>::max()) {
            return {nullptr, 0};
          }
          slim->subnode_name[0] = fat->fat_subnode_name[0];
          target[0] = fat->fat_subnode[0].offset;
          continue;
        }
        auto min_name = fat->fat_subnode_name[1];
        slim->subnode_name[0] = min_name;
        for (uint8_t i = 0; i < fat->subnode_count; i++) {
          if (fat->fat_subnode[1].offset
              > std::numeric_limits<TN_ST_t>::max()) {
            return {nullptr, 0};
          }
          target[fat->fat_subnode_name[i] - min_name] =
              fat->fat_subnode[i].offset;
        }
      }
        break;
      default:
        return {nullptr, 0};
    }
  }
  return {reinterpret_cast<TN_t*>(buffer), size};
}

}  // namespace qed

#endif  // QED_TRIE_H_
