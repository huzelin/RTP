#ifndef ORC_UTIL_LOCAL_CACHE_H_
#define ORC_UTIL_LOCAL_CACHE_H_

#include <list>
#include <unordered_map>
#include <iostream>

namespace orc {

template<typename K, typename V>
struct LocalCacheEntry {
  K key;
  V val;
  uint64_t ts;
};

template<typename K>
struct LocalCacheHash {
  std::size_t operator()(const K* k) const noexcept {
    return std::hash<K>{}(*k);
  }
};

template<typename K>
struct LocalCacheKeyEqual {
  bool operator()(const K* l, const K* r) const {
    return std::equal_to<K>{}(*l, *r);
  }
};

template<typename K, typename V>
class LocalCache {
 public:
  using EntryList = std::list<LocalCacheEntry<K, V>>;
  using EntryIndexKey = const K*;
  using EntryIndexVal = typename EntryList::iterator;
  using EntryIndex = std::unordered_map<EntryIndexKey, EntryIndexVal,
                                        LocalCacheHash<K>, LocalCacheKeyEqual<K>,
                                        std::allocator<std::pair<EntryIndexKey, EntryIndexVal>>
                                        >;

  LocalCache(size_t capacity, uint64_t expire)
      : capacity_(capacity), size_(0), expire_(expire) {}
  ~LocalCache() = default;

  size_t capacity() const { return capacity_; }
  size_t expire() const { return expire_; }
  size_t size() const { return size_; }

  bool Get(const K& k, V* v, uint64_t now);
  bool Put(const K& k, const V& v, uint64_t now);

 private:
  EntryList entry_list_;
  EntryIndex entry_index_;

  EntryList free_entry_;

  size_t capacity_;
  size_t size_;
  uint64_t expire_;
};

template<typename K, typename V>
void DumpList(const std::list<LocalCacheEntry<K, V>>& l, const char* file, int line) {
  std::cout << "List: " << &l;
  std::cout << ", file" << file;
  std::cout << ":" << line << std::endl;

  for (auto& entry : l) {
    std::cout << "key: " << entry.key;
    std::cout << ", val: " << entry.val;
    std::cout << ", ts: " << entry.ts << std::endl;
  }
}

// #define DUMP_LIST(l) DumpList(l, __FILE__, __LINE__)
#define DUMP_LIST(l)

template<typename K, typename V>
bool LocalCache<K, V>::Get(const K& k, V* v, uint64_t now) {
  DUMP_LIST(entry_list_);
  auto it = entry_index_.find(&k);
  if (it != entry_index_.end()) {
    auto index = it->second;
    if ((now - index->ts) < expire_) {
      *v = index->val;
      entry_list_.splice(entry_list_.begin(), entry_list_, index);
      DUMP_LIST(entry_list_);
      return true;
    } else {
      entry_index_.erase(&index->key);
      free_entry_.splice(free_entry_.begin(), entry_list_, index);
      --size_;
      DUMP_LIST(entry_list_);
      DUMP_LIST(free_entry_);
    }
  }
  return false;
}

template<typename K, typename V>
bool LocalCache<K, V>::Put(const K& k, const V& v, uint64_t now) {
  auto it = entry_index_.find(&k);
  if (it == entry_index_.end()) {
    while (size_ >= capacity_) {
      entry_index_.erase(&entry_list_.back().key);
      free_entry_.splice(free_entry_.begin(), entry_list_, --(entry_list_.end()));
      --size_;
    }

    DUMP_LIST(entry_list_);
    DUMP_LIST(free_entry_);

    if (free_entry_.empty()) {
      entry_list_.emplace_front(LocalCacheEntry<K, V>{k, v, now});
    } else {
      free_entry_.front().key = k;
      free_entry_.front().val = v;
      free_entry_.front().ts = now;
      entry_list_.splice(entry_list_.begin(), free_entry_, free_entry_.begin());
    }

    DUMP_LIST(entry_list_);
    DUMP_LIST(free_entry_);

    ++size_;
    entry_index_[&entry_list_.front().key] = entry_list_.begin();
  } else {
    auto index = it->second;
    index->val = v;
    index->ts = now;
    entry_list_.splice(entry_list_.begin(), entry_list_, index);

    DUMP_LIST(entry_list_);
    DUMP_LIST(free_entry_);
  }
  return true;
}

}  // namespace orc

#endif  // ORC_UTIL_LOCAL_CACHE_H_
