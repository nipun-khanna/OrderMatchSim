#include <gtest/gtest.h>

#include "analytics/Analytics.h"
#include "analytics/Dashboard.h"
#include "order_match.pb.h"

using oms::Analytics;

namespace {

ordermatch::OrderCreated makeOrder(const std::string& symbol,
                                   int32_t quantity) {
  ordermatch::OrderCreated o;
  o.set_symbol(symbol);
  o.set_quantity(quantity);
  return o;
}

ordermatch::TradeExecuted makeTrade(const std::string& symbol,
                                    int64_t price_ticks, int32_t quantity,
                                    int64_t created_us, int64_t executed_us) {
  ordermatch::TradeExecuted t;
  t.set_symbol(symbol);
  t.set_price_ticks(price_ticks);
  t.set_quantity(quantity);
  t.set_aggressor_created_timestamp_us(created_us);
  t.set_executed_timestamp_us(executed_us);
  return t;
}

}

TEST(Analytics, CountsOrdersAndAverageOrderSize) {
  Analytics analytics;
  analytics.onOrder(makeOrder("AAPL", 100));
  analytics.onOrder(makeOrder("AAPL", 200));

  const auto s = analytics.snapshot();
  EXPECT_EQ(s.orders_processed, 2);
  EXPECT_DOUBLE_EQ(s.avg_order_size, 150.0);
}

TEST(Analytics, ComputesVwapAndTopSymbol) {
  Analytics analytics;
  analytics.onTrade(makeTrade("AAPL", 10000, 100, 0, 0));
  analytics.onTrade(makeTrade("AAPL", 10200, 50, 0, 0));
  analytics.onTrade(makeTrade("MSFT", 20000, 10, 0, 0));

  const auto s = analytics.snapshot();
  EXPECT_EQ(s.trades_executed, 3);
  EXPECT_EQ(s.top_symbol, "AAPL");
  EXPECT_EQ(s.top_symbol_volume, 150);
  EXPECT_NEAR(s.top_symbol_vwap_ticks, 10066.6667, 0.01);
}

TEST(Analytics, ComputesAverageLatencyIgnoringNegative) {
  Analytics analytics;
  analytics.onTrade(makeTrade("AAPL", 10000, 10, 1000, 3000));
  analytics.onTrade(makeTrade("AAPL", 10000, 10, 2000, 4000));
  analytics.onTrade(makeTrade("AAPL", 10000, 10, 5000, 4000));

  const auto s = analytics.snapshot();
  EXPECT_DOUBLE_EQ(s.avg_latency_us, 2000.0);
}

TEST(Dashboard, RendersMarketBlockWithMetrics) {
  Analytics analytics;
  analytics.onOrder(makeOrder("AAPL", 100));
  analytics.onTrade(makeTrade("AAPL", 19834, 100, 1000, 2200));

  const std::string out = oms::formatDashboard(analytics.snapshot(), 532.0);
  EXPECT_NE(out.find("======== MARKET ========"), std::string::npos);
  EXPECT_NE(out.find("Orders processed"), std::string::npos);
  EXPECT_NE(out.find("Trades/sec"), std::string::npos);
  EXPECT_NE(out.find("532"), std::string::npos);
  EXPECT_NE(out.find("AAPL"), std::string::npos);
  EXPECT_NE(out.find("198.34"), std::string::npos);
  EXPECT_NE(out.find("ms"), std::string::npos);
}
