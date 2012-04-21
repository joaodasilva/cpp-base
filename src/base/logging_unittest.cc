#include "base/logging.h"

#include <fcntl.h>

#include "base/unittest.h"

TEST(LoggingTest, Logs) {
#define TEST_LOG(LOG_F) \
    LOG_F(VERBOSE) << "Verbose log"; \
    LOG_F(DEBUG) << "Debug log"; \
    LOG_F(INFO) << "Info log"; \
    LOG_F(WARNING) << "Warning log"; \
    LOG_F(ERROR) << "Error log";

  TEST_LOG(LOG);
  EXPECT_DEATH({ LOG(FATAL) << "Fatal log"; }, "Fatal log");

  ASSERT_EQ(-1, open("\0/etc/passwd", O_RDONLY));
  TEST_LOG(LOGE);
  EXPECT_DEATH({ LOGE(FATAL) << "Fatal log"; }, "Fatal log \\(errno");

#if !defined(NDEBUG)
  TEST_LOG(DLOG);
  EXPECT_DEATH({ DLOG(FATAL) << "Fatal log"; }, "Fatal log");

  TEST_LOG(DLOGE);
  EXPECT_DEATH({ DLOGE(FATAL) << "Fatal log"; }, "Fatal log \\(errno");
#endif

#undef TEST_LOG
}

TEST(LoggingTest, Checks) {
  EXPECT_DEATH({ CHECK(false) << "Condition failed"; }, "Condition failed");

#if !defined(NDEBUG)
  EXPECT_DEATH({ DCHECK(false) << "Condition failed"; }, "Condition failed");
  EXPECT_DEATH({ NOTREACHED() << "Condition failed"; }, "Condition failed");
  EXPECT_DEATH({ NOTIMPLEMENTED() << "Condition failed"; }, "Condition failed");
#endif
}
