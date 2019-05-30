#ifndef ORC_UTIL_LOG_H_
#define ORC_UTIL_LOG_H_

#include <string>
#include <functional>
#include <cstdarg>

#include "orc/util/log_pub.h"

namespace orc {

void Log(LogLevel level, const char* file, int line, const char* func,
         const char* format, ...);

void Log(LogLevel level, const char* file, int line, const char* func,
         const char* format, va_list va);

}  // namespace orc

// Macros used in orc. Don't use these macros besides orc.
#define ORC_LOG_COMMON(level, format, ...) \
  if (orc::GetLogLevel() >= level) \
    orc::Log(level, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define ORC_FATAL(format, ...) \
    ORC_LOG_COMMON(orc::LogLevel::Fatal, format, ##__VA_ARGS__)

#define ORC_ERROR(format, ...) \
    ORC_LOG_COMMON(orc::LogLevel::Error, format, ##__VA_ARGS__)

#define ORC_WARN(format, ...) \
    ORC_LOG_COMMON(orc::LogLevel::Warn, format, ##__VA_ARGS__)

#define ORC_INFO(format, ...) \
    ORC_LOG_COMMON(orc::LogLevel::Info, format, ##__VA_ARGS__)

#define ORC_DEBUG(format, ...) \
    ORC_LOG_COMMON(orc::LogLevel::Debug, format, ##__VA_ARGS__)

#endif  // ORC_UTIL_LOG_H_
