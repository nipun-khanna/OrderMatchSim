#pragma once

#include <chrono>
#include <cstdint>

namespace oms::util {

inline int64_t nowMicros() {
  return std::chrono::duration_cast<std::chrono::microseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

}
