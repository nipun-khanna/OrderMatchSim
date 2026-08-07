#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>

#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include "analytics/Analytics.h"
#include "analytics/Dashboard.h"
#include "common/kafka/Consumer.h"
#include "common/kafka/Topics.h"
#include "common/utils/Env.h"
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
    oms::Consumer consumer(brokers, "analytics-engine",
                           {oms::topic::ORDERS, oms::topic::TRADES},
                           /*fromBeginning=*/true);
    oms::Analytics analytics;

    spdlog::info("Analytics engine started; consuming '{}' + '{}' [{}]",
                 oms::topic::ORDERS, oms::topic::TRADES, brokers);

    auto lastRender = std::chrono::steady_clock::now();
    int64_t previousTrades = 0;

    while (g_running.load()) {
      const auto records = consumer.poll(std::chrono::milliseconds(200));
      for (const auto& record : records) {
        if (record.topic == oms::topic::ORDERS) {
          ordermatch::OrderCreated order;
          if (order.ParseFromString(record.payload)) {
            analytics.onOrder(order);
          }
        } else if (record.topic == oms::topic::TRADES) {
          ordermatch::TradeExecuted trade;
          if (trade.ParseFromString(record.payload)) {
            analytics.onTrade(trade);
          }
        }
      }

      const auto now = std::chrono::steady_clock::now();
      const auto elapsed = now - lastRender;
      if (elapsed >= std::chrono::seconds(1)) {
        const auto snap = analytics.snapshot();
        const double seconds =
            std::chrono::duration<double>(elapsed).count();
        const double tradesPerSec =
            seconds > 0.0
                ? static_cast<double>(snap.trades_executed - previousTrades) /
                      seconds
                : 0.0;
        previousTrades = snap.trades_executed;
        lastRender = now;

        fmt::print("\033[2J\033[H{}", oms::formatDashboard(snap, tradesPerSec));
        std::fflush(stdout);
      }
    }

    spdlog::info("Analytics engine shutting down.");
  } catch (const std::exception& ex) {
    spdlog::error("Analytics engine error: {}", ex.what());
    return EXIT_FAILURE;
  }

  google::protobuf::ShutdownProtobufLibrary();
  return EXIT_SUCCESS;
}
