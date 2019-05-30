#include "orc/util/log.h"

#include <time.h>
#include <sys/time.h>

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

#include <memory>

#include "orc/util/utils.h"

namespace orc {

static void DefaultLogWriter(LogLevel level, const char* file, int line,
                             const char* func, const char* format, va_list va);

static LogLevel g_log_level = LogLevel::Info;
static LogWriter g_log_writer = DefaultLogWriter;

static uint32_t g_log_max_length = 4096;

static void DefaultLogWriter(LogLevel level, const char* file, int line,
                             const char* func, const char* format, va_list va) {
  std::unique_ptr<char[]> delete_guard{new char[g_log_max_length]};
  char* buf = delete_guard.get();

  struct timeval tv;
  gettimeofday(&tv, NULL);
  struct tm t;
  localtime_r(&tv.tv_sec, &t);

  size_t size = g_log_max_length;

  int len = snprintf(buf, size,
                     "[%04d-%02d-%02d %02d:%02d:%02d.%03d] [%s] [%d] [%s:%d %s] ",
                     t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour,
                     t.tm_min, t.tm_sec, static_cast<int>(tv.tv_usec)/1000,
                     ToString(level).c_str(),
                     tid(),
                     file, line, func);

  if (len >= static_cast<int>(size) || len < 0) {
    buf[size-1] = '\n';
    write(2, buf, size);
    return;
  }

  int len2 = vsnprintf(buf+len, size-len, format, va);
  if (len2 >= static_cast<int>(size-len) || len2 < 0) {
    buf[size-1] = '\n';
    write(2, buf, size);
    return;
  }

  buf[len+len2] = '\n';
  write(2, buf, len+len2+1);
}

void SetLogWriter(LogWriter log_writer) { g_log_writer = log_writer; }

void SetLogLevel(LogLevel level) { g_log_level = level; }

void SetLogLevel(const std::string& level) {
  SetLogLevel(ToLogLevel(level));
}

LogLevel GetLogLevel() { return g_log_level; }

std::string ToString(LogLevel level) {
  switch (level) {
    case LogLevel::Fatal: return "FATAL";
    case LogLevel::Error: return "ERROR";
    case LogLevel::Warn:  return "WARN";
    case LogLevel::Info:  return "INFO";
    case LogLevel::Debug: return "DEBUG";
    default: return "UNKNOWN";
  }
}

LogLevel ToLogLevel(const std::string& level) {
  if (level == "fatal" || level == "FATAL") return LogLevel::Fatal;
  if (level == "error" || level == "ERROR") return LogLevel::Error;
  if (level == "warn" || level == "WARN") return LogLevel::Warn;
  if (level == "info" || level == "INFO") return LogLevel::Info;
  if (level == "debug" || level == "DEBUG") return LogLevel::Debug;

  return LogLevel::Info;
}

void SetLogMaxLength(uint32_t length) { g_log_max_length = length; }
uint32_t GetLogMaxLength() { return g_log_max_length; }

void Log(LogLevel level, const char* file, int line, const char* func,
         const char* format, ...) {
  va_list va;
  va_start(va, format);
  Log(level, file, line, func, format, va);
  va_end(va);
}

void Log(LogLevel level, const char* file, int line, const char* func,
         const char* format, va_list va) {
  g_log_writer(level, file, line, func, format, va);
}

}  // namespace orc
