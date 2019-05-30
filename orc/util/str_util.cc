#include "orc/util/str_util.h"

#include <string.h>
#include <limits.h>
#include <errno.h>


namespace orc {
namespace str {

static const size_t kMaxNumberLength = 32;

bool ToFloat(const char* str, float* d) {
  return ToFloat(str, strlen(str), d);
}

bool ToFloat(const char* str, size_t len, float* d) {
  char* end = nullptr;
  errno = 0;
  float r = strtof(str, &end);
  if (errno != 0 || (str+len) != end) {
    return false;
  }
  *d = r;
  return true;
}

bool ToDouble(const char* str, double* d) {
  return ToDouble(str, strlen(str), d);
}

bool ToDouble(const char* str, size_t len, double* d) {
  char* end = nullptr;
  errno = 0;
  double r = strtod(str, &end);
  if (errno != 0 || (str+len) != end) {
    return false;
  }
  *d = r;
  return true;
}

bool ToInt32(const char* str, int32_t* d) {
  return ToInt32(str, strlen(str), d);
}

bool ToInt32(const char* str, size_t len, int32_t* d) {
  char* end = nullptr;
  errno = 0;
  int64_t r = strtol(str, &end, 10);
  if (errno != 0 || (str+len) != end) {
    return false;
  }
  if (r > UINT_MAX || r < INT_MIN) {
    return false;
  }
  *d = r;
  return true;
}

bool ToUint32(const char* str, uint32_t* d) {
  return ToUint32(str, strlen(str), d);
}

bool ToUint32(const char* str, size_t len, uint32_t* d) {
  char* end = nullptr;
  errno = 0;
  int64_t r = strtol(str, &end, 10);
  if (errno != 0 || (str+len) != end) {
    return false;
  }
  if (r < 0 || r > UINT_MAX) {
    return false;
  }
  *d = r;
  return true;
}

bool ToInt64(const char* str, int64_t* d) {
  return ToInt64(str, strlen(str), d);
}

bool ToInt64(const char* str, size_t len, int64_t* d) {
  char* end = nullptr;
  errno = 0;
  int64_t r = strtol(str, &end, 10);
  if (errno != 0 || (str+len) != end) {
    return false;
  }
  *d = r;
  return true;
}

bool ToUint64(const char* str, uint64_t* d) {
  return ToUint64(str, strlen(str), d);
}

bool ToUint64(const char* str, size_t len, uint64_t* d) {
  char* end = nullptr;
  errno = 0;
  uint64_t r = strtoul(str, &end, 10);
  if (errno != 0 || (str+len) != end) {
    return false;
  }
  *d = r;
  return true;
}

}  // namespace str
}  // namespace orc
