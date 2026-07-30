#include <gtest/gtest.h>

#include "order_match.pb.h"

TEST(OrderCreatedRoundTrip, PreservesAllFields) {
  ordermatch::OrderCreated original;
  original.set_order_id("ord-42");
  original.set_symbol("AAPL");
  original.set_quantity(100);
  original.set_price_ticks(10120);  // $101.20
  original.set_side(ordermatch::BUY);
  original.set_created_timestamp_us(1'700'000'000'000'000LL);

  std::string wire;
  ASSERT_TRUE(original.SerializeToString(&wire));

  ordermatch::OrderCreated parsed;
  ASSERT_TRUE(parsed.ParseFromString(wire));

  EXPECT_EQ(parsed.order_id(), "ord-42");
  EXPECT_EQ(parsed.symbol(), "AAPL");
  EXPECT_EQ(parsed.quantity(), 100);
  EXPECT_EQ(parsed.price_ticks(), 10120);
  EXPECT_EQ(parsed.side(), ordermatch::BUY);
  EXPECT_EQ(parsed.created_timestamp_us(), 1'700'000'000'000'000LL);
}

TEST(TradeExecutedRoundTrip, PreservesAllFields) {
  ordermatch::TradeExecuted original;
  original.set_trade_id("trd-7");
  original.set_symbol("MSFT");
  original.set_price_ticks(41850);
  original.set_quantity(50);
  original.set_aggressor_order_id("ord-100");
  original.set_resting_order_id("ord-99");
  original.set_aggressor_created_timestamp_us(1'700'000'000'000'000LL);
  original.set_executed_timestamp_us(1'700'000'000'001'200LL);

  std::string wire;
  ASSERT_TRUE(original.SerializeToString(&wire));

  ordermatch::TradeExecuted parsed;
  ASSERT_TRUE(parsed.ParseFromString(wire));

  EXPECT_EQ(parsed.trade_id(), "trd-7");
  EXPECT_EQ(parsed.symbol(), "MSFT");
  EXPECT_EQ(parsed.price_ticks(), 41850);
  EXPECT_EQ(parsed.quantity(), 50);
  EXPECT_EQ(parsed.aggressor_order_id(), "ord-100");
  EXPECT_EQ(parsed.resting_order_id(), "ord-99");
  EXPECT_EQ(parsed.aggressor_created_timestamp_us(), 1'700'000'000'000'000LL);
  EXPECT_EQ(parsed.executed_timestamp_us(), 1'700'000'000'001'200LL);

  EXPECT_EQ(parsed.executed_timestamp_us() -
                parsed.aggressor_created_timestamp_us(),
            1200);
}

TEST(SideEnum, DefaultsToUnspecified) {
  ordermatch::OrderCreated order;
  EXPECT_EQ(order.side(), ordermatch::SIDE_UNSPECIFIED);
}
