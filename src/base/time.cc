#include "base/time.h"

namespace {

std::function<Time()> g_now;

}

Time Now() {
  if (g_now) {
    return g_now();
  } else {
    return std::chrono::steady_clock::now();
  }
}

void SetNowFunction(const std::function<Time()> now) {
  g_now = now;
}
