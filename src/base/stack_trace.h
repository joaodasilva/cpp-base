#ifndef BASE_STACK_TRACE_H
#define BASE_STACK_TRACE_H

#include <string>

// Stores a snapshot of the stack when constructed, if supported by the
// platform.
class StackTrace {
 public:
  StackTrace();
  ~StackTrace() {}

  static bool SupportedByPlatform();

  // The returned string is owned by |this|.
  const std::string& ToString() const { return trace_; }

 private:
  std::string trace_;
};

#endif
