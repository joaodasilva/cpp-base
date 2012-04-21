#include "base/stack_trace.h"

#if defined(__APPLE__)

#include <AvailabilityMacros.h>
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5
#define SUPPORTS_STACK_TRACE  1
#endif

#elif defined(__GLIBCXX__)

#define SUPPORTS_STACK_TRACE  1

#endif

#if defined(SUPPORTS_STACK_TRACE)
#include <cstdlib>
#include <cstring>
#include <memory>
#include <sstream>

#include <cxxabi.h>
#include <execinfo.h>

namespace {

// The prefix used for mangled symbols, per the Itanium C++ ABI:
// http://www.codesourcery.com/cxx-abi/abi.html#mangling
const char kMangledSymbolPrefix[] = "_Z";

const char kSymbolCharacters[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";

void Demangle(std::string* symbol) {
  size_t search_from = 0;
  while (search_from < symbol->size()) {
    size_t mangled_start = symbol->find(kMangledSymbolPrefix, search_from);
    if (mangled_start == std::string::npos)
      break;

    size_t mangled_end =
        symbol->find_first_not_of(kSymbolCharacters, mangled_start);
    if (mangled_end == std::string::npos)
      mangled_end = symbol->size();

    std::string mangled_symbol =
        symbol->substr(mangled_start, mangled_end - mangled_start);

    int status = 0;
    char* demangled = abi::__cxa_demangle(mangled_symbol.c_str(),
                                          NULL, 0, &status);
    if (status == 0) {
      // Demangling is successful.
      // Remove the mangled symbol.
      symbol->erase(mangled_start, mangled_end - mangled_start);
      // Insert the demangled symbol.
      symbol->insert(mangled_start, demangled);
      // Next time, start right after the demangled symbol just inserted.
      search_from = mangled_start + std::strlen(demangled);
    } else {
      // Failed to demangle.  Retry after the "_Z" just found.
      search_from = mangled_start + 2;
    }
    free(demangled);
  }
}

}  // namespace

StackTrace::StackTrace() {
  void* array[256];
  int size = backtrace(array, 256);

  char** symbols = backtrace_symbols(array, size);

  std::stringstream ss;
  for (int i = 0; i < size; i++) {
    std::string symbol(symbols[i]);
    Demangle(&symbol);
    ss << symbol << "\n";
  }
  trace_ = ss.str();

  free(symbols);
}

// static
bool StackTrace::SupportedByPlatform() {
  return true;
}

#else

StackTrace::StackTrace() {}

// static
bool StackTrace::SupportedByPlatform() {
  return false;
}

#endif  // SUPPORTS_STACK_TRACE
