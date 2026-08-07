#include <gtest/gtest.h>

#include "matching_engine/OrderBook.h"

using oms::Order;
using oms::OrderBook;
using oms::Side;
using oms::Trade;

namespace {
Order makeOrder(const std::string& id, int64_t price, int32_t qty,
                int64_t ts = 0) {
  return Order{id, price, qty, ts};
}
}

TEST(OrderBook, NonCrossingOrdersRest) {
  OrderBook book;

  auto t1 = book.add(Side::Sell, makeOrder("s1", 10100, 50));
  auto t2 = book.add(Side::Buy, makeOrder("b1", 9900, 50));

  EXPECT_TRUE(t1.empty());
  EXPECT_TRUE(t2.empty());
  EXPECT_EQ(book.bestAsk(), 10100);
  EXPECT_EQ(book.bestBid(), 9900);
  EXPECT_EQ(book.quantityAt(Side::Sell, 10100), 50);
  EXPECT_EQ(book.quantityAt(Side::Buy, 9900), 50);
}

TEST(OrderBook, PartialFillLeavesRestingRemainder) {
  OrderBook book;
  book.add(Side::Sell, makeOrder("s1", 10000, 50));

  auto trades = book.add(Side::Buy, makeOrder("b1", 10100, 100));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].price_ticks, 10000);
  EXPECT_EQ(trades[0].quantity, 50);
  EXPECT_EQ(trades[0].aggressor_order_id, "b1");
  EXPECT_EQ(trades[0].resting_order_id, "s1");

  EXPECT_FALSE(book.hasAsks());
  EXPECT_EQ(book.bestBid(), 10100);
  EXPECT_EQ(book.quantityAt(Side::Buy, 10100), 50);
}

TEST(OrderBook, TradesExecuteAtPassivePrice) {
  OrderBook book;
  book.add(Side::Sell, makeOrder("s1", 10000, 50));

  auto trades = book.add(Side::Buy, makeOrder("b1", 10500, 50));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].price_ticks, 10000);
  EXPECT_EQ(trades[0].quantity, 50);
  EXPECT_FALSE(book.hasAsks());
  EXPECT_FALSE(book.hasBids());
}

TEST(OrderBook, TimePriorityFifoWithinLevel) {
  OrderBook book;
  book.add(Side::Sell, makeOrder("s1", 10000, 30));
  book.add(Side::Sell, makeOrder("s2", 10000, 40));

  auto trades = book.add(Side::Buy, makeOrder("b1", 10000, 50));

  ASSERT_EQ(trades.size(), 2u);
  EXPECT_EQ(trades[0].resting_order_id, "s1");
  EXPECT_EQ(trades[0].quantity, 30);
  EXPECT_EQ(trades[1].resting_order_id, "s2");
  EXPECT_EQ(trades[1].quantity, 20);

  EXPECT_EQ(book.quantityAt(Side::Sell, 10000), 20);
  EXPECT_EQ(book.ordersAt(Side::Sell, 10000), 1u);
}

TEST(OrderBook, SweepsBestPriceFirstAcrossLevels) {
  OrderBook book;
  book.add(Side::Sell, makeOrder("s_low", 10000, 5));
  book.add(Side::Sell, makeOrder("s_high", 10100, 5));

  auto trades = book.add(Side::Buy, makeOrder("b1", 10100, 8));

  ASSERT_EQ(trades.size(), 2u);
  EXPECT_EQ(trades[0].price_ticks, 10000);
  EXPECT_EQ(trades[0].quantity, 5);
  EXPECT_EQ(trades[1].price_ticks, 10100);
  EXPECT_EQ(trades[1].quantity, 3);

  EXPECT_EQ(book.quantityAt(Side::Sell, 10100), 2);
  EXPECT_FALSE(book.hasBids());
}

TEST(OrderBook, SellSideMatchesAgainstBidsAtPassivePrice) {
  OrderBook book;
  book.add(Side::Buy, makeOrder("b1", 10000, 50));

  auto trades = book.add(Side::Sell, makeOrder("s1", 9900, 100));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].price_ticks, 10000);
  EXPECT_EQ(trades[0].quantity, 50);
  EXPECT_EQ(trades[0].aggressor_order_id, "s1");
  EXPECT_EQ(trades[0].resting_order_id, "b1");

  EXPECT_FALSE(book.hasBids());
  EXPECT_EQ(book.bestAsk(), 9900);
  EXPECT_EQ(book.quantityAt(Side::Sell, 9900), 50);
}

TEST(OrderBook, SellSweepsHighestBidsFirst) {
  OrderBook book;
  book.add(Side::Buy, makeOrder("b_high", 10000, 10));
  book.add(Side::Buy, makeOrder("b_low", 9900, 10));

  auto trades = book.add(Side::Sell, makeOrder("s1", 9800, 15));

  ASSERT_EQ(trades.size(), 2u);
  EXPECT_EQ(trades[0].price_ticks, 10000);
  EXPECT_EQ(trades[0].quantity, 10);
  EXPECT_EQ(trades[1].price_ticks, 9900);
  EXPECT_EQ(trades[1].quantity, 5);

  EXPECT_EQ(book.bestBid(), 9900);
  EXPECT_EQ(book.quantityAt(Side::Buy, 9900), 5);
  EXPECT_FALSE(book.hasAsks());
}

TEST(OrderBook, CarriesAggressorTimestampIntoTrade) {
  OrderBook book;
  book.add(Side::Sell, makeOrder("s1", 10000, 50, 111));

  auto trades = book.add(Side::Buy, makeOrder("b1", 10000, 50, 999));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].aggressor_created_timestamp_us, 999);
}

TEST(OrderBook, ExactFillRemovesLevelAndRestsNothing) {
  OrderBook book;
  book.add(Side::Sell, makeOrder("s1", 10000, 50));

  auto trades = book.add(Side::Buy, makeOrder("b1", 10000, 50));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].quantity, 50);
  EXPECT_FALSE(book.hasAsks());
  EXPECT_FALSE(book.hasBids());
}
