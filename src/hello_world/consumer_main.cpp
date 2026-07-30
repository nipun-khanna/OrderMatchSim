// Phase 1 hello-world consumer: subscribes to the `orders` topic, waits for one
// OrderCreated message, prints it, and exits. Pairs with hello_producer.

#include <chrono>
#include <cstdlib>

#include <spdlog/spdlog.h>

#include "common/kafka/Consumer.h"
#include "common/kafka/Topics.h"
#include "common/utils/Env.h"
#include "order_match.pb.h"

namespace {
constexpr int kMaxPollAttempts = 30;  // ~30s total at 1s per poll
}

int main() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  const std::string brokers = oms::util::bootstrapServers();

  try {
    oms::Consumer consumer(brokers, "hello-consumer", {oms::topic::ORDERS},
                           /*fromBeginning=*/true);
    spdlog::info("Waiting for an OrderCreated on '{}' [{}]...",
                 oms::topic::ORDERS, brokers);

    for (int attempt = 0; attempt < kMaxPollAttempts; ++attempt) {
      const auto records = consumer.poll(std::chrono::milliseconds(1000));
      for (const auto& record : records) {
        ordermatch::OrderCreated order;
        if (!order.ParseFromString(record.payload)) {
          spdlog::warn("Skipping record with un-parseable payload (key={})",
                       record.key);
          continue;
        }
        spdlog::info(
            "Received OrderCreated (key={}): id={} symbol={} side={} qty={} "
            "price_ticks={} created_us={}",
            record.key, order.order_id(), order.symbol(),
            ordermatch::Side_Name(order.side()), order.quantity(),
            order.price_ticks(), order.created_timestamp_us());

        google::protobuf::ShutdownProtobufLibrary();
        return EXIT_SUCCESS;
      }
    }
  } catch (const std::exception& ex) {
    spdlog::error("Consumer error: {}", ex.what());
    return EXIT_FAILURE;
  }

  spdlog::error("No message received within {} attempts.", kMaxPollAttempts);
  return EXIT_FAILURE;
}
