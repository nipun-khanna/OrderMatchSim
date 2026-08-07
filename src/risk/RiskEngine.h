#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>

#include "order_match.pb.h"

namespace oms {

class RiskEngine {
 public:
  struct SymbolRisk {
    int64_t net_position = 0;
    double avg_cost_ticks = 0.0;
    double realized_pnl_ticks = 0.0;
    int64_t mark_ticks = 0;
    double unrealized_pnl_ticks = 0.0;
    double exposure_ticks = 0.0;
  };

  void onOrder(const ordermatch::OrderCreated& order);
  void onTrade(const ordermatch::TradeExecuted& trade,
               ordermatch::Side aggressor_side);

  std::map<std::string, SymbolRisk> snapshot() const;

 private:
  struct Position {
    int64_t net = 0;
    double avg_cost_ticks = 0.0;
    double realized_pnl_ticks = 0.0;
    int64_t mark_ticks = 0;
  };

  std::unordered_map<std::string, Position> positions_;
};

std::string formatRiskReport(
    const std::map<std::string, RiskEngine::SymbolRisk>& risk);

}
