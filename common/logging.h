/*!
 * \file logging.h
 * \brief The logging module
 */
#ifndef COMMON_LOGGING_H_
#define COMMON_LOGGING_H_

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <stdexcept>

#include <assert.h>
#include <iostream>
#include <sstream>
#include <ctime>

namespace common {

enum LogLevel {
  kLogDebug = 0,
  kLogInfo,
  kLogWarn,
  kLogError,
  kLogFatal,
};
/// init logging
extern void InitLogging(common::LogLevel log_level);

/// Date logger
class DateLogger {
 public:
  DateLogger() { }

  const char* HumanDate() {
    time_t time_value = time(NULL);
    struct tm *pnow;
    struct tm now;
    pnow = localtime_r(&time_value, &now);
    snprintf(buffer_, sizeof(buffer_), "%04d-%02d-%02d %02d:%02d:%02d",
             1900 + pnow->tm_year, 1 + pnow->tm_mon, pnow->tm_mday,
             pnow->tm_hour, pnow->tm_min, pnow->tm_sec);
    return buffer_;
  }

 private:
  char buffer_[32];
};

/*! \brief Return filename */ 
inline std::string FileName(const std::string &filepath) {
  return filepath; /// return full path
}

/*! \brief LogMessage */
class LogMessage {
 public:
  LogMessage(const char* file, int line, LogLevel level) {
    log_level_ = level;
    log_stream_ << "[" << pretty_date_.HumanDate() << "] ["
        << FileName(file) << ":" << line << "] ";
  }
  virtual ~LogMessage();
  std::stringstream& stream() { return log_stream_; }
  void RawLog(const char* format, ...);

 protected:
  std::stringstream log_stream_;
  DateLogger pretty_date_;
  LogLevel log_level_;

 private:
  LogMessage(const LogMessage&);
  void operator=(const LogMessage&);
};

/// LogMessageFatal
class LogMessageFatal : public LogMessage {
 public:
  LogMessageFatal(const char* file, int line, LogLevel level) : LogMessage(file, line, level) { }
  ~LogMessageFatal();

 private:
  LogMessageFatal(const LogMessageFatal&);
  void operator=(const LogMessageFatal&);
};

}  // namespace common

/// Always-on checking
#ifndef CHECK
#define CHECK(x)                                                                                \
    if (!(x))                                                                                   \
      common::LogMessageFatal(__FILE__, __LINE__, common::kLogFatal).stream() << "Check "       \
          << " failed: " #x << ' '
#endif

#ifndef CHECK_LT
#define CHECK_LT(x, y) CHECK((x) <  (y))
#endif

#ifndef CHECK_GT
#define CHECK_GT(x, y) CHECK((x) >  (y))
#endif

#ifndef CHECK_LE
#define CHECK_LE(x, y) CHECK((x) <= (y))
#endif

#ifndef CHECK_GE
#define CHECK_GE(x, y) CHECK((x) >= (y))
#endif

#ifndef CHECK_EQ
#define CHECK_EQ(x, y) CHECK((x) == (y))
#endif

#ifndef CHECK_NE
#define CHECK_NE(x, y) CHECK((x) != (y))
#endif

#ifndef CHECK_NOTNULL
#define CHECK_NOTNULL(x)                                                                   \
        ((x) == NULL ?                                                                     \
              common::LogMessageFatal(__FILE__, __LINE__, common::kLogFatal).stream()      \
              << "Check notnull: " #x << ' ', (x) : (x))
#endif

/// Debug-only checking
#ifdef NDEBUG
#ifndef DCHECK
#define DCHECK(x) while(false) CHECK(x)
#endif
#else
#ifndef DCHECK
#define DCHECK(x) CHECK(x)
#endif
#endif

#ifndef DCHECK_LT
#define DCHECK_LT(x, y) DCHECK((x) <  (y))
#endif

#ifndef DCHECK_GT
#define DCHECK_GT(x, y) DCHECK((x) >  (y))
#endif

#ifndef DCHECK_LE
#define DCHECK_LE(x, y) DCHECK((x) <= (y))
#endif

#ifndef DCHECK_GE
#define DCHECK_GE(x, y) DCHECK((x) >= (y))
#endif

#ifndef DCHECK_EQ
#define DCHECK_EQ(x, y) DCHECK((x) == (y))
#endif

#ifndef DCHECK_NE
#define DCHECK_NE(x, y) DCHECK((x) != (y))
#endif

#ifndef LOG_DEBUG
#define LOG_DEBUG    common::LogMessage(__FILE__, __LINE__, common::kLogDebug)
#endif

#ifndef LOG_INFO
#define LOG_INFO     common::LogMessage(__FILE__, __LINE__, common::kLogInfo)
#endif

#ifndef LOG_ERROR
#define LOG_ERROR    common::LogMessage(__FILE__, __LINE__, common::kLogError)
#endif

#ifndef LOG_WARNING
#define LOG_WARNING  common::LogMessage(__FILE__, __LINE__, common::kLogWarn)
#endif

#ifndef LOG_FATAL
#define LOG_FATAL    common::LogMessageFatal(__FILE__, __LINE__, common::kLogFatal)
#endif

#ifndef LOG
#define LOG(severity) LOG_##severity.stream()
#endif

#ifndef RAW_LOG
#define RAW_LOG(severity, fmt, ...) LOG_##severity.RawLog(fmt, ##__VA_ARGS__)
#endif

#ifdef NDEBUG
#ifndef DLOG
#define DLOG(severity) while (false) LOG(severity)
#endif
#else
#ifndef DLOG
#define DLOG(severity) LOG(severity)
#endif
#endif

#endif // COMMON_LOGGING_H_
