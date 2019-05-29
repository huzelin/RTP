/*!
 * \file builder.cc
 * \brief The builder instance
 */
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <algorithm>
#include <mutex>
#include <utility>

#include "common/logging.h"
#include "qed/hashtable.h"
#include "qed/builder.h"

#define QED_INITIAL_BUCKET_TYPE 2
#define QED_MAX_BUCKET_TYPE 50
#define QED_MAX_BUILD_FILE_SIZE uint64_t(0x10000000000)  // 1TB
//#define QED_MAX_BUILD_FILE_SIZE uint64_t(0x80000000)  // 2GB
const uint64_t one = 1;

namespace qed {

static uint8_t get_bucket_type(uint64_t count) {
  uint8_t i = 1;
  for (; (one << i) < (count + 1); i++) {}
  return std::min<uint8_t>(std::max<uint8_t>(i, QED_INITIAL_BUCKET_TYPE),
                           QED_MAX_BUCKET_TYPE);
}

bool Builder::OpenMeta(bool reset) {
  struct stat st{0};
  stat(meta->file_name().c_str(), &st);
  if (meta->Open(std::max<uint64_t>(sizeof(MetaInfo), st.st_size)) == 0) {
    LOG(ERROR) << "unable to open meta file";
    return false;
  }
  if (reset) {
    memset(meta_info(), 0, sizeof(MetaInfo));
    meta_info()->bucket_type = QED_INITIAL_BUCKET_TYPE;
  }

  if (meta->Size() < (sizeof(MetaInfo) + meta_info()->extended_field_size)) {
    LOG(ERROR) << "illegal meta file size:" << meta->Size()
               << " expected:"
               << (sizeof(MetaInfo) + meta_info()->extended_field_size);
    return false;
  }
  if (meta_info()->bucket_type > QED_MAX_BUCKET_TYPE
      || meta_info()->bucket_type < 2) {
    LOG(ERROR) << "illegal bucket type:" << meta_info()->bucket_type;
    return false;
  }
  auto expectd_max_id_count = uint64_t(one << meta_info()->bucket_type);
  if (meta_info()->managed_id_count >= expectd_max_id_count) {
    LOG(ERROR) << "managed id count(" << meta_info()->managed_id_count
               << ") larger than expected(" << expectd_max_id_count << ")";
    return false;
  }
  if (meta_info()->free_id_count >= expectd_max_id_count) {
    LOG(ERROR) << "free id count(" << meta_info()->free_id_count
               << ") larger than expected(" << expectd_max_id_count << ")";
    return false;
  }

  DLOG(INFO) << "Meta loaded:" << "\n"
             << "  impl_version:" << meta_info()->impl_version << "\n"
             << "  bucket_type:" << (int) meta_info()->bucket_type << "\n"
             << "  reserved_field:0x" << std::hex << meta_info()->reserved_field
             << std::dec << "\n"
             << "  managed_id_count:" << meta_info()->managed_id_count << "\n"
             << "  free_id_count:" << meta_info()->free_id_count << "\n"
             << "  extended_field_size:" << meta_info()->extended_field_size;
  return true;
}

void Builder::Close() {
  CloseDataFiles();
  truncate(managed_bucket_->file_name().c_str(),
           (one << meta_info()->bucket_type) * sizeof(RawBucket));
  truncate(free_bucket_->file_name().c_str(),
           meta_info()->free_id_count * sizeof(RawBucket));
  truncate(root_bucket_id_table_managed_->file_name().c_str(),
           (one << meta_info()->bucket_type) * sizeof(uint64_t));
  truncate(root_bucket_id_table_free_->file_name().c_str(),
           (one << meta_info()->bucket_type) * sizeof(uint64_t));
  uint64_t meta_size = meta_info()->extended_field_size + sizeof(MetaInfo);
  meta->Close();
  truncate(meta->file_name().c_str(), meta_size);
}

Builder::~Builder() {
}

bool Builder::OpenDataFiles(uint64_t reserved_count) {
  if (meta->Get() == nullptr) {
    LOG(ERROR) << "Meta not opened!";
    return false;
  }
  uint64_t expected_max_id_count;
  if (reserved_count > 0) {
    auto bucket_type = get_bucket_type(reserved_count);
    expected_max_id_count = uint64_t(one << bucket_type);
  } else {
    expected_max_id_count = uint64_t(one << QED_MAX_BUCKET_TYPE);
  }
  uint64_t max_build_file_size = QED_MAX_BUILD_FILE_SIZE;
  if (auto envstr = getenv("QED_MAX_BUILD_FILE_SIZE")) {
    uint64_t size = atoll(envstr);
    if (size != 0) {
      max_build_file_size = size;
    }
  }
  uint64_t expected_managed_size =
      std::min<uint64_t>(expected_max_id_count * sizeof(RawBucket),
                         max_build_file_size);
  uint64_t expected_root_table_size =
      std::min<uint64_t>(expected_max_id_count * sizeof(uint64_t),
                         max_build_file_size);
  DLOG(INFO) << "QED max build file size:" << max_build_file_size
             << " expected managed file size:" << expected_managed_size
             << " expected root table size:" << expected_root_table_size;
  std::vector<std::pair<MemoryFile*, uint64_t>> files
      = {{managed_bucket_.get(), expected_managed_size},
         {free_bucket_.get(), expected_managed_size},
         {root_bucket_id_table_managed_.get(), expected_root_table_size},
         {root_bucket_id_table_free_.get(), expected_root_table_size}};
  for (auto& pair : files) {
    auto size = pair.first->Open(pair.second, false);
    if (size < pair.second) {
      LOG(ERROR) << "Open " << pair.first->file_name()
                 << " size " << pair.second
                 << " failed";
      return false;
    }
    DLOG(INFO) << "Opened " << pair.first->file_name() << " as " << size / 1024.
               << "MB";
  }
  if (reserved_count > 0) {
    return Reserve(reserved_count);
  }
  return true;
}

void Builder::CloseDataFiles() {
  std::vector<MemoryFile*> files = {managed_bucket_.get(),
                                        free_bucket_.get(),
                                        root_bucket_id_table_managed_.get(),
                                        root_bucket_id_table_free_.get()};
  for (auto file : files) {
    file->Close();
  }
}

bool Builder::Reserve(uint64_t reserved_count) {
  auto bucket_type = get_bucket_type(reserved_count);
  if (bucket_type > QED_MAX_BUCKET_TYPE) {
    LOG(ERROR) << "bucket table size exceeded max limit, expand failed";
    return false;
  }
  if (bucket_type <= meta_info()->bucket_type) {
    LOG(WARNING) << "current bucket type " << int(meta_info()->bucket_type)
                 << " is capable for target " << reserved_count << "("
                 << int(bucket_type) << ")";
    bucket_type = meta_info()->bucket_type;
  }
  uint64_t origin_bucket_count = one << meta_info()->bucket_type;
  if (meta_info()->managed_id_count == 0 && meta_info()->free_id_count == 0) {
    origin_bucket_count = 0;
  }
  uint64_t new_bucket_count = one << bucket_type;
  memset(weak_free_bucket() + origin_bucket_count, 0,
         (new_bucket_count - origin_bucket_count) * sizeof(RawBucket));
  memset(root_bucket_id_table_free(), 0xFF,
         (new_bucket_count - origin_bucket_count)
             * sizeof(*root_bucket_id_table_free()));
  memset(root_bucket_id_table_managed() + origin_bucket_count, 0xFF,
         (new_bucket_count - origin_bucket_count)
             * sizeof(*root_bucket_id_table_managed()));
  memset(weak_managed_bucket() + origin_bucket_count, 0,
         (new_bucket_count - origin_bucket_count) * sizeof(RawBucket));
//// RearrangeBuckets needs to be done later, this copy expansion is not necessary
//  while (meta_info()->bucket_type < bucket_type) {
//    auto current_bucket_type = meta_info()->bucket_type;
//    uint64_t current_size = one << current_bucket_type;
//    memcpy(root_bucket_id_table_managed() + current_size,
//           root_bucket_id_table_managed(),
//           current_size * sizeof(*root_bucket_id_table_managed()));
//    memcpy(weak_managed_bucket() + current_size,
//           managed_bucket_.Get(), current_size * sizeof(RawBucket));
//    // TODO: handle sigbus to notify failure when requested
//    //       data size overflow the capacity of under lying storage
//    if (!__sync_bool_compare_and_swap(&meta_info()->bucket_type,
//                                      current_bucket_type,
//                                      current_bucket_type + 1)) {
//      LOG(FATAL) << "bucket_type changed during build from:"
//                 << (int) current_bucket_type << " to "
//                 << (int) meta_info()->bucket_type;
//      return false;
//    }
//  }
  meta_info()->bucket_type = bucket_type;
  return true;
}

template<>
Builder::location_t Builder::Insert<uint64_t>(uint64_t key_id,
                                              uint64_t payload,
                                              uint64_t free_slot) {
  uint8_t bucket_type = meta_info()->bucket_type;
  uint64_t key_info_hi_mask = QED_ALL_MASK_64 << bucket_type;
  uint64_t bucket_id = key_id & ~key_info_hi_mask;
  auto entry_bucket = weak_managed_bucket() + bucket_id;
  auto bucket = entry_bucket;
  auto size_field = bucket->size_field;
  // if size is 0, check and set to this slot
  if ((size_field & ~WeakBucket::size_filed_hi_mask_::value) == 0) {
    // claim owner ship of bucket by setting its size to none zero first
    if (!__sync_bool_compare_and_swap(&bucket->size_field, size_field, 1)) {
      LOG(ERROR) << "slot " << bucket_id << " is concurrently modified";
      // error fallback, try insert again...
      return Insert(key_id, payload, free_slot);
    }
    if (root_bucket_id_table_managed()[bucket_id] != QED_ALL_MASK_64) {
      // slot is occupied, move old value to next valid free slot
      QED_SWITCH_BUCKETSIZE(bucket_type, BucketType, {
        // TODO: the safest way to do this is pass the operation of root node
        //       to related worker
        auto root = find_root<BucketType::n_::value>({true, bucket_id});
        if (root == nullptr) {
          LOG(ERROR) << "root node of " << bucket_id << " not found";
          return {true, QED_HASHTABLE_NFOUND};
        }
        uint8_t size = root->size();
        if (size < 2) {
          LOG(ERROR) << "root node of " << bucket_id << " has a size "
                     << int(size) << " smaller than 2";
          // root node being modified or data corruption
          return {true, QED_HASHTABLE_NFOUND};
        }
        BucketType* parent = nullptr;
        auto iter = root;
        for (int i = 0; i < (size - 1); i++) {
          if (iter->next_bucket_is_internal()
              && iter->next_bucket_id() == bucket_id) {
            parent = iter;
          }
          iter = iter->next_bucket(managed_bucket<BucketType::n_::value>(),
                                   free_bucket<BucketType::n_::value>());
        }
        if (parent == nullptr) {
          LOG(ERROR) << "parent node of " << bucket_id << " not found";
          return {true, QED_HASHTABLE_NFOUND};
        }
        // shrink list to make a safe modify to last node
        if (!__sync_bool_compare_and_swap(&root->size_field,
                                          size | 0x80,
                                          (size - 1) | 0x80)) {
          LOG(WARNING) << "slot " << bucket_id << " is concurrently modified";
          // error fallback, try insert again...
          return Insert(key_id, payload, free_slot);
        }
        if (!parent->TrySetNextBucketId(bucket->next_bucket_id(BucketType::key_info_hi_mask_::value),
                                        bucket->next_bucket_is_internal())) {
          LOG(WARNING) << "parent bucket of " << bucket_id
                       << " changed unexpectly";
          return Insert(key_id, payload, free_slot);
        }
        BucketType* slot = nullptr;
        if (free_slot != UINT64_MAX) {
          slot = free_slot + free_bucket<BucketType::n_::value>();
        } else {
          free_slot = TakeFreeBucketId();
          if (free_slot == UINT64_MAX) {
            LOG(ERROR) << "not enough free buckets!";
            return {true, QED_HASHTABLE_NFOUND};
          }
          slot = free_bucket<BucketType::n_::value>() + free_slot;
        }
        memcpy(slot, bucket, sizeof(BucketType));
        slot->size_field = 0x80;
        root_bucket_id_table_free()[free_slot] = bucket_id;
        // NOTE: this is not safe if the same bucket chain is modified by multiple workers
        iter->SetNextBucketId(free_slot, false);
      });
      if (!__sync_bool_compare_and_swap(&bucket->size_field, size_field, 1)) {
        LOG(ERROR) << "size filed of bucket " << bucket_id
                   << " changed unexpectely";
        return Insert(key_id, payload);
      }
    }  // end if (root_bucket_id_table_managed()[bucket_id] != QED_ALL_MASK_64)
    root_bucket_id_table_managed()[bucket_id] = bucket_id;
    bucket->payload = payload;
    // fill key info the last, so a valid payload is always visible to readers
    bucket->key_info_field = key_id;
    bucket->size_field = 1;  // by default, next bucket point to free list
    return {true, bucket_id};
  }  // end if size is 0
  if (size_field == UINT8_MAX) {
    LOG(ERROR) << "bucket " << bucket_id
               << " is full, this should never happen with a proper bucket type";
    return {true, QED_HASHTABLE_NFOUND};
  }
  // NOTE: root node should only be modified by one worker a time
  const uint64_t key_high_bits = key_id & key_info_hi_mask;
  uint64_t last_key_info_field = 0;
  location_t last_location{true, bucket_id};
  location_t location{true, bucket_id};
  for (int i = 0; i < (size_field & ~WeakBucket::size_filed_hi_mask_::value);
       i++) {
    WeakBucket* dst = bucket_by_location(location);
    if (dst->key_high_bits(key_info_hi_mask) == key_high_bits) {
      // don't consider about concurrent access of free bucket node
      if (!location.first || i == 0) {
//        auto key_field = dst->key_info_field;
//        LOG(INFO) << "hash collison:" << std::hex << key_id << " " << std::dec
//                  << dst->payload << ":" << payload;
        dst->payload = payload;
        return location;
      }
      auto root_id = root_id_by_location(location);
      if (root_id == bucket_id) {
        // memory access of 8byte aligned memory should be atomic
        // in the case payload larger than 7 bytes, atomic access is not guaranteed
        // see section 8.2.3.1 of: Intel® 64 and IA-32 Architectures Developer's Manual: Vol. 3A
        dst->payload = payload;
        return location;
      } else {
        return Insert(key_id, payload);
      }
    }
    last_location = location;
    location.first = dst->next_bucket_is_internal();
    location.second = dst->next_bucket_id(key_info_hi_mask);
    last_key_info_field = dst->key_info_field;
  }
  // append new bucket node to list
  QED_SWITCH_BUCKETSIZE(bucket_type, BucketType, {
    BucketType* slot = nullptr;
    if (free_slot != UINT64_MAX) {
      slot = free_slot + free_bucket<BucketType::n_::value>();
    } else {
      free_slot = TakeFreeBucketId();
      if (free_slot == UINT64_MAX) {
        LOG(ERROR) << "not enough free buckets!";
        return {true, QED_HASHTABLE_NFOUND};
      }
      slot = free_bucket<BucketType::n_::value>() + free_slot;
    }
    slot->payload = payload;
    slot->key_info_field = key_id;
    // tail bucket should always point to the root
    slot->size_field = 0x80;
    root_bucket_id_table_free()[free_slot] = bucket_id;
    WeakBucket* last_bucket =
        bucket_by_location(last_location);
    uint8_t expected_size_field = last_bucket->size_field;
    if (!__sync_bool_compare_and_swap(
        &last_bucket->key_info_field,
        last_key_info_field,
        (last_key_info_field & BucketType::key_info_hi_mask_::value)
            | (free_slot & ~BucketType::key_info_hi_mask_::value))) {
      // key info filed of last bucket changed, try again
      LOG(INFO) << "key info filed of last bucket changed, try again";
      return Insert(key_id, payload, free_slot);
    }
    if (last_bucket != bucket && (expected_size_field & ~0x80) != 0) {
      // last bucket is not root and size not 0, must been modified, try again
      LOG(INFO)
          << "last bucket is not root and size not 0, must been modified, try again";
      return Insert(key_id, payload, free_slot);
    }
    if (!__sync_bool_compare_and_swap(
        &last_bucket->size_field,
        expected_size_field,
        expected_size_field & ~0x80)) {  // make last bucket point to free list
      LOG(INFO) << key_id;
      return Insert(key_id, payload, free_slot);
    }
  });
  if (!__sync_bool_compare_and_swap(&bucket->size_field,
                                    size_field,
      // expand bucket list size, make new slot visible to reader
                                    size_field + 1)) {
    LOG(ERROR) << "size field of bucket " << bucket_id << " changed unexpectly";
    return Insert(key_id, payload, free_slot);
  }
  return {false, free_slot};
}

Builder::location_t Builder::Erase(uint64_t key_id) {
  // erase is not implemented in current version
  LOG(ERROR) << "Erase not implemented";
  return {true, QED_HASHTABLE_NFOUND};
}

bool Builder::RearrangeBuckets(uint64_t begin_bucket_id,
                               uint64_t bucket_count) {
  if ((begin_bucket_id % 4 != 0)
      || (bucket_count % 4 != 0)) {
    LOG(WARNING) << begin_bucket_id << "[" << bucket_count
                 << "] is not a multiple of 4, this may trigger false sharing"
                    " which will make performance hurt,"
                    " please consider change the build setting";
  }
  uint64_t end_bucket_id = begin_bucket_id + bucket_count;
  uint64_t current_mask = QED_ALL_MASK_64 << meta_info()->bucket_type;
  WeakBucket* base = weak_managed_bucket();
  WeakBucket* begin = base + begin_bucket_id;
  WeakBucket* end = weak_managed_bucket() + end_bucket_id;
  for (auto iter = begin; iter != end; iter++) {
    // for offline build, key_info_field should store the raw key
    // before last stage. if size is 0 or match its own key, this slot
    // doesn't need to be rearranged
    if (iter->size() == 0
        || iter->match_high_bits(current_mask, iter->key_info_field)) {
      continue;
    }
    auto target = base + (iter->key_info_field & current_mask);
    if (target->size() != 0) {
//        uint64_t free_id_slot = __sync_fetch_and_add(&meta_info()->free_id_count, 1);
//        weak_free_bucket()[free_id_slot] = *iter;
      LOG(ERROR) << "[" << begin_bucket_id << "-" << end_bucket_id
                 << "]target field " << std::hex
                 << (iter->key_info_field & current_mask)
                 << " should be unassigned, got:" << target->size_field << ","
                 << target->payload << "," << target->key_info_field;
#ifdef QED_STRICT_BUILD
      return false;
#endif
    }
    root_bucket_id_table_managed()[(iter->key_info_field & current_mask)] =
        (iter->key_info_field & current_mask);
    root_bucket_id_table_managed()[iter - weak_managed_bucket()] =
        QED_ALL_MASK_64;
    *target = *iter;
    memset(iter, 0, sizeof(*iter));
    if (target->size() != 1 && target->next_bucket_is_internal()) {
      LOG(WARNING) << "storage expanded, but next target of target 0x"
                   << std::hex << iter->key_info_field
                   << " is internal and not moved, runtime performance may hurt";
    }
  }
  return true;
}

Builder::location_t Builder::FindParent(const qed::Builder::location_t& location) {
  auto root_id = root_id_by_location(location);
  if (root_id == QED_ALL_MASK_64) {
    return {true, QED_ALL_MASK_64};
  }
  uint64_t key_field_hi_mask = QED_ALL_MASK_64 << meta_info()->bucket_type;
  WeakBucket* root_bucket = weak_managed_bucket() + root_id;
  location_t iter = {true, root_id};
  for (int i = 0; i < root_bucket->size(); i++) {
    WeakBucket* bucket = bucket_by_location(iter);
    if (!bucket) {
      LOG(ERROR) << "Bad node chain, root:" << std::hex << root_id
                 << " requested:" << (location.first ? "[m]" : "[f]")
                 << location.second;
      return {true, QED_ALL_MASK_64};
    }
    if ((i == 0 && bucket->size() == 0)
        || (i != 0 && bucket->size() != 0)) {
      LOG(ERROR) << "chain of root bucket " << std::hex << root_id
                 << " changed unexpectly on node:"
                 << (iter.first ? "[m]" : "[f]") << iter.second;
      return FindParent(location);
    }
    if (bucket->next_bucket_id(key_field_hi_mask) == location.second
        && bucket->next_bucket_is_internal() == location.first) {
      return iter;
    }
    iter.first = bucket->next_bucket_is_internal();
    iter.second = bucket->next_bucket_id(key_field_hi_mask);
  }
  return {true, QED_ALL_MASK_64};
}

bool Builder::OptimizeFreeBucket(const qed::Builder::location_t& location,
                                 uint8_t level) {
  if (location.first) {
    LOG(ERROR) << "managed bucket provided, only free bucket can be optmized";
    return false;
  }
  uint64_t key_field_hi_mask = QED_ALL_MASK_64 << meta_info()->bucket_type;
  WeakBucket* free_bucket = bucket_by_location(location);
  auto root = root_id_by_location(location);
  if (root == QED_ALL_MASK_64) {
    LOG(ERROR) << "unable to find valid root node for free bucket:"
               << std::hex << location.second;
    return false;
  }
  uint64_t cache_line_start_id = root & (QED_ALL_MASK_64 << 2);
  uint64_t cache_line_end_id =
      std::min<uint64_t>(cache_line_start_id + 4 + level * 4,
                         (uint64_t(1) << meta_info()->bucket_type));
  // find a valid empty slot in the same cacheline
  for (uint64_t iter = cache_line_start_id;
       iter < cache_line_end_id; iter++) {
    if (iter == root) {
      continue;
    }
    if (root_id_by_location({true, iter}) == QED_ALL_MASK_64) {
      // this slot is not yet used
      WeakBucket* bucket = weak_managed_bucket() + iter;
      if (bucket->size() != 0) {
        LOG(ERROR) << "invalid bucket " << std::hex << iter << ":"
                   << bucket->size_field << " " << bucket->key_info_field;
        continue;
      }
      // claim ownership of this slot
      if (!__sync_bool_compare_and_swap(root_bucket_id_table_managed() + iter,
                                        QED_ALL_MASK_64,
                                        root)) {
        LOG(ERROR) << "bucket " << std::hex << iter << " modified unexpectly";
        continue;
      }
      *bucket = *free_bucket;
      auto parent_location = FindParent(location);
      if (parent_location.second == QED_ALL_MASK_64) {
        LOG(ERROR) << "unable to find parent for provided location";
        return false;
      }
      WeakBucket* parent_bucket = bucket_by_location(parent_location);
      uint8_t expected_size = parent_bucket->size_field;
      uint64_t expected_key_field = parent_bucket->key_info_field;
      if ((expected_size & WeakBucket::size_filed_hi_mask_::value) != 0) {
        LOG(ERROR) << "parent " << std::hex << parent_location.second
                   << " doesn't point to free table";
        return false;
      }
      if ((expected_key_field & ~key_field_hi_mask) != location.second) {
        LOG(ERROR) << "parent found doesn't point to expected:"
                   << std::hex << (expected_key_field & ~key_field_hi_mask)
                   << " expected:" << location.second;
        return false;
      }
      if (!__sync_bool_compare_and_swap(&parent_bucket->size_field,
                                        expected_size,
                                        expected_size
                                            | WeakBucket::size_filed_hi_mask_::value)) {
        LOG(ERROR) << "size field of parent node " << std::hex
                   << parent_location.second << " changed unexpectly";
        return false;
      }
      parent_bucket->key_info_field = (expected_key_field & key_field_hi_mask)
          | (iter & ~key_field_hi_mask);
      root_bucket_id_table_free()[location.second] = QED_ALL_MASK_64;
      return true;
    }
  }
//  WeakBucket* cache_line_start_bucket = weak_managed_bucket() + cache_line_start_id;
//  LOG(WARNING) << "try optmize failed:" << std::hex
//               << cache_line_start_bucket[0].key_info_field << ":"
//               << int(cache_line_start_bucket[0].size()) << ","
//               << cache_line_start_bucket[1].key_info_field << ":"
//               << int(cache_line_start_bucket[1].size()) << ","
//               << cache_line_start_bucket[2].key_info_field << ":"
//               << int(cache_line_start_bucket[2].size()) << ","
//               << cache_line_start_bucket[3].key_info_field << ":"
//               << int(cache_line_start_bucket[3].size());
  return false;
}

void BlockDataBuilder::Close() {
  block_file->Close();
  if (need_truncate) {
    truncate(block_file->file_name().c_str(), offset_);
  }
}

BlockDataBuilder::~BlockDataBuilder() {
  if (block_file->Get()) {
    Close();
  }
}

bool BlockDataBuilder::Open(size_t size, bool populate) {
  if (size == 0) {
    size = QED_MAX_BUILD_FILE_SIZE;
    need_truncate = true;
  } else if (size > QED_MAX_BUILD_FILE_SIZE) {
    LOG(WARNING) << "expected size " << size << " exceeds max build file size "
                 << QED_MAX_BUILD_FILE_SIZE;
    size = QED_MAX_BUILD_FILE_SIZE;
  }
  try {
    max_size_ = block_file->Open(size, populate);
    if (max_size_ < size) {
      return false;
    }
  } catch (const std::exception& e) {
    LOG(ERROR) << e.what();
    return false;
  }
  return true;
}

}  // namespace qed
