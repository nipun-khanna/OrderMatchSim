#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <vector>

#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include "common/kafka/Consumer.h"
#include "common/kafka/Topics.h"
#include "common/utils/Env.h"
#include "order_match.pb.h"
#include "risk/RiskEngine.h"

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
    oms::Consumer consumer(brokers, "risk-engine",
                           {oms::topic::ORDERS, oms::topic::TRADES},
                           /*fromBeginning=*/true);
    oms::RiskEngine engine;
    std::unordered_map<std::string, ordermatch::Side> orderSide;
    std::vector<ordermatch::TradeExecuted> pending;

    spdlog::info("Risk engine started; consuming '{}' + '{}' [{}]",
                 oms::topic::ORDERS, oms::topic::TRADES, brokers);

    const auto applyTrade = [&](const ordermatch::TradeExecuted& trade) -> bool {
      const auto it = orderSide.find(trade.aggressor_order_id());
      if (it == orderSide.end()) {
        return false;
      }
      engine.onTrade(trade, it->second);
      return true;
    };

    auto lastReport = std::chrono::steady_clock::now();

    while (g_running.load()) {
      const auto records = consumer.poll(std::chrono::milliseconds(200));

      for (const auto& record : records) {
        if (record.topic != oms::topic::ORDERS) {
          continue;
        }
        ordermatch::OrderCreated order;
        if (!order.ParseFromString(record.payload)) {
          continue;
        }
        engine.onOrder(order);
        orderSide[order.order_id()] = order.side();
      }

      std::vector<ordermatch::TradeExecuted> stillPending;
      for (auto& trade : pending) {
        if (!applyTrade(trade)) {
          stillPending.push_back(std::move(trade));
        }
      }
      pending.swap(stillPending);

      for (const auto& record : records) {
        if (record.topic != oms::topic::TRADES) {
          continue;
        }
        ordermatch::TradeExecuted trade;
        if (!trade.ParseFromString(record.payload)) {
          continue;
        }
        if (!applyTrade(trade)) {
          pending.push_back(std::move(trade));
        }
      }

      const auto now = std::chrono::steady_clock::now();
      if (now - lastReport >= std::chrono::seconds(1)) {
        lastReport = now;
        fmt::print("\033[2J\033[H{}", oms::formatRiskReport(engine.snapshot()));
        std::fflush(stdout);
      }
    }

    spdlog::info("Risk engine shutting down.");
  } catch (const std::exception& ex) {
    spdlog::error("Risk engine error: {}", ex.what());
    return EXIT_FAILURE;
  }

  google::protobuf::ShutdownProtobufLibrary();
  return EXIT_SUCCESS;
}
