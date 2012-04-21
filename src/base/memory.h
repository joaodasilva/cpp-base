#ifndef BASE_MEMORY_H
#define BASE_MEMORY_H

#include <memory>

using std::unique_ptr;

template<typename T>
inline unique_ptr<T> make_unique(T* ptr) {
  return unique_ptr<T>(ptr);
}

#endif  // BASE_MEMORY_H
