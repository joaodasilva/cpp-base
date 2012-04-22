#ifndef BASE_WEAK_H
#define BASE_WEAK_H

#include "base/base.h"
#include "base/lock.h"
#include "base/logging.h"
#include "base/memory.h"
#include "base/thread_checker.h"

template<typename T>
class ScopedWeakPtrFactory;

template<typename T>
class Weakling;

template<typename T>
class WeakPtr;

// A WeakFlag keeps track of a boolean flag that can be invalidated. WeakFlags
// can be copied and created sharing the same flag; once the flag is
// invalidated, all the WeakFlags will become invalidated too.
// WeakFlags can be moved and copied across threads, but should only be tested
// and invalidated from the owning thread.
class WeakFlag {
 public:
  explicit WeakFlag() : shared_(new Shared(true)) {}

  WeakFlag(const WeakFlag& other)
      : shared_(other.shared_) {
    ScopedLock lock(shared_->lock);
    shared_->ref_count++;
  }

  ~WeakFlag() {
    Unref(NULL);
  }

  WeakFlag& operator=(const WeakFlag& other) {
    // |other| might be |this|; bump the |ref_count| first.
    {
      ScopedLock lock(other.shared_->lock);
      other.shared_->ref_count++;
    }
    Unref(other.shared_);
    return *this;
  }

  bool IsValid() const {
    ScopedLock lock(shared_->lock);
#if !defined(NDEBUG)
    DCHECK(shared_->checker.Check());
#endif
    return shared_->valid;
  }

  explicit operator bool() const { return IsValid(); }

  // Returns true if there's at least another WeakFlag sharing the validation
  // flag with this WeakFlag.
  bool IsSharing() const {
    ScopedLock lock(shared_->lock);
#if !defined(NDEBUG)
    DCHECK(shared_->checker.Check());
#endif
    return shared_->valid && shared_->ref_count > 1;
  }

  // Invalidates all WeakFlags shared with this.
  void InvalidateAll() {
    ScopedLock lock(shared_->lock);
#if !defined(NDEBUG)
    DCHECK(shared_->checker.Check());
#endif
    shared_->valid = false;
  }

  // Invalidates only this WeakFlag, but not the other WeakFlags sharing the
  // same flag.
  void Reset() {
    Unref(new Shared(false));
  }

 private:
  // |Shared| is the shared state. A WeakFlag always has a pointer to such a
  // struct. Copies of a WeakFlag bump the |ref_count|, and decrease it when
  // destroyed. The last copy releases the struct. Access to this struct must
  // own |lock|.
  struct Shared {
    explicit Shared(bool is_valid) : ref_count(1), valid(is_valid) {}
#if !defined(NDEBUG)
    ThreadChecker checker;
#endif
    Lock lock;
    int ref_count;
    bool valid;
  };

  void Unref(Shared* new_shared) {
    Shared* destroy_shared = NULL;
    {
      ScopedLock lock(shared_->lock);
      shared_->ref_count--;
      if (shared_->ref_count == 0)
        destroy_shared = shared_;
      shared_ = new_shared;
    }
    if (destroy_shared)
      delete destroy_shared;
  }

  Shared* shared_;
};

// A smart pointer that automatically invalidates itself when its WeakFlag is
// invalidated. Has the same threading behavior as WeakFlag.
template<typename T>
class WeakPtr {
 public:
  WeakPtr() : ptr_(NULL) {}

  template<typename U>
  WeakPtr(const WeakPtr<U>& other) : flag_(other.flag_), ptr_(other.ptr_) {}

  template<typename U>
  WeakPtr<T>& operator=(const WeakPtr<U>& other) {
    flag_ = other.flag_;
    ptr_ = other.ptr_;
    return *this;
  }

  const WeakFlag& GetWeakFlag() const {
    return flag_;
  }

  T* get() const {
    return flag_ ? ptr_ : NULL;
  }

  operator T*() const { return get(); }

  T& operator*() const {
    DCHECK(get());
    return *get();
  }

  T* operator->() const {
    DCHECK(get());
    return get();
  }

  // Invalidates this |WeakPtr| but not other WeakPtrs shared with this one.
  void Reset() {
    flag_.Reset();
    ptr_ = NULL;
  }

 private:
  template<typename U>
  friend class WeakPtr;
  friend class Weakling<T>;
  friend class ScopedWeakPtrFactory<T>;

  WeakPtr(const WeakFlag& flag, T* ptr)
      : flag_(flag), ptr_(ptr) {}

  WeakFlag flag_;
  T* ptr_;
};

// A Weakling class can create WeakPtrs to itself and invalidate them. The
// pointers are also automatically invalidated when the instance is destroyed.
// Weaklings also have the WeakMethod() call, which returns a wrapper to one
// of the class methods that only forwards the call to the method if a weak
// pointer created at the time of the invocation of WeakMethod() is still valid.
// Example:
//
// class Foo : public Weakling<Foo> {
//   void Bar() {
//     loop_->Post(Bind(&Foo::OnLongThingDone, GetWeakPtr()));
//   }
//
//   void OnLongThingDone(Result* result) {
//     ...
//   }
// };
//
// If the task posted to |loop_| executes after the Foo instance is destroyed,
// then it won't forward the call to the weak method.
template<typename T>
class Weakling {
 protected:
  Weakling() {}

  virtual ~Weakling() {
    InvalidateAll();
  }

  // Invalidates all currently existing WeakPtrs. WeakPtrs created after this
  // call will be valid again, and will become bound to the current thread then.
  void InvalidateAll() {
    if (flag_) {
      flag_->InvalidateAll();
      flag_.reset();
    }
  }

 public:
  WeakPtr<T> GetWeakPtr() const {
    if (!flag_)
      flag_.reset(new WeakFlag);
    return WeakPtr<T>(*flag_, const_cast<T*>(static_cast<const T*>(this)));
  }

  bool HasWeakPtrs() const {
    return flag_ && flag_->IsSharing();
  }

 private:
  mutable unique_ptr<WeakFlag> flag_;

  DISALLOW_COPY_AND_ASSIGN(Weakling);
};

// Similar to Weakling<T>, but allows several factories per object with separate
// scopes.
template<typename T>
class ScopedWeakPtrFactory {
 public:
  explicit ScopedWeakPtrFactory(T* object) : ptr_(object) {}

  ~ScopedWeakPtrFactory() {
    InvalidateAll();
  }

  // Invalidates all currently existing WeakPtrs. WeakPtrs created after this
  // call will be valid again, and will become bound to the current thread then.
  void InvalidateAll() {
    if (flag_) {
      flag_->InvalidateAll();
      flag_.reset();
    }
  }

  WeakPtr<T> GetWeakPtr() const {
    if (!flag_)
      flag_.reset(new WeakFlag);
    return WeakPtr<T>(*flag_, ptr_);
  }

  bool HasWeakPtrs() const {
    return flag_ && flag_->IsSharing();
  }

 private:
  mutable unique_ptr<WeakFlag> flag_;
  T* ptr_;

 DISALLOW_COPY_AND_ASSIGN(ScopedWeakPtrFactory);
};

#endif  // BASE_WEAK_H
