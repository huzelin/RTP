/*
 * \file common_defines.h 
 * \brief The common defines
 */
#ifndef COMMON_COMMON_DEFINES_H_
#define COMMON_COMMON_DEFINES_H_

#include <string.h>

#include <exception>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <sstream>

#ifndef DISABLE_COPY_AND_ASSIGN
#define DISABLE_COPY_AND_ASSIGN(classname)           \
    classname(const classname&) = delete;            \
    classname& operator=(const classname&) = delete
#endif

#define CONCATENATE_IMPL(s1, s2)  s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_IMPL(s1, s2)
#define ANONYMOUS_VARIABLE(str) CONCATENATE(str, __LINE__)

#define UNUSED __attribute__((__unused__))
#define USED __attribute__((__used__))

#define ALIGNED(x) __attribute__((aligned(x)))

#define AlignN(N, align) ((((N) + align - 1) / align) * align)

#define RTP_VERSION_MAJOR 2
#define RTP_VERSION_MINOR 0

template <typename T>
void SharedNoDestroy(T* p) { }

#endif  // COMMON_COMMON_DEFINES_H_
