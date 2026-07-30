// Phase 1 hello-world producer: publishes a single OrderCreated message to the
// `orders` topic, keyed by symbol, then exits. Pairs with hello_consumer.

#include <cstdlib>

#include <spdlog/spdlog.h>

#include "common/kafka/Producer.h"
#include "common/kafka/Topics.h"
#include "common/utils/Env.h"
#include "common/utils/Ids.h"
#include "common/utils/Time.h"
#include "order_match.pb.h"

int main() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  const std::string brokers = oms::util::bootstrapServers();

  ordermatch::OrderCreated order;
  order.set_order_id(oms::util::newId("ord"));
  order.set_symbol("AAPL");
  order.set_quantity(100);
  order.set_price_ticks(10120);  // $101.20
  order.set_side(ordermatch::BUY);
  order.set_created_timestamp_us(oms::util::nowMicros());

  try {
    oms::Producer producer(brokers);
    producer.publish(oms::topic::ORDERS, order.symbol(), order);
  } catch (const std::exception& ex) {
    spdlog::error("Failed to publish order: {}", ex.what());
    return EXIT_FAILURE;
  }

  spdlog::info(
      "Published OrderCreated to '{}' [{}]: id={} symbol={} side=BUY "
      "qty={} price_ticks={} created_us={}",
      oms::topic::ORDERS, brokers, order.order_id(), order.symbol(),
      order.quantity(), order.price_ticks(), order.created_timestamp_us());

  google::protobuf::ShutdownProtobufLibrary();
  return EXIT_SUCCESS;
}
