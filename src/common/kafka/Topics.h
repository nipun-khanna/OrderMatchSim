#pragma once

#include <string>

namespace oms {

namespace topic {
inline constexpr char ORDERS[] = "orders";
inline constexpr char TRADES[] = "trades";
}

// Default broker address for the local docker-compose Kafka (HOST listener)
inline constexpr char kDefaultBrokers[] = "localhost:9092";

}
