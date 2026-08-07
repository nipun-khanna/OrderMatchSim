#include <gtest/gtest.h>

#include "order_match.pb.h"
#include "risk/RiskEngine.h"

using oms::RiskEngine;

namespace {

ordermatch::OrderCreated makeOrder(const std::string& symbol,
                                   int64_t price_ticks) {
  ordermatch::OrderCreated o;
  o.set_symbol(symbol);
  o.set_price_ticks(price_ticks);
  return o;
}

ordermatch::TradeExecuted makeTrade(const std::string& symbol,
                                    int64_t price_ticks, int32_t quantity,
                                    const std::string& aggressor_order_id) {
  ordermatch::TradeExecuted t;
  t.set_symbol(symbol);
  t.set_price_ticks(price_ticks);
  t.set_quantity(quantity);
  t.set_aggressor_order_id(aggressor_order_id);
  return t;
}

}

TEST(RiskEngine, LongPositionAveragesEntryAndRealizesOnReduction) {
  RiskEngine engine;
  engine.onTrade(makeTrade("AAPL", 10000, 100, "a1"), ordermatch::BUY);
  engine.onTrade(makeTrade("AAPL", 10100, 40, "a2"), ordermatch::SELL);

  const auto risk = engine.snapshot();
  ASSERT_EQ(risk.count("AAPL"), 1u);
  const auto& r = risk.at("AAPL");

  EXPECT_EQ(r.net_position, 60);
  EXPECT_DOUBLE_EQ(r.avg_cost_ticks, 10000.0);
  EXPECT_DOUBLE_EQ(r.realized_pnl_ticks, 4000.0);
}

TEST(RiskEngine, ShortPositionRealizesProfitWhenCoveringLower) {
  RiskEngine engine;
  engine.onTrade(makeTrade("MSFT", 10000, 50, "a1"), ordermatch::SELL);
  engine.onTrade(makeTrade("MSFT", 9900, 20, "a2"), ordermatch::BUY);

  const auto& r = engine.snapshot().at("MSFT");
  EXPECT_EQ(r.net_position, -30);
  EXPECT_DOUBLE_EQ(r.avg_cost_ticks, 10000.0);
  EXPECT_DOUBLE_EQ(r.realized_pnl_ticks, 2000.0);
}

TEST(RiskEngine, PositionFlipResetsAverageToTradePrice) {
  RiskEngine engine;
  engine.onTrade(makeTrade("GOOG", 10000, 50, "a1"), ordermatch::SELL);
  engine.onTrade(makeTrade("GOOG", 9900, 80, "a2"), ordermatch::BUY);

  const auto& r = engine.snapshot().at("GOOG");
  EXPECT_EQ(r.net_position, 30);
  EXPECT_DOUBLE_EQ(r.avg_cost_ticks, 9900.0);
  EXPECT_DOUBLE_EQ(r.realized_pnl_ticks, 5000.0);
}

TEST(RiskEngine, UnrealizedPnlAndExposureUseMarkFromOrders) {
  RiskEngine engine;
  engine.onTrade(makeTrade("AAPL", 10000, 100, "a1"), ordermatch::BUY);
  engine.onOrder(makeOrder("AAPL", 10500));

  const auto& r = engine.snapshot().at("AAPL");
  EXPECT_EQ(r.net_position, 100);
  EXPECT_EQ(r.mark_ticks, 10500);
  EXPECT_DOUBLE_EQ(r.unrealized_pnl_ticks, 50000.0);
  EXPECT_DOUBLE_EQ(r.exposure_ticks, 1050000.0);
}

TEST(RiskEngine, UnknownAggressorSideIsIgnored) {
  RiskEngine engine;
  engine.onTrade(makeTrade("AAPL", 10000, 100, "a1"),
                 ordermatch::SIDE_UNSPECIFIED);

  EXPECT_TRUE(engine.snapshot().empty());
}

TEST(RiskEngine, ReportContainsSymbolAndDollarValues) {
  RiskEngine engine;
  engine.onTrade(makeTrade("AAPL", 10000, 100, "a1"), ordermatch::BUY);
  engine.onTrade(makeTrade("AAPL", 10100, 40, "a2"), ordermatch::SELL);

  const std::string report = oms::formatRiskReport(engine.snapshot());
  EXPECT_NE(report.find("AAPL"), std::string::npos);
  EXPECT_NE(report.find("40.00"), std::string::npos);
}
