#include "risk/RiskEngine.h"

#include <algorithm>
#include <cstdlib>

#include <fmt/format.h>

namespace oms {

void RiskEngine::onOrder(const ordermatch::OrderCreated& order) {
  positions_[order.symbol()].mark_ticks = order.price_ticks();
}

void RiskEngine::onTrade(const ordermatch::TradeExecuted& trade,
                         ordermatch::Side aggressor_side) {
  if (aggressor_side != ordermatch::BUY && aggressor_side != ordermatch::SELL) {
    return;
  }

  Position& p = positions_[trade.symbol()];
  const int64_t qty = trade.quantity();
  const int64_t signed_qty = aggressor_side == ordermatch::BUY ? qty : -qty;
  const double price = static_cast<double>(trade.price_ticks());

  const bool increasing = p.net == 0 || (p.net > 0) == (signed_qty > 0);
  if (increasing) {
    const double abs_pos = static_cast<double>(std::llabs(p.net));
    const double abs_add = static_cast<double>(std::llabs(signed_qty));
    const double total = abs_pos + abs_add;
    p.avg_cost_ticks =
        total > 0.0 ? (p.avg_cost_ticks * abs_pos + price * abs_add) / total
                    : 0.0;
    p.net += signed_qty;
    return;
  }

  const int64_t old = p.net;
  const int64_t closing = std::min(std::llabs(old), std::llabs(signed_qty));
  const double direction = old > 0 ? 1.0 : -1.0;
  p.realized_pnl_ticks +=
      direction * (price - p.avg_cost_ticks) * static_cast<double>(closing);
  p.net = old + signed_qty;
  if (p.net == 0) {
    p.avg_cost_ticks = 0.0;
  } else if ((p.net > 0) != (old > 0)) {
    p.avg_cost_ticks = price;
  }
}

std::map<std::string, RiskEngine::SymbolRisk> RiskEngine::snapshot() const {
  std::map<std::string, SymbolRisk> out;
  for (const auto& entry : positions_) {
    const Position& p = entry.second;
    SymbolRisk r;
    r.net_position = p.net;
    r.avg_cost_ticks = p.avg_cost_ticks;
    r.realized_pnl_ticks = p.realized_pnl_ticks;
    r.mark_ticks = p.mark_ticks;
    r.unrealized_pnl_ticks =
        (static_cast<double>(p.mark_ticks) - p.avg_cost_ticks) *
        static_cast<double>(p.net);
    r.exposure_ticks = static_cast<double>(std::llabs(p.net)) *
                       static_cast<double>(p.mark_ticks);
    out.emplace(entry.first, r);
  }
  return out;
}

std::string formatRiskReport(
    const std::map<std::string, RiskEngine::SymbolRisk>& risk) {
  std::string out = "======== RISK ========\n\n";
  out += fmt::format("{:<8}{:>12}{:>16}{:>16}{:>16}\n", "Symbol", "Net Pos",
                     "Realized PnL", "Unrealized PnL", "Exposure");
  for (const auto& entry : risk) {
    const RiskEngine::SymbolRisk& r = entry.second;
    out += fmt::format("{:<8}{:>12}{:>16.2f}{:>16.2f}{:>16.2f}\n", entry.first,
                       r.net_position, r.realized_pnl_ticks / 100.0,
                       r.unrealized_pnl_ticks / 100.0, r.exposure_ticks / 100.0);
  }
  return out;
}

}
