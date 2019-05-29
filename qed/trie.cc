/*
 * \file trie.cc
 * \brief The trie implementation
 *
 */
#include "qed/trie.h"

namespace qed {

bool insert_to_fat_trie(FatTrieNode* root,
                        const std::string& key,
                        FatTrieNode::ValueType value,
                        std::function<FatTrieNode*()>&& allocator) {
  FatTrieNode* current = root;
  size_t offset = 0;
  for (; offset < key.size() && current->subnode_count < 255; offset++) {
    int insertion_place = 0;
    auto keyc = key[offset];
    if (current->subnode_count == 0) {
      goto INSERT_NODE;
    }
    if (current->fat_subnode_name[0] < keyc) {
      current->fat_subnode_name[current->subnode_count] =
          current->fat_subnode_name[0];
      current->fat_subnode[current->subnode_count].ptr =
          current->fat_subnode[0].ptr;
      goto INSERT_NODE;
    }
    for (; insertion_place < current->subnode_count; insertion_place++) {
      if (keyc == current->fat_subnode_name[insertion_place]) {
        current = current->fat_subnode[insertion_place].ptr;
        goto CONTINUE;
      } else if (keyc < current->fat_subnode_name[insertion_place]) {
        memmove(current->fat_subnode + insertion_place + 1,
                current->fat_subnode + insertion_place,
                (current->subnode_count - insertion_place) * sizeof(uint64_t));
        memmove(current->fat_subnode_name + insertion_place + 1,
                current->fat_subnode_name + insertion_place,
                (current->subnode_count - insertion_place) * sizeof(char));
        break;
      }
    }
    INSERT_NODE:
    current->fat_subnode_name[insertion_place] = keyc;
    current->fat_subnode[insertion_place].ptr = allocator();
    current->subnode_count++;
    current = current->fat_subnode[insertion_place].ptr;
    CONTINUE:;
  }
  if (offset != key.size()) {
    return false;
  }
  current->value = value;
  return true;
}

}  // namespace qed
