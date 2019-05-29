/*!
 * \file protocol.h
 * \brief The protocol in QED
 */
#ifndef QED_PROTOCOL_H_
#define QED_PROTOCOL_H_

namespace qed {

struct RawBucket {
  RawBucket() : key_info_field(0), size_field(0), payload(0) {}
  uint64_t key_info_field;
  uint8_t size_field;
  // NOTE: access to bit field need gcc -O2 to get acceptable
  // performance
  uint64_t payload : 56;
  // end of member variables
};

struct MetaInfo {
  uint64_t impl_version;
  uint8_t  bucket_type;
  uint64_t reserved_field : 56;
  uint64_t managed_id_count;
  uint64_t free_id_count;
  uint64_t extended_field_size;
};

#define QED_DESC_FILE_NAME                   "desc.yml"
#define QED_DESC_KEY_DATA_VALUE_TYPE         "data_type"
#define QED_DESC_KEY_GIDMAPPING_FILE_NAME    "gidmapping_file"
#define QED_DESC_KEY_GID_LIST                "gids"
#define QED_DESC_KEY_GID_LIST_ITEM_MANAGED   "managed"
#define QED_DESC_KEY_GID_LIST_ITEM_FREE      "free"
#define QED_DESC_KEY_GID_LIST_ITEM_META      "meta"
#define QED_DESC_KEY_GID_LIST_ITEM_DATA      "data"
#define QED_DESC_KEY_GID_LIST_ITEM_EXT_DIM   "dim"

}  // namespace qed

#endif  // QED_PROTOCOL_H_
