#ifndef BASE_LOCK_H
#define BASE_LOCK_H

#include <mutex>

typedef std::mutex Lock;
typedef std::recursive_mutex RecursiveLock;
typedef std::lock_guard<Lock> ScopedLock;

#endif  // BASE_LOCK_H
