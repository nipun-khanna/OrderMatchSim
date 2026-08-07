#include "matching_engine/OrderBook.h"

#include <algorithm>

namespace oms {

namespace {

template <typename BookSide>
std::vector<Trade> matchAgainst(BookSide& opposite, Order& incoming,
                                bool incomingIsBuy) {
  std::vector<Trade> trades;
  while (incoming.quantity > 0 && !opposite.empty()) {
    auto level = opposite.begin();
    const int64_t restingPrice = level->first;
    const bool crosses = incomingIsBuy ? (incoming.price_ticks >= restingPrice)
                                       : (incoming.price_ticks <= restingPrice);
    if (!crosses) {
      break;
    }
    auto& queue = level->second;
    while (incoming.quantity > 0 && !queue.empty()) {
      Order& resting = queue.front();
      const int32_t fill = std::min(incoming.quantity, resting.quantity);
      trades.push_back(Trade{restingPrice, fill, incoming.order_id,
                             resting.order_id,
                             incoming.aggressor_created_timestamp_us});
      incoming.quantity -= fill;
      resting.quantity -= fill;
      if (resting.quantity == 0) {
        queue.pop_front();
      }
    }
    if (queue.empty()) {
      opposite.erase(level);
    }
  }
  return trades;
}

}

std::vector<Trade> OrderBook::add(Side side, Order incoming) {
  std::vector<Trade> trades;
  if (side == Side::Buy) {
    trades = matchAgainst(asks_, incoming, true);
    if (incoming.quantity > 0) {
      bids_[incoming.price_ticks].push_back(incoming);
    }
  } else {
    trades = matchAgainst(bids_, incoming, false);
    if (incoming.quantity > 0) {
      asks_[incoming.price_ticks].push_back(incoming);
    }
  }
  return trades;
}

int32_t OrderBook::quantityAt(Side side, int64_t price_ticks) const {
  int32_t total = 0;
  if (side == Side::Buy) {
    const auto it = bids_.find(price_ticks);
    if (it != bids_.end()) {
      for (const Order& o : it->second) total += o.quantity;
    }
  } else {
    const auto it = asks_.find(price_ticks);
    if (it != asks_.end()) {
      for (const Order& o : it->second) total += o.quantity;
    }
  }
  return total;
}

std::size_t OrderBook::ordersAt(Side side, int64_t price_ticks) const {
  if (side == Side::Buy) {
    const auto it = bids_.find(price_ticks);
    return it != bids_.end() ? it->second.size() : 0;
  }
  const auto it = asks_.find(price_ticks);
  return it != asks_.end() ? it->second.size() : 0;
}

}
