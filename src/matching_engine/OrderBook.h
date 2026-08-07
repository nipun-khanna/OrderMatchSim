#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace oms {

enum class Side { Buy, Sell };

struct Order {
  std::string order_id;
  int64_t price_ticks;
  int32_t quantity;
  int64_t aggressor_created_timestamp_us;
};

struct Trade {
  int64_t price_ticks;
  int32_t quantity;
  std::string aggressor_order_id;
  std::string resting_order_id;
  int64_t aggressor_created_timestamp_us;
};

class OrderBook {
 public:
  std::vector<Trade> add(Side side, Order incoming);

  bool hasBids() const { return !bids_.empty(); }
  bool hasAsks() const { return !asks_.empty(); }
  int64_t bestBid() const { return bids_.empty() ? 0 : bids_.begin()->first; }
  int64_t bestAsk() const { return asks_.empty() ? 0 : asks_.begin()->first; }

  int32_t quantityAt(Side side, int64_t price_ticks) const;
  std::size_t ordersAt(Side side, int64_t price_ticks) const;

 private:
  std::map<int64_t, std::deque<Order>, std::greater<int64_t>> bids_;
  std::map<int64_t, std::deque<Order>, std::less<int64_t>> asks_;
};

}
