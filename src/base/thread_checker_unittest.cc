#include "base/thread_checker.h"

#include "base/unittest.h"

TEST(ThreadChecker, SameID) {
  ThreadChecker checker;
  EXPECT_TRUE(checker.Check());
}

void CheckFalse(const ThreadChecker& checker, bool* checked, bool* result) {
  *checked = true;
  *result = checker.Check();
}

TEST(ThreadChecker, DifferentID) {
  ThreadChecker checker;
  EXPECT_TRUE(checker.Check());

  bool checked = false;
  bool result = true;
  std::thread other(bind(CheckFalse, std::cref(checker), &checked, &result));
  EXPECT_TRUE(other.joinable());
  other.join();
  EXPECT_TRUE(checked);
  EXPECT_FALSE(result);
}
