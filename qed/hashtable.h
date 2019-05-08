/*!
 * \file hashtable.h
 * \brief The hashtable impls.
 */
#ifndef QED_HASHTABLE_H_
#define QED_HASHTABLE_H_

#include <functional>
#include <x86intrin.h>

#include "qed/mmap.h"
#include "qed/hash.h"
#include "qed/bucket.h"

#include "common/logging.h"

namespace qed {

#ifdef __AVX512F__
#define QED_BATCH_SIZE 8
#else
#define QED_NO_BATCH_SUPPORT
#define QED_BATCH_PREFETCH
#define QED_BATCH_SIZE 8
#endif

#if defined(__GNUC__) && !defined(__clang__)
#define GCC_VERSION (__GNUC__ * 10000 \
                     + __GNUC_MINOR__ * 100 \
                     + __GNUC_PATCHLEVEL__)
#if GCC_VERSION < 50100
#define _mm512_cmpeq_epu64_mask(A, B) \
  _mm512_cmp_epu64_mask((A), (B), _MM_CMPINT_EQ)
#endif
#endif

/**
 * @brief hashtable read api
 * @note for the same bucket, only one writer is allowed, or there will be a data race issue.
 */
class HashTable {
 public:
  HashTable(WeakBucket* managed_buckets,
            WeakBucket* free_buckets,
            uint8_t bucket_size_log2)
      : managed_buckets_(managed_buckets),
      free_buckets_(free_buckets),
      key_info_hi_mask_(QED_ALL_MASK_64 << bucket_size_log2),
      bucket_size_log2_(bucket_size_log2) {
#ifndef QED_NO_BATCH_SUPPORT
        batch_key_info_hi_mask_ = _mm512_set1_epi64(key_info_hi_mask_);
        batch_size_field_hi_max_ = _mm512_set1_epi64(0xFFFFFFFFFFFFFF80);
#endif
      }

  ~HashTable() = default;
  /**
   * @brief find payload by key_id
   * @param vread true if do a volatile Find, volatile Find will treat hash table as being changed by other process/thread at runtime.
   *              none volatile Find can not promise data race safety if hash table is real time updated
   * @param key_id hashed key
   * @return 7Bytes of payload, QED_HASHTABLE_NFOUND if no key_id related bucket was found
   */
  template<bool vread = false>
  uint64_t Find(uint64_t key_id) const {
    uint64_t bucket_id = key_id & ~key_info_hi_mask_;
    auto entry_bucket = managed_buckets_ + bucket_id;
    auto bucket = entry_bucket;
    uint8_t size = bucket->size();
    if (unlikely(size == 0)) {
      DLOG(INFO) << "empty:" << key_id
                 << ":" << bucket_id
                 << ":" << bucket->size(); 
      return QED_HASHTABLE_NFOUND;
    }
    DLOG(INFO) << "ne:" << key_id
               << ":" << bucket_id
               << ":" << bucket->size();
    const uint64_t key_high_bits = key_id & key_info_hi_mask_;
    for (int i = 0; i < size; i++) {
      if (likely(bucket->key_high_bits(key_info_hi_mask_) == key_high_bits)) {
        if (vread) {
          // get payload before size check to ensure data version
          // match each other
          auto payload = bucket->payload;
          // place a SW barrier here to guarantee load of payload here is not
          // opitmized out or moved to below by compiler
          asm volatile("": : :"memory");
          auto currentSize = bucket->size();
          // for 1st check(entry bucket), if size is 0 equals to i(0) means
          // removed for following iterations, if size is not 0 means slot
          // occupied by others, need go back and recheck
          if (unlikely(
                  ((currentSize == 0 && i == 0) || (currentSize != 0 && i != 0))
                  // recheck payload to avoid a data changed and
                  // then removed condition
                  || payload != bucket->payload)) {
            // bucket chain updated, recheck
            DLOG(INFO) << "chain error:" << key_id
                       << ":" << bucket_id
                       << ":" << bucket->size();
            bucket = entry_bucket;
            size = bucket->size();
            i = -1;
            continue;
          } else {
            return payload;
          }
        } else {
          return bucket->payload;
        }
      }
      DLOG(INFO) << "a:" << key_id
                 << ":" << bucket->key_info_field
                 << ":" << bucket->size()
                 << ":" << bucket_id;
      bucket = bucket->next_bucket(key_info_hi_mask_,
                                   managed_buckets_,
                                   free_buckets_);
    }
    return QED_HASHTABLE_NFOUND;
  }

#ifndef QED_NO_BATCH_SUPPORT
  /**
   * @brief batched find payload by key_id
   * @note batched find doesn't support verbose read by now
   * @param key_id hashed key
   * @param shallow_find true if do a shallow find which only check the root node
   *                     of each bucket, if root node doesn't match and there are
   *                     more nodes, QED_HASHTABLE_NVALUE will be returned
   * @return 8*7Bytes of payload, QED_HASHTABLE_NFOUND if no key_id related bucket was found,
   *         if QED_HASHTABLE_NVALUE, user should call Find to find again,
   */
  __m512i BatchFind(__m512i key_id, bool shallow_find = false) const {
    auto bucket_id = _mm512_andnot_si512(batch_key_info_hi_mask_, key_id);
    // left shift 4 equals mul 16, to get the bucket_id in bytes which
    // _mm512_i64gather_epi64 required
    bucket_id = _mm512_slli_epi64(bucket_id, 4);
    auto size_field =
        _mm512_i64gather_epi64(bucket_id,
                               reinterpret_cast<const long long int*>
                               (reinterpret_cast<char*>(managed_buckets_)
                                + offsetof(RawBucket, size_field)),
                               1);
    auto key_field =
        _mm512_i64gather_epi64(bucket_id,
                               reinterpret_cast<const long long int*>
                               (reinterpret_cast<char*>(managed_buckets_)
                                + offsetof(RawBucket, key_info_field)),
                               1);
    // each bit of mask: true if size 0
    auto size = _mm512_andnot_si512(batch_size_field_hi_max_, size_field);
    auto next_internal_mask =
        _mm512_test_epi64_mask(size_field, _mm512_set1_epi64(0x80));
    auto next_bucket_id =
        _mm512_andnot_epi64(batch_key_info_hi_mask_, key_field);
#ifdef QED_BATCH_PREFETCH
    // do prefetch asap
    // don't prefetch internal mask because they should already in cache if built correctly
    _mm512_mask_prefetch_i64gather_pd(next_bucket_id, ~next_internal_mask,
                                      reinterpret_cast<const long long int*>(
                                          reinterpret_cast<char*>(free_buckets_)
                                          + offsetof(RawBucket,
                                                     size_field)),
                                      gather_scale,
                                      1);  // hint for load into L1 cache
    _mm512_mask_prefetch_i64gather_pd(next_bucket_id, ~next_internal_mask,
                                      reinterpret_cast<const long long int*>(
                                          reinterpret_cast<char*>(free_buckets_)
                                          + offsetof(RawBucket,
                                                     key_info_field)),
                                      gather_scale,
                                      1);  // hint for load into L1 cache
#endif  // QED_BATCH_PREFETCH
    auto batch_zero = _mm512_setzero_si512();
    __mmask8 zero_size_mask = _mm512_cmpeq_epu64_mask(size,
                                                      batch_zero);
    if (unlikely(zero_size_mask == 0xFF)) {
      // if all empty, return immediatly
      return _mm512_set1_epi64(QED_HASHTABLE_NVALUE);
    }
    auto high_key_id = _mm512_and_epi64(batch_key_info_hi_mask_, key_id);
    auto matched_mask = _mm512_cmpeq_epi64_mask(
        high_key_id,
        _mm512_and_epi64(batch_key_info_hi_mask_, key_field));
    // right shift 8 bit to get payload
    auto payload = _mm512_srli_epi64(size_field, 8);
    // if match or size zero, look up is finished for the element
    auto finished_mask = matched_mask | zero_size_mask;
    // if size is zero, result should be QED_HASHTABLE_NFOUND, or payload is set
    __m512i result =
        _mm512_mask_set1_epi64(payload, zero_size_mask,
                               QED_HASHTABLE_NFOUND);
    if (shallow_find) {
      return _mm512_mask_set1_epi64(result,
                                    ~finished_mask,
                                    QED_HASHTABLE_NVALUE);
    }
    while (finished_mask != 0xFF) {
      next_bucket_id = _mm512_slli_epi64(next_bucket_id, 4);
      size_field =
          _mm512_mask_i64gather_epi64(size_field,
                                      next_internal_mask & ~finished_mask,
                                      next_bucket_id,
                                      reinterpret_cast<const long long int*>(
                                          reinterpret_cast<char*>(managed_buckets_)
                                          + offsetof(RawBucket,
                                                     size_field)),
                                      1);
      size_field =
          _mm512_mask_i64gather_epi64(size_field,
                                      ~next_internal_mask & ~finished_mask,
                                      next_bucket_id,
                                      reinterpret_cast<const long long int*>(
                                          reinterpret_cast<char*>(free_buckets_)
                                          + offsetof(RawBucket,
                                                     size_field)),
                                      1);
      key_field =
          _mm512_mask_i64gather_epi64(key_field,
                                      next_internal_mask & ~finished_mask,
                                      next_bucket_id,
                                      reinterpret_cast<const long long int*>(
                                          reinterpret_cast<char*>(managed_buckets_)
                                          + offsetof(RawBucket,
                                                     key_info_field)),
                                      1);
      key_field =
          _mm512_mask_i64gather_epi64(key_field,
                                      ~next_internal_mask & ~finished_mask,
                                      next_bucket_id,
                                      reinterpret_cast<const long long int*>(
                                          reinterpret_cast<char*>(free_buckets_)
                                          + offsetof(RawBucket,
                                                     key_info_field)),
                                      1);
      next_internal_mask =
          _mm512_test_epi64_mask(size_field, _mm512_set1_epi64(0x80));
      next_bucket_id =
          _mm512_andnot_epi64(batch_key_info_hi_mask_, key_field);
      // IMPORTANT: real-time updated data is not friendly to batched lookup
      //            we don't plan to support it by now
#if GCC_VERSION < 50100 || !defined(__AVX512BW__)
      // AVX512BW is not supported until gcc 5.1
      size = _mm512_sub_epi64(size, _mm512_set1_epi64(0x1));
      size = _mm512_mask_set1_epi64(size,
                                    _mm512_cmplt_epi64_mask(size, batch_zero),
                                    0);
#else
      // saturate minus 1 from size to get a minium 0 result
      size = _mm512_subs_epu16(size, _mm512_set1_epi64(0x1));
#endif
      zero_size_mask = _mm512_cmpeq_epu64_mask(size,
                                               batch_zero);
      auto current_round_matched_mask = _mm512_mask_cmpeq_epi64_mask(
          ~finished_mask, high_key_id,
          _mm512_maskz_and_epi64(finished_mask,
                                 batch_key_info_hi_mask_,
                                 key_field));
      // right shift 8 bit to get payload
      payload = _mm512_srli_epi64(size_field, 8);
      // if match or size zero, look up is finished for the element
      result =
          _mm512_mask_mov_epi64(result, current_round_matched_mask, payload);
      matched_mask |= current_round_matched_mask;
      finished_mask |= (matched_mask | zero_size_mask);
    }
    return _mm512_mask_set1_epi64(result,
                                  ~matched_mask,
                                  QED_HASHTABLE_NVALUE);
  }
#endif
 
 protected:
  WeakBucket* managed_buckets_;
  WeakBucket* free_buckets_;
  uint64_t key_info_hi_mask_;
#ifndef QED_NO_BATCH_SUPPORT
  __m512i batch_key_info_hi_mask_;
  __m512i batch_size_field_hi_max_;
#endif
  uint8_t bucket_size_log2_;
};

}  // namespace qed

#endif  // QED_HASHTABLE_H_
