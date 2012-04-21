#ifndef BASE_LOG_H
#define BASE_LOG_H

#include <functional>
#include <sstream>
#include <string>

#define LOG(severity) \
    ::logging::LogMessage(__FILE__, __LINE__, ::logging::LOG_ ## severity).stream()

// Same as LOG(severity), but appends the current errno value and message.
#define LOGE(severity) \
    ::logging::ErrnoLogMessage(__FILE__, __LINE__, ::logging::LOG_ ## severity).stream()

#define LOG_IF(severity, condition) \
    !(condition) ? (void) 0 : ::logging::LogMessageVoidify() & LOG(severity)

#define LOGE_IF(severity, condition) \
    !(condition) ? (void) 0 : ::logging::LogMessageVoidify() & LOGE(severity)

#define CHECK(condition)  LOG_IF(FATAL, !(condition))


#if defined(NDEBUG)

#define DLOG(severity)    LOG_IF(severity, false)
#define DLOGE(severity)   LOGE_IF(severity, false)
#define DCHECK(condition) LOG_IF(FATAL, false)

#else  // !NDEBUG

#define DLOG(severity)    LOG(severity)
#define DLOGE(severity)   LOGE(severity)
#define DCHECK(condition) LOG_IF(FATAL, !(condition))

#endif  // NDEBUG

#define NOTIMPLEMENTED()  DCHECK(false)
#define NOTREACHED()      DCHECK(false)

namespace logging {

// By default, INFO and higher are always logged. FATAL also aborts the program.
enum LogSeverity {
  LOG_VERBOSE = 0,
  LOG_DEBUG   = 1,
  LOG_INFO    = 2,
  LOG_WARNING = 3,
  LOG_ERROR   = 4,
  LOG_FATAL   = 5,
};

// Holds the contents of a log line.
class LogMessage {
 public:
  LogMessage(const char* file, size_t line, LogSeverity severity);
  ~LogMessage();

  // The underlying stream. Don't use this directly.
  std::ostream& stream() { return stream_; }

  std::string message() const { return stream_.str(); }
  const char* file() const { return file_; }
  size_t line() const { return line_; }
  LogSeverity severity() const { return severity_; }

 private:
  const char* file_;
  size_t line_;
  LogSeverity severity_;
  std::ostringstream stream_;
};

// Similar to LogMessage, but appends the errno value and message at the end
// of the log line.
class ErrnoLogMessage : public LogMessage {
 public:
  ErrnoLogMessage(const char* file, size_t line, LogSeverity severity);
  ~ErrnoLogMessage();
};
  
// This class is used to explicitly ignore values in the conditional
// logging macros.  This avoids compiler warnings like "value computed
// is not used" and "statement has no effect".
class LogMessageVoidify {
 public:
  LogMessageVoidify() { }
  // This has to be an operator with a precedence lower than << but
  // higher than ?:
  void operator&(std::ostream&) { }
};

// The callback type for SetLogCallback().
typedef std::function<void(const LogMessage& message)> LogCallback;

// Installs a custom LogMessage handler.
void SetLogCallback(const LogCallback& callback);

}  // namespace logging

#endif  // BASE_LOG_H
