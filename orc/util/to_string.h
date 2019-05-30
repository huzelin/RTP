#ifndef ORC_UTIL_TO_STRING_H_
#define ORC_UTIL_TO_STRING_H_

#include <stdio.h>
#include <string>
#include <sstream>
#include <iterator>
#include <algorithm>

namespace orc {

#define TO_STRING(format, val) \
  do { \
    char buf[256]; \
    int32_t n = snprintf(buf, sizeof (buf), format, (val)); \
    return std::string(buf, n); \
  } while (0)

inline std::string ToString(const std::string& val) { return val; }

inline std::string ToString(int32_t val) { TO_STRING("%d", val); }
inline std::string ToString(uint32_t val) { TO_STRING("%u", val); }

inline std::string ToString(int64_t val) {
  TO_STRING("%lld", (long long int)val);
}

inline std::string ToString(uint64_t val) {
  TO_STRING("%llu", (unsigned long long int)val);
}

inline std::string ToString(float val) { TO_STRING("%f", val); }
inline std::string ToString(double val) { TO_STRING("%f", val); }
#undef TO_STRING

template<typename C>
std::string ToString(const C& c, const char* delimiter) {
  std::stringstream ss;
  std::ostream_iterator<typename C::value_type> out(ss, delimiter);
  std::copy(c.begin(), c.end(), out);
  std::string str = ss.str();
  return str.substr(0, str.size() - 1);  // -1 for tail delimiter
}

}  // namespace orc

#endif  // ORC_UTIL_TO_STRING_H_`
