/*!
 * \file qed.cc
 * \brief The qed implementation
 *
 */
#include "qed/qed.h"

#include <dirent.h>
#include <sys/stat.h>
#include <string>
#include <algorithm>
#include <fstream>

#include "common/logging.h"
#include "yaml-cpp/yaml.h"
#include "qed/trie.h"
#include "qed/protocol.h"

namespace qed {

typedef QuickEmbeddingDict::FeagroupStore FeagroupStore;
typedef TrieNode<uint16_t, uint16_t, TrieType::STRONG> Trie_t;

QuickEmbeddingDict::QuickEmbeddingDict(const std::string& embedding_file_path)
    : value_type_(DictValueType::unknown),
      path_(embedding_file_path) {
}

QuickEmbeddingDict::~QuickEmbeddingDict() {
  for (FeagroupStore* fgs : fg_stores_) {
    delete (fgs);
  }
}

bool QuickEmbeddingDict::Validate(const std::string& embedding_file_path) {
  try {
    YAML::Node desc_node;
    desc_node = YAML::LoadFile(embedding_file_path + "/" + QED_DESC_FILE_NAME);
    if (!desc_node.IsMap()) {
      DLOG(INFO) << "not a map:" << desc_node;
      return false;
    }
    uint32_t gid_count = 0;
    bool has_gid_mapping = false;
    DictValueType type = DictValueType::unknown;
    struct stat file_stat{0};
    for (const auto& pair : desc_node) {
      if (pair.first.Scalar() == QED_DESC_KEY_DATA_VALUE_TYPE) {
        if (pair.second.Scalar() == "0") {
          type = DictValueType::fp32;
        } else if (pair.second.Scalar() == "1") {
          type = DictValueType::fp16;
        }
      } else if (pair.first.Scalar() == QED_DESC_KEY_GIDMAPPING_FILE_NAME) {
        auto gidmappingFile = embedding_file_path + "/" + pair.second.Scalar();
        if (stat(gidmappingFile.c_str(),
                 &file_stat) == 0
            && file_stat.st_size > 0) {
          has_gid_mapping = true;
        } else {
          DLOG(INFO) << "bad gid mapping file:" << gidmappingFile;
          return false;
        }
      } else if (pair.first.Scalar() == QED_DESC_KEY_GID_LIST) {
        for (const auto& spair : pair.second) {
          if (!spair.second.IsMap()) {
            DLOG(INFO) << "not a map:" << spair.second;
            return false;
          }
          const auto& gid = spair.first.Scalar();
          const auto& freeFile =
              embedding_file_path + "/"
                  + spair.second[QED_DESC_KEY_GID_LIST_ITEM_FREE].Scalar();
          const auto& managedFile =
              embedding_file_path + "/"
                  + spair.second[QED_DESC_KEY_GID_LIST_ITEM_MANAGED].Scalar();
          const auto& metaFile =
              embedding_file_path + "/"
                  + spair.second[QED_DESC_KEY_GID_LIST_ITEM_META].Scalar();
          if (stat(freeFile.c_str(), &file_stat) != 0) {
            DLOG(INFO) << "bad freeFile " << freeFile;
            return false;
          }
          if (stat(managedFile.c_str(), &file_stat) != 0) {
            DLOG(INFO) << "bad managedFile " << managedFile;
            return false;
          }
          if (stat(metaFile.c_str(), &file_stat) != 0) {
            DLOG(INFO) << "bad metaFile " << metaFile;
            return false;
          }
          gid_count++;
        }
      }
    }
    DLOG(INFO) << "gid count:" << gid_count << " has gid mapping:"
               << (has_gid_mapping ? "true" : "false") << " type:" << int(type);
    return gid_count > 0 && has_gid_mapping && type != DictValueType::unknown;
  } catch (const std::exception& e) {
    DLOG(INFO) << e.what();
    return false;
  }
}

bool QuickEmbeddingDict::Load(bool populate) {
  if (!fg_stores_.empty()
      || !hash_tables_.empty()
      || !data_blocks_.empty()
      || trie_data_ != nullptr) {
    return false;
  }
  try {
    YAML::Node desc_node;
    desc_node = YAML::LoadFile(path_ + "/" + QED_DESC_FILE_NAME);
    if (!desc_node.IsMap()) {
      return false;
    }
    const auto& dataType = desc_node[QED_DESC_KEY_DATA_VALUE_TYPE].Scalar();
    if (dataType == "0") {
      value_type_ = DictValueType::fp32;
    } else if (dataType == "1") {
      value_type_ = DictValueType::fp16;
    } else {
      LOG(ERROR) << "invalid data type:" << dataType;
      return false;
    }
    const auto& gidmapping_file =
        desc_node[QED_DESC_KEY_GIDMAPPING_FILE_NAME].Scalar();
    trie_data_.reset(new MmapedMemory(path_ + "/" + gidmapping_file));
    if (trie_data_->Open(0, populate) == 0) {
      trie_data_.reset();
      return false;
    }
    if (trie_data_->Get() == nullptr) {
      trie_data_.reset();
      return false;
    }
    const auto& gidNodes = desc_node[QED_DESC_KEY_GID_LIST];
    uint32_t max_gid = 0;
    for (const auto& pair : gidNodes) {
      max_gid = std::max<uint32_t>(max_gid,
                                   std::atoi(pair.first.Scalar().c_str()));
    }
    fg_stores_.resize(max_gid + 1, nullptr);
    for (const auto& pair : gidNodes) {
      uint32_t gid = std::atoi(pair.first.Scalar().c_str());
      if (fg_stores_[gid] != nullptr) {
        LOG(ERROR) << path_ << " duplicated gid " << gid << "|"
                   << pair.first.Scalar();
        fg_stores_.clear();
        return false;
      }
      const auto& freeFile =
          path_ + "/" + pair.second[QED_DESC_KEY_GID_LIST_ITEM_FREE].Scalar();
      const auto& managedFile =
          path_ + "/"
              + pair.second[QED_DESC_KEY_GID_LIST_ITEM_MANAGED].Scalar();
      const auto& metaFile =
          path_ + "/" + pair.second[QED_DESC_KEY_GID_LIST_ITEM_META].Scalar();
      const auto& dataFile =
          path_ + "/" + pair.second[QED_DESC_KEY_GID_LIST_ITEM_DATA].Scalar();
      auto freeMmap = new MmapedMemory(freeFile);
      auto managedMmap = new MmapedMemory(managedFile);
      auto metaMmap = new MmapedMemory(metaFile);
      auto dataMmap = new MmapedMemory(dataFile);
      data_blocks_.emplace_back(freeMmap);
      data_blocks_.emplace_back(managedMmap);
      data_blocks_.emplace_back(metaMmap);
      data_blocks_.emplace_back(dataMmap);
      metaMmap->Open(0, populate);
      auto metaInfo = reinterpret_cast<MetaInfo*>(metaMmap->Get());
      if (metaInfo == nullptr) {
        LOG(ERROR) << "Get meta info from " << metaFile << " failed";
        return false;
      }
      if (metaInfo->free_id_count != 0) {
        if (freeMmap->Open(0, populate)
            < metaInfo->free_id_count * sizeof(RawBucket)) {
          LOG(ERROR) << "Bad free file(" << freeFile
                     << ") size " << freeMmap->Size()
                     << " expected:"
                     << metaInfo->free_id_count * sizeof(RawBucket);
          return false;
        }
      }
      if (managedMmap->Open(0, populate) <
          (uint64_t(1) << metaInfo->bucket_type) * sizeof(RawBucket)) {
        LOG(ERROR) << "Bad managed file(" << managedFile
                   << ") size " << managedMmap->Size()
                   << " expected:"
                   << (uint64_t(1) << metaInfo->bucket_type)
                       * sizeof(RawBucket);
        return false;
      }
      if (dataMmap->Open(0, populate) == 0) {
        LOG(ERROR) << "Open data file " << dataFile << " failed";
        return false;
      }
      auto hash_table =
          new HashTable(reinterpret_cast<WeakBucket*>(managedMmap->Get()),
                        reinterpret_cast<WeakBucket*>(freeMmap->Get()),
                        metaInfo->bucket_type);
      hash_tables_.emplace_back(hash_table);
      auto fg_store = new FeagroupStore();
      fg_store->block_ = dataMmap;
      fg_store->table_ = hash_table;
      fg_store->gid_ = gid;
      auto ext_dim_node = pair.second[QED_DESC_KEY_GID_LIST_ITEM_EXT_DIM];
      if (ext_dim_node && ext_dim_node.IsScalar()) {
        fg_store->dim_ = std::stoi(ext_dim_node.Scalar());
      } else {
        fg_store->dim_ = 0;
      }
      fg_stores_[gid] = fg_store;
    }
  } catch (const std::exception& e) {
    LOG(ERROR) << e.what();
    return false;
  }
  return !fg_stores_.empty() && !hash_tables_.empty() && !data_blocks_.empty()
      && trie_data_ != nullptr;
}

const FeagroupStore* QuickEmbeddingDict::GetGid(uint16_t gid) const {
  if (unlikely(gid >= fg_stores_.size())) {
    return nullptr;
  }
  return fg_stores_[gid];
}

const FeagroupStore* QuickEmbeddingDict::GetGid(const char* feagroup_id,
                                                size_t len) const {
  if (unlikely(!trie_data_ || trie_data_->Get() == nullptr)) {
    return nullptr;
  }
  auto trie_root = reinterpret_cast<const Trie_t*>(trie_data_->Get());
  auto node = find_trie(feagroup_id, len, trie_root);
  if (unlikely(node == nullptr)) {
    return nullptr;
  }
  auto gid = node->value;
  if (unlikely(gid >= fg_stores_.size())) {
    return nullptr;
  }
  return fg_stores_[gid];
}

}  // namespace qed
