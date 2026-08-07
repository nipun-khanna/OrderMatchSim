#include "matching_engine/MatchingEngine.h"

#include <utility>

#include "common/utils/Ids.h"
#include "common/utils/Time.h"

namespace oms {

std::vector<ordermatch::TradeExecuted> MatchingEngine::process(
    const ordermatch::OrderCreated& order) {
  std::vector<ordermatch::TradeExecuted> executed;

  Side side;
  if (order.side() == ordermatch::BUY) {
    side = Side::Buy;
  } else if (order.side() == ordermatch::SELL) {
    side = Side::Sell;
  } else {
    return executed;
  }

  Order incoming{order.order_id(), order.price_ticks(), order.quantity(),
                 order.created_timestamp_us()};

  OrderBook& book = books_[order.symbol()];
  const std::vector<Trade> trades = book.add(side, std::move(incoming));

  executed.reserve(trades.size());
  for (const Trade& t : trades) {
    ordermatch::TradeExecuted te;
    te.set_trade_id(util::newId("trd"));
    te.set_symbol(order.symbol());
    te.set_price_ticks(t.price_ticks);
    te.set_quantity(t.quantity);
    te.set_aggressor_order_id(t.aggressor_order_id);
    te.set_resting_order_id(t.resting_order_id);
    te.set_aggressor_created_timestamp_us(t.aggressor_created_timestamp_us);
    te.set_executed_timestamp_us(util::nowMicros());
    executed.push_back(std::move(te));
  }
  return executed;
}

}
