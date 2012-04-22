#include "base/thread_checker.h"

ThreadChecker::ThreadChecker() : id_(std::this_thread::get_id()) {}

ThreadChecker::~ThreadChecker() {}

bool ThreadChecker::Check() const {
  return std::this_thread::get_id() == id_;
}
