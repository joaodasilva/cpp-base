#include "base/logging.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
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

// TODO: a more spiffy LogCallback that uses command line flags for filtering.
// TODO: cleanup file name (remove '..', etc)
void DefaultLogCallback(const LogMessage& message) {
  std::cerr << "[" << kSeverity[message.severity()] << " " << message.file()
            << ":" << message.line() << "] " << message.message() << std::endl;

  // TODO: have something that prints a stack trace instead.
  if (message.severity() == LOG_FATAL)
    std::abort();
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
