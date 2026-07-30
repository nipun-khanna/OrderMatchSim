#pragma once

#include <atomic>
#include <string>

#include "common/utils/Time.h"

namespace oms::util {

inline std::string newId(const std::string& prefix) {
  static std::atomic<uint64_t> counter{0};
  return prefix + "-" + std::to_string(nowMicros()) + "-" +
         std::to_string(counter.fetch_add(1, std::memory_order_relaxed));
}

} 
