#include "base/stack_trace.h"

#include "base/logging.h"
#include "base/unittest.h"

TEST(StackTraceTest, Trace) {
  StackTrace trace;
  if (StackTrace::SupportedByPlatform()) {
    EXPECT_FALSE(trace.ToString().empty());
    LOG(INFO) << "Stack trace:\n" << trace.ToString();
  } else {
    EXPECT_TRUE(trace.ToString().empty());
  }
}
