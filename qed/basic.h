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

#define QED_SWITCH_BUCKETSIZE(size, t, ...)  \
switch(size) { \
  case 2 : {typedef qed::Bucket<2> t; __VA_ARGS__} break; \
  case 3 : {typedef qed::Bucket<3> t; __VA_ARGS__} break; \
  case 4 : {typedef qed::Bucket<4> t; __VA_ARGS__} break; \
  case 5 : {typedef qed::Bucket<5> t; __VA_ARGS__} break; \
  case 6 : {typedef qed::Bucket<6> t; __VA_ARGS__} break; \
  case 7 : {typedef qed::Bucket<7> t; __VA_ARGS__} break; \
  case 8 : {typedef qed::Bucket<8> t; __VA_ARGS__} break; \
  case 9 : {typedef qed::Bucket<9> t; __VA_ARGS__} break; \
  case 10 : {typedef qed::Bucket<10> t; __VA_ARGS__} break; \
  case 11 : {typedef qed::Bucket<11> t; __VA_ARGS__} break; \
  case 12 : {typedef qed::Bucket<12> t; __VA_ARGS__} break; \
  case 13 : {typedef qed::Bucket<13> t; __VA_ARGS__} break; \
  case 14 : {typedef qed::Bucket<14> t; __VA_ARGS__} break; \
  case 15 : {typedef qed::Bucket<15> t; __VA_ARGS__} break; \
  case 16 : {typedef qed::Bucket<16> t; __VA_ARGS__} break; \
  case 17 : {typedef qed::Bucket<17> t; __VA_ARGS__} break; \
  case 18 : {typedef qed::Bucket<18> t; __VA_ARGS__} break; \
  case 19 : {typedef qed::Bucket<19> t; __VA_ARGS__} break; \
  case 20 : {typedef qed::Bucket<20> t; __VA_ARGS__} break; \
  case 21 : {typedef qed::Bucket<21> t; __VA_ARGS__} break; \
  case 22 : {typedef qed::Bucket<22> t; __VA_ARGS__} break; \
  case 23 : {typedef qed::Bucket<23> t; __VA_ARGS__} break; \
  case 24 : {typedef qed::Bucket<24> t; __VA_ARGS__} break; \
  case 25 : {typedef qed::Bucket<25> t; __VA_ARGS__} break; \
  case 26 : {typedef qed::Bucket<26> t; __VA_ARGS__} break; \
  case 27 : {typedef qed::Bucket<27> t; __VA_ARGS__} break; \
  case 28 : {typedef qed::Bucket<28> t; __VA_ARGS__} break; \
  case 29 : {typedef qed::Bucket<29> t; __VA_ARGS__} break; \
  case 30 : {typedef qed::Bucket<30> t; __VA_ARGS__} break; \
  case 31 : {typedef qed::Bucket<31> t; __VA_ARGS__} break; \
  case 32 : {typedef qed::Bucket<32> t; __VA_ARGS__} break; \
  case 33 : {typedef qed::Bucket<33> t; __VA_ARGS__} break; \
  case 34 : {typedef qed::Bucket<34> t; __VA_ARGS__} break; \
  case 35 : {typedef qed::Bucket<35> t; __VA_ARGS__} break; \
  case 36 : {typedef qed::Bucket<36> t; __VA_ARGS__} break; \
  case 37 : {typedef qed::Bucket<37> t; __VA_ARGS__} break; \
  case 38 : {typedef qed::Bucket<38> t; __VA_ARGS__} break; \
  case 39 : {typedef qed::Bucket<39> t; __VA_ARGS__} break; \
  case 40 : {typedef qed::Bucket<40> t; __VA_ARGS__} break; \
  case 41 : {typedef qed::Bucket<41> t; __VA_ARGS__} break; \
  case 42 : {typedef qed::Bucket<42> t; __VA_ARGS__} break; \
  case 43 : {typedef qed::Bucket<43> t; __VA_ARGS__} break; \
  case 44 : {typedef qed::Bucket<44> t; __VA_ARGS__} break; \
  case 45 : {typedef qed::Bucket<45> t; __VA_ARGS__} break; \
  case 46 : {typedef qed::Bucket<46> t; __VA_ARGS__} break; \
  case 47 : {typedef qed::Bucket<47> t; __VA_ARGS__} break; \
  case 48 : {typedef qed::Bucket<48> t; __VA_ARGS__} break; \
  case 49 : {typedef qed::Bucket<49> t; __VA_ARGS__} break; \
  case 50 : {typedef qed::Bucket<50> t; __VA_ARGS__} break; \
  case 51 : {typedef qed::Bucket<51> t; __VA_ARGS__} break; \
  case 52 : {typedef qed::Bucket<52> t; __VA_ARGS__} break; \
  case 53 : {typedef qed::Bucket<53> t; __VA_ARGS__} break; \
  case 54 : {typedef qed::Bucket<54> t; __VA_ARGS__} break; \
  case 55 : {typedef qed::Bucket<55> t; __VA_ARGS__} break; \
  case 56 : {typedef qed::Bucket<56> t; __VA_ARGS__} break; \
  case 57 : {typedef qed::Bucket<57> t; __VA_ARGS__} break; \
  case 58 : {typedef qed::Bucket<58> t; __VA_ARGS__} break; \
  case 59 : {typedef qed::Bucket<59> t; __VA_ARGS__} break; \
  case 60 : {typedef qed::Bucket<60> t; __VA_ARGS__} break; \
  case 61 : {typedef qed::Bucket<61> t; __VA_ARGS__} break; \
  case 62 : {typedef qed::Bucket<62> t; __VA_ARGS__} break; \
  case 63 : {typedef qed::Bucket<63> t; __VA_ARGS__} break; \
  default : LOG(ERROR) << "unsupported size:" << size; \
            break;\
}
#ifndef NDEBUG
#define DEBUG_ONLY(...) {__VA_ARGS__}
#else
#define DEBUG_ONLY(...)
#endif

enum class DictValueType {
  unknown = -1,
  fp32 = 0,
  fp16 = 1
};

#define QED2_SWITCHTYPE_DictValueType(vt, t, ...)  \
switch(vt) { \
case qed::DictValueType::fp32: {typedef qed::fp32_t t; __VA_ARGS__} break; \
case qed::DictValueType::fp16: {typedef qed::fp16_t t; __VA_ARGS__} break; \
default : LOG(ERROR) << "unsupported value:" << \
     static_cast<std::underlying_type<qed::DictValueType>::type>(vt); break;\
}

template<typename T>
struct DictValueTypeFromType {
  static DictValueType valueType() {
    return DictValueType::unknown;
  }
};

template<>
struct DictValueTypeFromType<fp32_t> {
  static DictValueType valueType() {
    return DictValueType::fp32;
  }
};

template<>
struct DictValueTypeFromType<fp16_t> {
  static DictValueType valueType() {
    return DictValueType::fp16;
  }
};

}  // namespace qed

#endif  // QED_BASIC_H_
