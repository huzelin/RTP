/*!
 * \file string_utils.h
 * \brief The string utility
 */
#ifndef COMMON_STRING_UTILS_H_
#define COMMON_STRING_UTILS_H_

#include <string>
#include <vector>
#include <sstream>
#include <cstdint>
#include <iomanip>
#include <map>

namespace common {

class StringUtils {
 public:
  static bool strToInt8(const char* str, int8_t& value);
  static bool strToUInt8(const char* str, uint8_t& value);
  static bool strToInt16(const char* str, int16_t& value);
  static bool strToUInt16(const char* str, uint16_t& value);
  static bool strToInt32(const char* str, int32_t& value);
  static bool strToUInt32(const char* str, uint32_t& value);
  static bool strToInt64(const char* str, int64_t& value);
  static bool strToUInt64(const char* str, uint64_t& value);
  static bool strToFloat(const char *str, float &value);
  static bool strToDouble(const char *str, double &value);    
  static std::vector<std::string> split(
      const std::string& text, 
      const std::string &sepStr, 
      bool ignoreEmpty = true);

  template<typename T>
  static std::string toString(const T &x);

  template<typename T>
  static std::string toString(
      const std::vector<T> &x, 
      const std::string &delim = " ");

  template<typename T>
  static std::string toString(
      const std::vector<std::vector<T> > &x, 
      const std::string &delim1, 
      const std::string &delim2);

  static std::string toString(const double &x, int32_t precision);    

  static bool GetValueFromMap(
      const std::map<std::string, std::string>& params, 
      const std::string& key,
      std::string* value);

 private:
  static std::stringstream* getStringStream();
  static void putStringStream(std::stringstream* ss);
  friend class StringStreamWrapper;
  class StringStreamWrapper {
   public:
    StringStreamWrapper(const std::string &str = "") 
      : _ss(StringUtils::getStringStream()) {_ss->str(str);}
    ~StringStreamWrapper() {StringUtils::putStringStream(_ss);}
    template<typename T>
    StringStreamWrapper& operator << (const T &x) {
      *_ss << x;
      return *this;
    }
    template<typename T>
    StringStreamWrapper& operator >> (T &x) {
      *_ss >> x;
      return *this;
    }
    std::string str() {return _ss->str();}
    bool eof() {return _ss->eof();}
   private:
    std::stringstream *_ss;
  };
};

template<typename T>
inline std::string StringUtils::toString(const T &x) {
  StringStreamWrapper oss;
  oss << x;
  return oss.str();    
}

template<> 
inline std::string StringUtils::toString<int8_t>(const int8_t &x) {
  char buf[8] = {0,};
  snprintf(buf, sizeof(buf), "%d", x);
  std::string res(buf);
  return res;
}

template<> 
inline std::string StringUtils::toString<uint8_t>(const uint8_t &x) {
  char buf[8] = {0,};
  snprintf(buf, sizeof(buf), "%u", x);
  std::string res(buf);
  return res;
}

template<> 
inline std::string StringUtils::toString<int16_t>(const int16_t &x) {
  char buf[16] = {0,};
  snprintf(buf, sizeof(buf), "%d", x);
  std::string res(buf);
  return res;
}

template<> 
inline std::string StringUtils::toString<uint16_t>(const uint16_t &x) {
  char buf[16] = {0,};
  snprintf(buf, sizeof(buf), "%u", x);
  std::string res(buf);
  return res;
}

template<> 
inline std::string StringUtils::toString<int32_t>(const int32_t &x) {
  char buf[32] = {0,};
  snprintf(buf, sizeof(buf), "%d", x);
  std::string res(buf);
  return res;
}

template<> 
inline std::string StringUtils::toString<uint32_t>(const uint32_t &x) {
  char buf[32] = {0,};
  snprintf(buf, sizeof(buf), "%u", x);
  std::string res(buf);
  return res;
}

template<> 
inline std::string StringUtils::toString<int64_t>(const int64_t &x) {
  char buf[64] = {0,};
  snprintf(buf, sizeof(buf), "%ld", x);
  std::string res(buf);
  return res;
}

template<> 
inline std::string StringUtils::toString<uint64_t>(const uint64_t &x) {
  char buf[64] = {0,};
  snprintf(buf, sizeof(buf), "%lu", x);
  std::string res(buf);
  return res;
}

template<> 
inline std::string StringUtils::toString<float>(const float &x) {
  StringStreamWrapper oss;
  oss << std::setprecision(6) << x;
  return oss.str();
}

template<> 
inline std::string StringUtils::toString<double>(const double &x) {
  StringStreamWrapper oss;
  oss << std::setprecision(15) << x;
  return oss.str();
}


inline std::string StringUtils::toString(const double &x, 
                                         int32_t precision) {
  StringStreamWrapper oss;
  oss << std::setprecision(precision) << x;
  return oss.str();
}

template<typename T>
inline std::string StringUtils::toString(
    const std::vector<T> &x, 
    const std::string &delim) {
  StringStreamWrapper oss; 
  for (typename std::vector<T>::const_iterator it = x.begin();
       it != x.end(); ++it)
  {
    if (it != x.begin()) oss << delim;
    oss << toString((*it));
  }
  return oss.str();
}

template<typename T>
inline std::string StringUtils::toString(
    const std::vector<std::vector<T> > &x, 
    const std::string &delim1,
    const std::string &delim2) {
  std::vector<std::string> strVec;
  for (typename std::vector<std::vector<T> >::const_iterator it = x.begin();
       it != x.end(); ++it)
  {
    strVec.push_back(toString(*it, delim1));
  }    
  return toString(strVec, delim2);
}

} // namespace common

#endif  // COMMON_STRING_UTILS_H_
