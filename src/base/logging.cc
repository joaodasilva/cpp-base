#include "base/logging.h"

#include "base/stack_trace.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>

namespace logging {

namespace {

LogCallback log_callback;

const char* kSeverity[] = {
  "VERBOSE",
  "DEBUG",
  "INFO",
  "WARNING",
  "ERROR",
  "FATAL",
};

const char* CleanPath(const char* path) {
  // Skip leading '..' in the path.
  while (std::strncmp(path, "../", 3) == 0)
    path += 3;
  return path;
}

// TODO: a more spiffy LogCallback that uses command line flags for filtering.
void DefaultLogCallback(const LogMessage& message) {
  std::cerr << "[" << kSeverity[message.severity()] << " "
            << CleanPath(message.file()) << ":" << message.line() << "] "
            << message.message() << std::endl;

  if (message.severity() == LOG_FATAL) {
    if (StackTrace::SupportedByPlatform())
      std::cerr << "Stack trace:\n" << StackTrace().ToString() << std::endl;
    std::abort();
  }
}

}  // namespace

LogMessage::LogMessage(const char* file, size_t line, LogSeverity severity)
    : file_(file),
      line_(line),
      severity_(severity) {}

LogMessage::~LogMessage() {
  if (!log_callback)
    log_callback = std::bind(DefaultLogCallback, std::placeholders::_1);
  log_callback(*this);
}

ErrnoLogMessage::ErrnoLogMessage(
    const char* file, size_t line, LogSeverity severity)
    : LogMessage(file, line, severity) {}

ErrnoLogMessage::~ErrnoLogMessage() {
  auto err = errno;
  stream() << " (errno " << err << ": " << std::strerror(err) << ")";
}

void SetLogCallback(const LogCallback& callback) {
  log_callback = callback;
}

}  // namespace logging
