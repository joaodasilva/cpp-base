#ifndef BASE_TIME_H
#define BASE_TIME_H

#include <chrono>
#include <functional>

#if !defined(__clang__) && defined(__GNUC__) && \
    (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 7))
#define steady_clock monotonic_clock
#endif

typedef std::chrono::time_point<std::chrono::steady_clock> Time;
typedef std::chrono::duration<int, std::milli> TimeDelta;

template<typename Diff>
inline TimeDelta ToTimeDelta(Diff d) {
  return std::chrono::duration_cast<TimeDelta>(d);
}

Time Now();

void SetNowFunction(const std::function<Time()> now);

#endif  // BASE_TIME_H
