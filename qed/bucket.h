/*
 * \file bucket.h
 * \brief The bucket definition
 */
#ifndef QED_BUCKECT_H_
#define QED_BUCKECT_H_

#include <cstdint>
#include <string>
#include <type_traits>

#include "qed/basic.h"
#include "qed/protocol.h"

namespace qed {

/**
 * @brief a 16 byte struct used to present information of each bucket slot
 * @param
 *      true if next bucket is managed
 * @param size_field
 *      highest bit presents next_bucket_is_internal, max size is 127,
 *      0 if current bucket is empty,
 *      an empty bucket slot may be used to storage information of other buckets
 * @param payload
 *      information storaged in this bucket
 * @param key_info_field
 * @verbatim
 *      mask = 0xffffffffffffffff &lt&lt n;
 *      key_info_field = (hash(key) & mask) | (next_bucket_offset &~ mask)
 * @endverbatim
 *      combine of key_high_bits next_bucket_offset.
 *      key_high_bits is high 64-n bits of the hashed key,
 *      can be used to do conflict check inside the bucket
 *      next_bucket_offset is offset to next bucket if there's any.
 *      reserved if this is the last element in bucket.
 *      if next_bucket_is_internal is false, next_bucket_offset will be point to the free buckets area.
 *
 * @verbatim
 * Binary structure(LE) is defined as below:
 *
 *                    8 bytes
 *        +-+--------+------...--------+
 *        |*|  7bits |     7bytes      |
 *        +-+--------+------...--------+
 *        |  size    |    payload      |
 *        | max 127  |                 |
 *        +----------+------...--------+
 *        |           8 bytes          |
 *        +-----...------+--...--------+
 *        |   key high   &   bucket    |
 *        |      bits        offset    |
 *        +-----...------+-------------+
 * *: next_bucket_is_internal
 * @endverbatim
 */
template<int n>
struct Bucket : public RawBucket {
  static_assert(n < 64 && n > 0, "bucket count must between 2~2^63");
  typedef std::integral_constant<int, n> n_;
  typedef std::integral_constant<uint64_t, QED_ALL_MASK_64 << n>
      key_info_hi_mask_;
  typedef std::integral_constant<uint8_t, 0x80> size_filed_hi_mask_;

  inline void SetSize(uint8_t size) {
    size_field = (size_field & size_filed_hi_mask_::value)
        | (size & ~size_filed_hi_mask_::value);
  }

  inline void SetKey(uint64_t key) {
    key_info_field = (key & key_info_hi_mask_::value) | next_bucket_id();
  }

  inline void SetNextBucketId(uint64_t id, bool internal = true) {
    if (internal) {
      size_field |= size_filed_hi_mask_::value;
    } else {
      size_field &= ~size_filed_hi_mask_::value;
    }
    key_info_field = (key_info_field & key_info_hi_mask_::value)
        | (id & ~key_info_hi_mask_::value);
  }

  inline bool TrySetSize(uint8_t size) {
    auto expected = size_field;
    return __sync_bool_compare_and_swap(
        &size_field,
        expected,
        (expected & size_filed_hi_mask_::value)
        | (size & ~size_filed_hi_mask_::value));
  }

  inline bool TrySetKey(uint64_t key) {
    auto expected = key_info_field;
    return __sync_bool_compare_and_swap(
        &key_info_field,
        expected,
        (key & key_info_hi_mask_::value)
        | (expected & ~key_info_hi_mask_::value));
  }

  inline bool TrySetNextBucketId(uint64_t id, bool internal = true) {
    auto expected_size_field = size_field;
    auto expected_key_field = key_info_field;
    return __sync_bool_compare_and_swap(
        &size_field,
        expected_size_field,
        internal ? (expected_size_field | size_filed_hi_mask_::value)
        : (expected_size_field & ~size_filed_hi_mask_::value))
        && __sync_bool_compare_and_swap(
            &key_info_field,
            expected_key_field,
            (expected_key_field & key_info_hi_mask_::value)
            | (id & ~key_info_hi_mask_::value));
  }

  inline bool next_bucket_is_internal() {
    return (size_field & size_filed_hi_mask_::value) != 0;
  }

  inline bool match_high_bits(uint64_t key) {
    return key_high_bits() == (key & key_info_hi_mask_::value);
  }

  inline uint8_t size() {
    return size_field & ~size_filed_hi_mask_::value;
  }

  inline uint64_t next_bucket_id() {
    return key_info_field & ~key_info_hi_mask_::value;
  }

  inline uint64_t key_high_bits() {
    return key_info_field & key_info_hi_mask_::value;
  }

  inline Bucket<n>* next_bucket(Bucket<n>* managed_buckets,
                                Bucket<n>* free_buckets) {
    return next_bucket_is_internal()
        ? (managed_buckets + next_bucket_id())
        : (free_buckets + next_bucket_id());
  }
};

/**
 * @brief weak type bucket, key info hi-mask need to be provided by caller.
 *        this type is used for accelerate runtime lookup by avoiding
 *        virtual function or switch/case caused branch prediction failure
 * @see qed::template&lt int&gt struct Bucket
 */
struct WeakBucket : public RawBucket {
  typedef std::integral_constant<uint8_t, 0x80> size_filed_hi_mask_;

  /**
   * @brief writing operation disabled by default
   */
#ifdef QED_ENABLE_WEAKBUCKET_WRITING
  inline void SetSize(uint8_t size) {
    size_field = (size_field & size_filed_hi_mask_::value)
        | (size & ~size_filed_hi_mask_::value);
  }

  inline void SetKey(uint64_t key_info_hi_mask, uint64_t key) {
    key_info_field =
        (key & key_info_hi_mask) | next_bucket_id(key_info_hi_mask);
  }

  inline void SetNextBucketId(uint64_t key_info_hi_mask,
                              uint64_t id,
                              bool internal = true) {
    if (internal) {
      size_field |= size_filed_hi_mask_::value;
    } else {
      size_field &= ~size_filed_hi_mask_::value;
    }
    key_info_field = (key_info_field & key_info_hi_mask)
        | (id & ~key_info_hi_mask);
  }
#endif

  inline bool next_bucket_is_internal() {
    return (size_field & size_filed_hi_mask_::value) != 0;
  }

  inline bool match_high_bits(uint64_t key_info_hi_mask, uint64_t key) {
    return key_high_bits(key_info_hi_mask) == (key & key_info_hi_mask);
  }

  inline uint8_t size() {
    return size_field & ~size_filed_hi_mask_::value;
  }

  inline uint64_t next_bucket_id(uint64_t key_info_hi_mask) {
    return key_info_field & ~key_info_hi_mask;
  }

  inline uint64_t key_high_bits(uint64_t key_info_hi_mask) {
    return key_info_field & key_info_hi_mask;
  }

  inline WeakBucket* next_bucket(uint64_t key_info_hi_mask,
                                 WeakBucket* managed_buckets,
                                 WeakBucket* free_buckets) {
    return next_bucket_is_internal()
        ? (managed_buckets + next_bucket_id(key_info_hi_mask))
        : (free_buckets + next_bucket_id(key_info_hi_mask));
  }
};

}  // namespace qed

#endif  // QED_BUCKET_H_
