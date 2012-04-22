#ifndef BASE_BIND_INTERNAL_H
#define BASE_BIND_INTERNAL_H

#include <tuple>

#include "base/traits.h"
#include "base/weak.h"

// These are helpers for Bind() and Apply(), and aren't meant to be used
// directly.
namespace internal {

// Removes content from an std::tuple and passes it as arguments to a function.
template<std::size_t N>
struct UnpackTuple {
  template<typename Function, typename... TupleArgs, typename... Args>
  inline static typename CallableTraits<Function>::return_type
  unpack(
      const Function& function,
      const std::tuple<TupleArgs...>& tuple,
      const Args&... args) {
    return UnpackTuple<N-1>::unpack(
        function,
        tuple,
        std::get<N-1>(tuple),
        args...);
  }
};

// Base case for tuples whose content has been completely pumped into the
// arguments.
template<>
struct UnpackTuple<0> {
  // Special case for methods.
  // The Pointer& type is such that it supports smart pointers such as
  // std::unique_ptr too.
  template<
      typename T,
      typename... MethodArgs,
      typename Return,
      typename... TupleArgs,
      typename Pointer,
      typename... Args>
  inline static Return
  unpack(
      Return (T::*method)(MethodArgs...),
      const std::tuple<TupleArgs...>& tuple,
      Pointer& object,
      const Args&... args) {
    return ((*object).*method)(args...);
  }

  // Special case for const methods.
  template<
      typename T,
      typename... MethodArgs,
      typename Return,
      typename... TupleArgs,
      typename Pointer,
      typename... Args>
  inline static Return
  unpack(
      Return (T::*method)(MethodArgs...) const,
      const std::tuple<TupleArgs...>& tuple,
      Pointer& object,
      const Args&... args) {
    return ((*object).*method)(args...);
  }

  // Special case for methods of WeakPtrs.
  template<
      typename T,
      typename... MethodArgs,
      typename... TupleArgs,
      typename... Args>
  inline static void
  unpack(
      void (T::*method)(MethodArgs...),
      const std::tuple<TupleArgs...>& tuple,
      const WeakPtr<T>& weak_ptr,
      const Args&... args) {
    if (weak_ptr)
      return ((*weak_ptr).*method)(args...);
  }

  // Special case for const methods of WeakPtrs.
  template<
      typename T,
      typename... MethodArgs,
      typename... TupleArgs,
      typename... Args>
  inline static void
  unpack(
      void (T::*method)(MethodArgs...) const,
      const std::tuple<TupleArgs...>& tuple,
      const WeakPtr<T>& weak_ptr,
      const Args&... args) {
    if (weak_ptr)
      return ((*weak_ptr).*method)(args...);
  }

  // Generic case.
  template<typename Function, typename... TupleArgs, typename... Args>
  inline static typename CallableTraits<Function>::return_type
  unpack(
      const Function& function,
      const std::tuple<TupleArgs...>& tuple,
      const Args&... args) {
    return function(args...);
  }
};

}  // namespace internal

#endif  // BASE_BIND_INTERNAL_H
