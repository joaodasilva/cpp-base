#ifndef BASE_BIND_H
#define BASE_BIND_H

#include <tuple>
#include <type_traits>
#include <utility>

#include "base/bind_internal.h"
#include "base/logging.h"
#include "base/traits.h"

// Applies a |tuple| to a |function|, with optional extra |args|.
template<typename Function, typename... TupleArgs, typename... Args>
inline
typename CallableTraits<Function>::return_type
Apply(
    const Function& function,
    std::tuple<TupleArgs...>&& tuple,
    Args&&... args) {
  return internal::UnpackTuple<sizeof...(TupleArgs)>::unpack(
      function,
      std::forward<std::tuple<TupleArgs...>>(tuple),
      std::forward<Args>(args)...);
}

// The type returned by Bind().
//
// Callbacks can be copied and used concurrently. Copies are cheap, since they
// only copy a pointer to shared storage.
template<typename Function, typename... BoundArgs>
class Callback {
 public:
  template<typename InFunction, typename... InBoundArgs>
  explicit Callback(InFunction&& function, InBoundArgs&&... bound_args)
      : shared_storage_(new SharedStorage(
            std::forward<InFunction>(function),
            std::forward<InBoundArgs>(bound_args)...)) {}

  Callback(const Callback& callback)
      : shared_storage_(callback.shared_storage_) {
    if (shared_storage_)
      shared_storage_->Ref();
  }

  Callback(Callback& callback)
      : shared_storage_(callback.shared_storage_) {
    if (shared_storage_)
      shared_storage_->Ref();
  }

  Callback(Callback&& callback)
      : shared_storage_(callback.shared_storage_) {
    callback.shared_storage_ = NULL;
  }

  ~Callback() {
    if (shared_storage_)
      shared_storage_->Unref();
  }

  void Reset() {
    if (shared_storage_)
      shared_storage_->Unref();
    shared_storage_ = NULL;
  }

  explicit operator bool() const {
    return shared_storage_ != NULL;
  }

  template<typename... Args>
  typename CallableTraits<Function>::return_type
  operator()(Args&&... args) const {
    DCHECK(shared_storage_);
    return Apply(
        shared_storage_->function_,
        std::forward<std::tuple<BoundArgs...>>(shared_storage_->bound_args_),
        std::forward<Args>(args)...);
  }

 private:
  struct SharedStorage {
    template<typename InFunction, typename... InBoundArgs>
    SharedStorage(InFunction&& function, InBoundArgs&&... bound_args)
        : ref_count_(1),
          function_(std::forward<InFunction>(function)),
          bound_args_(std::forward<InBoundArgs>(bound_args)...) {}

    void Ref() {
      ScopedLock scoped_lock(lock_);
      ref_count_++;
    }

    void Unref() {
      bool was_last = false;
      {
        ScopedLock scoped_lock(lock_);
        ref_count_--;
        was_last = ref_count_ == 0;
      }
      if (was_last)
        delete this;
    }

    Lock lock_;
    size_t ref_count_;
    Function function_;
    std::tuple<BoundArgs...> bound_args_;
  };

  SharedStorage* shared_storage_;
};

// Binds the given arguments to the given callable.
//
// The |function| can be a function, method or std::function.
//
// The arguments are bound in the declaration order of the callable. Not every
// argument has to be bound; missing arguments can be added at invocation time.
// For methods, the first argument is the object.
template<typename Function, typename... BoundArgs>
inline
Callback<
    typename std::decay<Function>::type,
    typename std::decay<BoundArgs>::type...>
Bind(Function&& function, BoundArgs&&... bound_args) {
  typedef Callback<
      typename std::decay<Function>::type,
      typename std::decay<BoundArgs>::type...> CallbackType;
  return CallbackType(std::forward<Function>(function),
                      std::forward<BoundArgs>(bound_args)...);
}

#endif  // BASE_BIND_H
