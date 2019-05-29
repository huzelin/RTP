/*!
 * \file builder.h
 * \brief The offline builder of QED
 */
#ifndef QED_BUILDER_H_
#define QED_BUILDER_H_

#include <string>
#include <atomic>
#include <memory>
#include <functional>

#include "qed/mmap.h"
#include "qed/protocol.h"

namespace qed {

typedef std::function<std::unique_ptr<MemoryFile>(const std::string&)>
    MemFilePtrCreatorFunc;
class Builder {
 public:
  typedef std::pair<bool, uint64_t> location_t;
  Builder(const std::string& managed_bucket_file,
          const std::string& free_bucket_file,
          const std::string& root_bucket_id_table_managed_file,
          const std::string& root_bucket_id_table_free_file,
          const std::string& meta_file,
          MemFilePtrCreatorFunc&& iocreator =
          [](const std::string& f) -> std::unique_ptr<MemoryFile> {
            return std::unique_ptr<MemoryFile>(new MmapedMemory(f));
          })
      : managed_bucket_(iocreator(managed_bucket_file)),
        free_bucket_(iocreator(free_bucket_file)),
        root_bucket_id_table_managed_(iocreator(
            root_bucket_id_table_managed_file)),
        root_bucket_id_table_free_(iocreator(root_bucket_id_table_free_file)),
        meta(iocreator(meta_file)) {}

  ~Builder();

  /**
   * @brief Open meta file to get infomation
   * @param reset
   * @return
   */
  bool OpenMeta(bool reset = true);

  /**
   * @brief this must be implicitly called to finish the build process
   */
  void Close();

  bool Reserve(uint64_t reserved_count);

  /**
   * @brief open data files for access
   * @param reservice_count if not 0, Reserve is called inside of this function,
   *                        if 0, will open file as max file size, need
   *                        to note this will increase the chance of failure
   *                        caused by file open failue
   * @return true if success
   */
  bool OpenDataFiles(uint64_t reserved_count = 0);
  void CloseDataFiles();

  /**
   * @brief rearrange buckets on specified range, this should be called when
   *        change of meta_info()->bucket_type is detected on each worker.
   *        this function is concurrent safe if range of each call doesn't
   *        overlap.
   * @note FOR CURRENT IMPLEMENTATION worker_count MUST BE A N POWER OF 2
   *       LIKE 1, 2, 4, 8, 16 ETC. IF RearrangeBuckets WILL BE CALLED DURING INSERTION
   * @param begin_bucket_id
   * @param bucket_count
   * @return true if success
   */
  bool RearrangeBuckets(uint64_t begin_bucket_id, uint64_t bucket_count);

  /**
   * @brief Insert key and payload
   * @tparam payload_type type of payload, only uint64_t is supported for now
   * @param key_id
   * @param payload if payload_type is uin64_t, only low 7bytes will be storaged
   *        due to protocol limitation, larger payload will be supported in the future
   * @param free_slot free bucket id provided for insertion, should get by take_free_bucket_id,
   *                  if UINT64_MAX is provided, the implementation will try to
   *                  call take_free_bucket_id
   * @return pair of <is_managed_bucket, inserted_bucket_id>, if insertion failed,
   *         value of inserted_bucket_id will be QED_HASHTABLE_NFOUND.
   *         insertion failure may caused by bad data or unexpected change of data.
   */
  template<typename payload_type = uint64_t>
  location_t Insert(uint64_t key_id, payload_type payload,
                    uint64_t free_slot = UINT64_MAX);

  /**
   * @brief Get suggested id of worker, concurrency safety is only guaranteed
   *        when key_id is serial Inserted by the returned worker
   * @param key_id
   * @param worker_count
   * @return id of worker from 0 to worker_count-1
   */
  inline uint64_t GetWorkerId(uint64_t key_id, uint64_t worker_count) {
    static_assert(sizeof(RawBucket) == 16,
                  "extended raw bucket is not supported by now");
    // 4 bucket(16byte) in one cacheline(64 byte), bucket in the same cacheline
    // should not be shared by different worker. so there will be no problem
    // when reuse empty bucket in the same cacheline
    uint64_t key_cache_line_id = key_id >> 2;
    return key_cache_line_id % worker_count;
  }

  /**
   * @brief Try optimize provided free bucket into managed table
   * @param location location of free bucket
   * @param level number for cachelines this function will go down to find a fit slot
   * @return true if success
   */
  bool OptimizeFreeBucket(const location_t& location, uint8_t level = 0);

  /**
   * @brief Erase key
   * @note not yet implemented
   * @param key_id
   * @return pair of <is_internal_bucket, erased_bucket_id>, if erase failed,
   *         value of erased_bucket_id will be QED_HASHTABLE_NFOUND.
   *         insertion failure may caused by bad data or unexpected change of data.
   */
  location_t Erase(uint64_t key_id);

  template<int n>
  Bucket<n>* managed_bucket() {
    return reinterpret_cast<Bucket<n>*>(managed_bucket_->Get());
  }
  template<int n>
  Bucket<n>* free_bucket() {
    return reinterpret_cast<Bucket<n>*>(free_bucket_->Get());
  }

  /**
   * @brief take several free buckets to write with
   * @param count
   * @return id of first writable free bucket,
   *         UINT64_MAX if not enough free buckets to take from
   */
  uint64_t TakeFreeBucketId(uint64_t count = 1) {
    uint64_t current_free_bucket_count;
    do {
      current_free_bucket_count = meta_info()->free_id_count;
      if ((current_free_bucket_count + count)
          > (uint64_t(1) << meta_info()->bucket_type)) {
        return UINT64_MAX;
      }
    } while (!__sync_bool_compare_and_swap(&meta_info()->free_id_count,
                                           current_free_bucket_count,
                                           current_free_bucket_count + count));
    return current_free_bucket_count;
  }

  /**
   * @brief find parent node of provided location
   * @param location
   * @return parent location, if location pointed to a root node,
   *         its own location will be returned
   */
  location_t FindParent(const location_t& location);

  /**
   * @brief find root bucket of location provided
   * @tparam n bucket type
   * @param location
   * @return nullptr if location is invalid
   */
  template<int n>
  inline Bucket<n>* find_root(const location_t& location) {
    if (location.second >= (uint64_t(1) << n)) {
      return nullptr;
    }
    uint64_t root_id = root_id_by_location(location);
    if (root_id == QED_ALL_MASK_64
        || root_id >= (uint64_t(1) << n)) {
      return nullptr;
    }
    return managed_bucket<n>() + root_id;
  }

  /**
   * @brief Get a bucket pointer by location
   * @param location
   * @return nullptr if location is invalid
   */
  inline WeakBucket* bucket_by_location(const location_t& location) {
    if (location.first) {
      if (location.second >= (uint64_t(1) << meta_info()->bucket_type)) {
        return nullptr;
      }
      return weak_managed_bucket() + location.second;
    } else {
      if (location.second >= meta_info()->free_id_count) {
        return nullptr;
      }
      return weak_free_bucket() + location.second;
    }
  }

  /**
   * @brief Get root bucket id for location
   * @param location
   * @return id of managed root bucket, QED_ALL_MASK_64 if location
   *         does not belong to a bucket chain
   */
  inline uint64_t root_id_by_location(const location_t& location) {
    if (location.first) {
      if (location.second >= (uint64_t(1) << meta_info()->bucket_type)) {
        return QED_ALL_MASK_64;
      }
      return root_bucket_id_table_managed()[location.second];
    } else {
      if (location.second >= meta_info()->free_id_count) {
        return QED_ALL_MASK_64;
      }
      return root_bucket_id_table_free()[location.second];
    }

  }

  WeakBucket* weak_managed_bucket() {
    return reinterpret_cast<WeakBucket*>(managed_bucket_->Get());
  }
  WeakBucket* weak_free_bucket() {
    return reinterpret_cast<WeakBucket*>(free_bucket_->Get());
  }
  MetaInfo* meta_info() {
    return reinterpret_cast<MetaInfo*>(meta->Get());
  }
  uint64_t* root_bucket_id_table_managed() {
    return reinterpret_cast<uint64_t*>(root_bucket_id_table_managed_->Get());
  }
  uint64_t* root_bucket_id_table_free() {
    return reinterpret_cast<uint64_t*>(root_bucket_id_table_free_->Get());
  }
  std::unique_ptr<MemoryFile> managed_bucket_;
  std::unique_ptr<MemoryFile> free_bucket_;
  std::unique_ptr<MemoryFile> root_bucket_id_table_managed_;
  std::unique_ptr<MemoryFile> root_bucket_id_table_free_;
  std::unique_ptr<MemoryFile> meta;
};

class BlockDataBuilder {
 public:
  BlockDataBuilder(const std::string& file,
                   MemFilePtrCreatorFunc&& iocreator =
                   [](const std::string& f) -> std::unique_ptr<MemoryFile> {
                     return std::unique_ptr<MemoryFile>(new MmapedMemory(
                         f));
                   })
      : offset_(0),
        max_size_(0),
        need_truncate(false),
        block_file(iocreator(file)) {}
  ~BlockDataBuilder();

  /**
   * @brief
   * @param size if zero, file size will not be limited
   * @param populate true if populate file to memory
   * @return
   */
  bool Open(size_t size = 0, bool populate = false);

  /**
   * @brief close file, if file was Opened with size 0, file will be truncated
   *        to allocated size
   */
  void Close();

  template<typename T>
  size_t alloc(size_t count) {
    size_t current_offset;
    do {
      current_offset = offset_;
      if (count * sizeof(T) + current_offset > max_size_) {
        return UINT64_MAX;
      }
    } while (!offset_.compare_exchange_strong(current_offset,
                                              current_offset
                                                  + count * sizeof(T)));
    return current_offset;
  }

  template<typename T>
  T* base() {
    return reinterpret_cast<T*>(block_file->Get());
  }

  template<typename T>
  T* get(size_t offset) {
    return reinterpret_cast<T*>(base<char>() + offset);
  }

  size_t size() {
    return offset_;
  }

 protected:
  std::atomic<size_t> offset_;
  size_t max_size_;
  bool need_truncate;
  std::unique_ptr<MemoryFile> block_file;
};

}  // namespace qed

#endif  // QED_BUILDER_H_
