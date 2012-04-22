#ifndef BASE_TRAITS_H
#define BASE_TRAITS_H

template<typename T>
struct TypeTraits {
  typedef T Type;
  typedef T* Ptr;

  static const bool IsArray = false;

  static void Delete(Ptr ptr) { delete ptr; }
};

template<typename T>
struct TypeTraits<T[]> {
  typedef T Type;
  typedef T* Ptr;

  static const bool IsArray = true;

  static void Delete(Ptr ptr) { delete[] ptr; }
};


// Used to extract information about a callable type.
template<typename Sig>
struct CallableTraits;

// Specialization for methods.
template<typename Return,
         typename T,
         typename... Args>
struct CallableTraits<Return(T::*)(Args...)> {
  typedef Return return_type;
};

// Specialization for const methods.
template<typename Return,
         typename T,
         typename... Args>
struct CallableTraits<Return(T::*)(Args...) const> {
  typedef Return return_type;
};

// Specialization for functions.
template<typename Return,
         typename... Args>
struct CallableTraits<Return(Args...)> {
  typedef Return return_type;
};

// Specialization for functions.
template<typename Return,
         typename... Args>
struct CallableTraits<Return(*)(Args...)> {
  typedef Return return_type;
};

// Default: the callable type must typedef its result_type. This is used for
// the result of std::bind() and std::function<>.
template<typename HasResultType>
struct CallableTraits {
  typedef typename HasResultType::result_type return_type;
};

#endif  // BASE_TRAITS_H
