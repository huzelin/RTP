#ifndef ORC_UTIL_STR_UTIL_H_
#define ORC_UTIL_STR_UTIL_H_

#include <ctype.h>

#include <algorithm>
#include <string>
#include <vector>
#include <type_traits>

#include "orc/util/string_piece.h"

namespace orc {
namespace str {

inline void LeftTrim(std::string* s) {
  s->erase(s->begin(), std::find_if(s->begin(), s->end(), [](int c){ return !isspace(c); }));
}

inline void RightTrim(std::string* s) {
  s->erase(std::find_if(s->rbegin(), s->rend(), [](int c){ return !isspace(c); }).base(),
           s->end());
}

inline void Trim(std::string* s) {
  LeftTrim(s);
  RightTrim(s);
}

template<typename S, typename D>
void Split(const S& s, const std::string& sep, std::vector<D>* vec,
           bool ignore_empty = false) {
  size_t b = 0;
  size_t e = 0;

  while ((e = s.find(sep, e)) != S::npos) {
    if ((b != e) || (!ignore_empty)) {
      vec->emplace_back(s.data() + b, e-b);
    }
    e += sep.size();
    b = e;
  }

  if (b < s.size()) {
    vec->emplace_back(s.data() + b, s.size() - b);
  } else if (!ignore_empty) {
    vec->emplace_back(D());
  }
}

bool ToFloat(const char* str, float* d);
bool ToDouble(const char* str, double* d);
bool ToInt32(const char* str, int32_t* d);
bool ToInt64(const char* str, int64_t* d);
bool ToUint32(const char* str, uint32_t* d);
bool ToUint64(const char* str, uint64_t* d);

bool ToFloat(const char* str, size_t len, float* d);
bool ToDouble(const char* str, size_t len, double* d);
bool ToInt32(const char* str, size_t len, int32_t* d);
bool ToInt64(const char* str, size_t len, int64_t* d);
bool ToUint32(const char* str, size_t len, uint32_t* d);
bool ToUint64(const char* str, size_t len, uint64_t* d);

template<typename S, typename T>
inline bool StringConvert(const S& s, T* d) {
  static_assert(std::is_same<S, T>::value,
                "1st template parameter's type must be same with 2st.");
  *d = s;
  return true;
}

template<typename S>
inline bool StringConvert(const S& s, S* d) {
  *d = s;
  return true;
}

template<typename S>
inline bool StringConvert(const S& s, bool* d)  {
  if (s == "true" || s == "1") {
      *d = true;
      return true;
  }

  if (s == "false" || s == "0") {
    *d = false;
    return true;
  }

  return false;
}

#define STR_CONV(SrcType, DstType, Convert) \
template<> \
inline bool StringConvert<SrcType, DstType>(const SrcType& s, DstType* d) { \
  return Convert(s.data(), s.size(), d); \
}

STR_CONV(std::string, float, ToFloat);
STR_CONV(std::string, double, ToDouble);
STR_CONV(std::string, int32_t, ToInt32);
STR_CONV(std::string, uint32_t, ToUint32);
STR_CONV(std::string, int64_t, ToInt64);
STR_CONV(std::string, uint64_t, ToUint64);

STR_CONV(StringPiece, float, ToFloat);
STR_CONV(StringPiece, double, ToDouble);
STR_CONV(StringPiece, int32_t, ToInt32);
STR_CONV(StringPiece, uint32_t, ToUint32);
STR_CONV(StringPiece, int64_t, ToInt64);
STR_CONV(StringPiece, uint64_t, ToUint64);

#undef STR_CONV

}  // namespace str
}  // namespace orc

#endif  // ORC_UTIL_STR_UTIL_H_
