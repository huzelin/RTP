/*!
 * \file hash.h
 * \brief The hash function in RTP.
 */
#ifndef QED_HASH_H_
#define QED_HASH_H_

#include <cstdint>
#include <type_traits>
#include <string>

#include "common/murmurhash.h"

namespace qed {

template<typename Key, typename HashKey = hash_key_t>
struct Hash {
  HashKey operator()(const Key& key) const noexcept {
    static_assert(std::is_same<Key, HashKey>::value, "hash not supported");
    return key;
  }
};

template<>
struct Hash<std::string, hash_key_t> {
  hash_key_t operator()(const std::string& key) const noexcept{
    return MurmurHash64A(key.c_str(), key.size());
  }
};

template<>
struct Hash<qed::cstr, hash_key_t> {
  hash_key_t operator()(const qed::cstr& key) const noexcept{
    return MurmurHash64A(key.data, key.size);
  }
};

}  // namespace qed

#endif  // QED_HASH_H_
