#pragma once

#include <cstdlib>
#include <string>

#include "common/kafka/Topics.h"

namespace oms::util {

inline std::string bootstrapServers() {
  if (const char* env = std::getenv("KAFKA_BOOTSTRAP_SERVERS")) {
    if (env[0] != '\0') return env;
  }
  return kDefaultBrokers;
}

}
