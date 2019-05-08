/*!
 * \file logging.cc
 * \brief The logging module
 */
#include "common/logging.h"

#include <stdarg.h>
#include <string.h>
#include <signal.h>

namespace {
const char* kLogLevelString[] = {
  "[DEBUG]",
  "[INFO]",
  "[WARN]",
  "[ERROR]",
  "[FATAL]",
};
FILE* fd = stdout; 
const int kBufferSize = 4096;
common::LogLevel g_log_level = common::kLogInfo;
}  // namespace

namespace common {

void InitLogging(common::LogLevel log_level) {
  fd = fopen("rtp.log", "w");

  if (fd == NULL) {
    fprintf(stderr, "open rtp.log failed");
    abort();
  }
  g_log_level = log_level;
}

LogMessage::~LogMessage() {
  /// Only loglevel >= g_log_level be logged!
  if (log_level_ >= g_log_level) {
    fprintf(fd, "%s", kLogLevelString[log_level_]);
    fprintf(fd, " %s\n", log_stream_.str().c_str());
    fflush(fd);
  }
}

void LogMessage::RawLog(const char* format, ...) {
  va_list ap;
  std::string msg;
  va_start(ap, format);
  char buffer[kBufferSize];
  vsnprintf(buffer, kBufferSize, format, ap);
  va_end(ap);

  log_stream_ << buffer;
}

LogMessageFatal::~LogMessageFatal() {
  fprintf(fd, "%s", kLogLevelString[log_level_]);
  fprintf(fd, " %s\n", log_stream_.str().c_str());
  abort();
}

}  // namespace common
