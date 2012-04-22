#ifndef BASE_THREAD_CHECKER_H
#define BASE_THREAD_CHECKER_H

#include <thread>

#include "base/base.h"

class ThreadChecker {
 public:
  ThreadChecker();
  ~ThreadChecker();

  // Returns true if invoked on the same thread as the constructor.
  bool Check() const;

 private:
  std::thread::id id_;

  DISALLOW_COPY_AND_ASSIGN(ThreadChecker);
};

#endif  // BASE_THREAD_CHECKER_H
