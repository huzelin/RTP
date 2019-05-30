#ifndef ORC_UTIL_MACROS_H__
#define ORC_UTIL_MACROS_H__


#define ORC_DISALLOW_COPY_AND_ASSIGN(Clazz) \
  Clazz(const Clazz&) = delete; \
  Clazz& operator=(const Clazz&) = delete; \
  Clazz(Clazz&&) = delete

#define ORC_CACHE_LINE_SIZE 64

#endif  // ORC_UTIL_MACROS_H__
