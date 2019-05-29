/*!
 * \file qed.h
 * \brief The qed header
 */
#ifndef QED_QED_H_
#define QED_QED_H_

#include <memory>
#include <vector>

#include "qed/hashtable.h"
#include "qed/mmap.h"

namespace qed {

/**
 * @brief High level wrapper for model serving use case
 */
class QuickEmbeddingDict {
 public:
  struct FeagroupStore {
    const HashTable* table_;
    const MmapedMemory* block_;
    uint16_t gid_;
    int dim_;  // if 0, dim information is missing,
               // user should consider follow embedding config
  };
  QuickEmbeddingDict(const std::string& embedding_file_path);
  virtual ~QuickEmbeddingDict();
  QuickEmbeddingDict(const QuickEmbeddingDict&) = delete;
  QuickEmbeddingDict& operator=(const QuickEmbeddingDict&) = delete;

  /**
   * @brief check if is a qed2 embedding dict path
   * @return true if able to load (may fail if data corrupt)
   */
  static bool Validate(const std::string& embedding_file_path);

  /**
   * @brief load quick embedding
   * @param populate load data populated, for online engine this should always
   *                 be true to get a acceptable performance,
   *                 none populated load will boost the load speed, can be used
   *                 in case like embedding dict lookup utility tools
   * @return true if success
   */
  bool Load(bool populate = true);

  /**
   * @brief get value type
   * @return value type of this quick embedding dict
   */
  DictValueType ValueType() const {
    return value_type_;
  }

  /**
   * @brief get feature group storage
   * @param feagroup_id
   * @return pointer to storage result, nullptr if not found
   */
  const FeagroupStore* GetGid(const char* feagroup_id, size_t len) const;

  /**
   * @brief get feature group storage
   * @param gid
   * @return pointer to storage result, nullptr if not found
   */
  const FeagroupStore* GetGid(uint16_t gid) const;

  /**
   * @brief lookup for value
   * @tparam T value type, can get by QED_SWITCHTYPE_DictValueType
   * @tparam verbose if true, do verbose find, strictly check table value for realtime upda support
   * @param store FeagroupStore pointer returned by GetGid
   * @param fid feature id to lookup for
   * @param dict_value pointer to storage result
   * @return true if success
   */
  template<typename T, bool verbose = false>
  inline bool Lookup(const FeagroupStore* store,
                     uint64_t fid,
                     T** dict_value) const {
    if (unlikely(store == nullptr
                     || dict_value == nullptr
                     || DictValueTypeFromType<typename std::decay<T>::type>::valueType() != value_type_)) {
      return false;
    }
    auto payload = store->table_->Find<verbose>(fid);
    if (unlikely(payload == QED_HASHTABLE_NFOUND)) {
      return false;
    }
    *dict_value = reinterpret_cast<T*>(
        reinterpret_cast<char*>(store->block_->Get()) + payload);
    return true;
  }

 protected:
  DictValueType value_type_;
  std::vector<FeagroupStore*> fg_stores_;
  std::vector<std::unique_ptr<HashTable>> hash_tables_;
  std::vector<std::unique_ptr<MmapedMemory>> data_blocks_;
  std::unique_ptr<MmapedMemory> trie_data_;
  std::string path_;
};

}  // namespace qed

#endif  // QED_QED_H_
