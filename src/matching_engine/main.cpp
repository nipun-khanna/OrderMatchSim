#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <thread>

#include <spdlog/spdlog.h>

#include "common/kafka/Consumer.h"
#include "common/kafka/Producer.h"
#include "common/kafka/Topics.h"
#include "common/utils/BlockingQueue.h"
#include "common/utils/Env.h"
#include "matching_engine/MatchingEngine.h"
#include "order_match.pb.h"

namespace {
std::atomic<bool> g_running{true};
void handleSignal(int) { g_running.store(false); }
}

int main() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  std::signal(SIGINT, handleSignal);
  std::signal(SIGTERM, handleSignal);

  const std::string brokers = oms::util::bootstrapServers();

  try {
    oms::Consumer consumer(brokers, "matching-engine", {oms::topic::ORDERS},
                           /*fromBeginning=*/true);
    oms::Producer producer(brokers);
    oms::MatchingEngine engine;

    oms::util::BlockingQueue<ordermatch::OrderCreated> orderQueue;
    oms::util::BlockingQueue<ordermatch::TradeExecuted> tradeQueue;

    std::atomic<int64_t> ordersConsumed{0};
    std::atomic<int64_t> ordersMatched{0};
    std::atomic<int64_t> tradesPublished{0};

    spdlog::info(
        "Matching engine started; consuming '{}', publishing '{}' [{}]",
        oms::topic::ORDERS, oms::topic::TRADES, brokers);

    std::thread consumerThread([&] {
      while (g_running.load()) {
        const auto records = consumer.poll(std::chrono::milliseconds(500));
        for (const auto& record : records) {
          ordermatch::OrderCreated order;
          if (!order.ParseFromString(record.payload)) {
            spdlog::warn("Skipping un-parseable order (key={})", record.key);
            continue;
          }
          orderQueue.push(std::move(order));
          ordersConsumed.fetch_add(1, std::memory_order_relaxed);
        }
      }
      orderQueue.close();
    });

    std::thread matcherThread([&] {
      ordermatch::OrderCreated order;
      while (orderQueue.pop(order)) {
        ordersMatched.fetch_add(1, std::memory_order_relaxed);
        const auto trades = engine.process(order);
        for (const auto& trade : trades) {
          tradeQueue.push(trade);
        }
      }
      tradeQueue.close();
    });

    std::thread publisherThread([&] {
      ordermatch::TradeExecuted trade;
      while (tradeQueue.pop(trade)) {
        producer.publish(oms::topic::TRADES, trade.symbol(), trade);
        tradesPublished.fetch_add(1, std::memory_order_relaxed);
      }
    });

    consumerThread.join();
    matcherThread.join();
    publisherThread.join();

    spdlog::info(
        "Shutting down. ordersConsumed={} ordersMatched={} tradesPublished={}",
        ordersConsumed.load(), ordersMatched.load(), tradesPublished.load());
  } catch (const std::exception& ex) {
    spdlog::error("Matching engine error: {}", ex.what());
    return EXIT_FAILURE;
  }

  google::protobuf::ShutdownProtobufLibrary();
  return EXIT_SUCCESS;
}
