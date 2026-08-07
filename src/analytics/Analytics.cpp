#include "analytics/Analytics.h"

namespace oms {

void Analytics::onOrder(const ordermatch::OrderCreated& order) {
  ++orders_processed_;
  order_quantity_sum_ += order.quantity();
}

void Analytics::onTrade(const ordermatch::TradeExecuted& trade) {
  ++trades_executed_;

  SymbolVolume& v = symbol_volume_[trade.symbol()];
  v.volume += trade.quantity();
  v.notional_ticks += static_cast<double>(trade.price_ticks()) *
                      static_cast<double>(trade.quantity());

  const int64_t latency =
      trade.executed_timestamp_us() - trade.aggressor_created_timestamp_us();
  if (latency >= 0) {
    latency_sum_us_ += static_cast<double>(latency);
    ++latency_count_;
  }
}

Analytics::Snapshot Analytics::snapshot() const {
  Snapshot s;
  s.orders_processed = orders_processed_;
  s.trades_executed = trades_executed_;
  s.avg_order_size =
      orders_processed_ > 0 ? static_cast<double>(order_quantity_sum_) /
                                  static_cast<double>(orders_processed_)
                            : 0.0;

  for (const auto& entry : symbol_volume_) {
    const SymbolVolume& v = entry.second;
    if (v.volume > s.top_symbol_volume) {
      s.top_symbol_volume = v.volume;
      s.top_symbol = entry.first;
      s.top_symbol_vwap_ticks =
          v.volume > 0 ? v.notional_ticks / static_cast<double>(v.volume) : 0.0;
    }
  }

  s.avg_latency_us = latency_count_ > 0
                         ? latency_sum_us_ / static_cast<double>(latency_count_)
                         : 0.0;
  return s;
}

}
