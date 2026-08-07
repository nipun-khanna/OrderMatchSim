#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "order_match.pb.h"

namespace oms {

class Analytics {
 public:
  struct Snapshot {
    int64_t orders_processed = 0;
    int64_t trades_executed = 0;
    double avg_order_size = 0.0;
    std::string top_symbol;
    int64_t top_symbol_volume = 0;
    double top_symbol_vwap_ticks = 0.0;
    double avg_latency_us = 0.0;
  };

  void onOrder(const ordermatch::OrderCreated& order);
  void onTrade(const ordermatch::TradeExecuted& trade);

  Snapshot snapshot() const;

 private:
  struct SymbolVolume {
    int64_t volume = 0;
    double notional_ticks = 0.0;
  };

  int64_t orders_processed_ = 0;
  int64_t trades_executed_ = 0;
  int64_t order_quantity_sum_ = 0;
  double latency_sum_us_ = 0.0;
  int64_t latency_count_ = 0;
  std::unordered_map<std::string, SymbolVolume> symbol_volume_;
};

}
