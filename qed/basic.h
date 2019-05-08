/*!
 * \file basic.h
 * \brief The basics
 */
#ifndef QED_BASIC_H_
#define QED_BASIC_H_

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace qed {

#define QED_HASHTABLE_NFOUND 0xFFFFFFFFFFFFFFFF
#define QED_HASHTABLE_NVALUE 0xF0FFFFFFFFFFFFFF
#define QED_ALL_MASK_64 0xFFFFFFFFFFFFFFFF

#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)

typedef float fp32_t;
typedef unsigned short fp16_t;

typedef uint64_t hash_key_t;
typedef struct {
  const char* data;
  uint64_t size;
} cstr;

}  // namespace qed

#endif  // QED_BASIC_H_
