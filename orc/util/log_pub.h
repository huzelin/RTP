#ifndef ORC_UTIL_LOG_PUB_H__
#define ORC_UTIL_LOG_PUB_H__

#include <string>
#include <functional>
#include <cstdarg>

namespace orc {

enum class LogLevel { Fatal, Error, Warn, Info, Debug };

using LogWriter = std::function<void(LogLevel, const char*, int, const char*,
                                     const char*, va_list)>;

void SetLogWriter(LogWriter log_writer);

void SetLogLevel(LogLevel level);
void SetLogLevel(const std::string& level);
LogLevel GetLogLevel();

std::string ToString(LogLevel level);
LogLevel ToLogLevel(const std::string& level);

void SetLogMaxLength(uint32_t max_length);
uint32_t GetLogMaxLength();

}  // namespace orc

#endif  // ORC_UTIL_LOG_PUB_H__
