# **High-Level Goal**

Build a miniature stock exchange that processes a stream of buy/sell orders.

Think of this as a simplified version of Nasdaq.

Instead of one giant program, you'll build several independent C++ programs that communicate through Kafka. They should ALL be built under the `/home/khanipun/testing/OrderMatchSim/` project folder.

```
              +----------------------+
              | Market Data Generator|
              +----------+-----------+
                         |
                         v
                   orders topic
                         |
         +---------------+---------------+
         |               |               |
         v               |               v
  Matching Engine        |       Analytics Engine
         |               |       (+ CLI dashboard)
         v               v               ^
    trades topic --> Risk Engine         |
         |                               |
         +-------------------------------+
```

Two Kafka topics only:

* `orders` — produced by the Market Data Generator; consumed by the Matching Engine, Risk Engine, and Analytics Engine
* `trades` — produced by the Matching Engine; consumed by the Risk Engine and Analytics Engine

- - -

# **Components**

## **1. Market Data Generator**

Purpose:

Continuously publishes new limit orders to the `orders` topic.

Produces

```
OrderCreated

{
    order_id
    created_timestamp_us
    symbol
    side
    price_ticks
    quantity
}
```

Notes:

* `created_timestamp_us` is microseconds since epoch, set at generation time. This is the starting point for latency measurement.
* Messages are **keyed by symbol** so all orders for a symbol land on the same partition (Kafka only guarantees ordering per partition; matching correctness depends on per-symbol order).

This service can simply loop forever:

```
while (true)
{
    generateRandomOrder();
    kafka.publish(order);   // key = symbol
}
```

- - -

## **2. Matching Engine (Main Project)**

This is the core.

Consumes `orders`, maintains an order book per symbol, publishes `TradeExecuted` to `trades`.

```
BUY

101.20 -> 50 shares
101.10 -> 25 shares
100.90 -> 80 shares

SELL

101.40 -> 20 shares
101.50 -> 75 shares
```

### Matching semantics (explicit)

* **Limit orders only.** No market orders, no cancels, no modifies.
* An incoming order **crosses** when: incoming buy price >= best ask, or incoming sell price <= best bid.
* An incoming order matches **repeatedly** against the opposite side of the book (best price first, FIFO within a price level) until it is fully filled or no longer crosses.
* Trades execute at the **resting (passive) order's price**, never the incoming order's price.
* Any unfilled remainder of the incoming order **rests in the book** at its limit price.
* Partial fills are normal: a 100-share buy matching a 50-share resting sell produces a 50-share trade; the resting order is removed and the buy rests with 50 shares remaining (if it no longer crosses).

Example

```
Resting sell: 50 shares @ 100
Incoming buy: 100 shares @ 101

Trade: 50 shares @ 100        (resting order's price)
Book:  buy 50 shares @ 101 rests
```

### Data structure

```
Buy side:  std::map<int64_t, std::deque<Order>, std::greater<>>   // highest price first
Sell side: std::map<int64_t, std::deque<Order>, std::less<>>      // lowest price first
```

With these comparators, `begin()` is always the best price on both sides. The `std::deque` per price level gives FIFO time priority. This demonstrates ordered containers and price-time priority.

### Prices are fixed-point integers

**Never use `double` for price.** Floating-point map keys are a bug factory (equality comparisons on doubles), and real exchanges don't do it. Prices are `int64_t price_ticks` where 1 tick = 1 cent (e.g., $101.20 = 10120). Convert to dollars only for display.

### TradeExecuted

```
TradeExecuted

{
    trade_id
    symbol
    price_ticks
    quantity
    aggressor_order_id
    resting_order_id
    aggressor_created_timestamp_us   // copied from the incoming order
    executed_timestamp_us            // set by the matching engine at execution
}
```

Carrying the aggressor's creation timestamp in the trade lets analytics compute latency without joining topics.

- - -

## **3. Risk Engine**

Consumes **both** `trades` and `orders`.

* `trades` → updates positions and realized PnL
* `orders` → tracks the last observed price per symbol (used as the mark price for unrealized PnL)

Tracks

```
Net Position          Realized PnL      Unrealized PnL     Exposure

AAPL   +200           ...               ...                ...
MSFT   -150           ...               ...                ...
GOOG   +75            ...               ...                ...
```

* **Realized PnL**: computed on trades that reduce a position (average-cost method is fine).
* **Unrealized PnL**: `(mark price - average entry price) * net position`, using the last observed order price as the mark.
* **Exposure**: `|net position| * mark price`, summed per symbol.

- - -

## **4. Analytics Engine (+ CLI Dashboard)**

Consumes both `orders` and `trades`. The CLI dashboard is part of this process — no separate service, no extra topic.

Calculates

* trades/sec
* average latency (see definition below)
* VWAP (Volume Weighted Average Price)
* top traded symbols
* average order size
* orders processed / trades executed counters

### Latency definition

**Latency = `executed_timestamp_us - aggressor_created_timestamp_us`**, i.e., time from order creation at the generator to trade execution at the matching engine, computed per trade from fields carried in `TradeExecuted`. This measures the full generator → Kafka → matching pipeline. Everything runs on one machine, so wall-clock timestamps across processes are comparable.

### Dashboard output

Every second, print:

```
======== MARKET ========

Orders processed    24839
Trades              9230
Trades/sec          532
Top Symbol          AAPL
VWAP (AAPL)         198.34
Avg order size      112
Avg latency         1.2 ms
```

- - -

# **Kafka Topics**

```
orders    key = symbol    OrderCreated messages
trades    key = symbol    TradeExecuted messages
```

Each service owns one responsibility. Keying by symbol preserves per-symbol ordering, which matching correctness depends on.

- - -

# **Project Structure**

```
OrderMatchSim/
    CMakeLists.txt          # top-level build
    docker-compose.yml      # Kafka broker (KRaft mode, single container)
    vcpkg.json              # dependency manifest
    proto/                  # .proto schema files
    matching_engine/
    market_generator/
    analytics/              # includes the CLI dashboard
    risk/
    common/
        kafka/              # producer/consumer wrappers
        models/             # generated protobuf code, shared types
        utils/
```

- - -

# **Serialization**

Instead of JSON, use Protocol Buffers.

```
enum Side
{
    SIDE_UNSPECIFIED = 0;
    BUY = 1;
    SELL = 2;
}

message OrderCreated
{
    string order_id = 1;
    string symbol = 2;
    int32 quantity = 3;
    int64 price_ticks = 4;          // fixed-point: 1 tick = 1 cent
    Side side = 5;
    int64 created_timestamp_us = 6; // microseconds since epoch
}

message TradeExecuted
{
    string trade_id = 1;
    string symbol = 2;
    int64 price_ticks = 3;
    int32 quantity = 4;
    string aggressor_order_id = 5;
    string resting_order_id = 6;
    int64 aggressor_created_timestamp_us = 7;
    int64 executed_timestamp_us = 8;
}
```

Use `int32` consistently for quantities in C++ as well — don't mix `int`/`size_t` casually.

This is much closer to production systems.

- - -

# **Concurrency**

**Build the matching engine single-threaded first.** A single poll → match → publish loop is correct, easy to test, and honestly how many real engines work. Add threading as a deliberate later phase, not the starting point.

The threaded end state:

```
Kafka Consumer Thread
        ↓  (mutex-guarded queue)
Matching Thread
        ↓  (mutex-guarded queue)
Trade Publisher Thread
```

Use

```
std::thread
std::mutex
std::condition_variable
std::atomic
```

Optional:

Replace mutex queues with a lock-free queue later as an optimization.

- - -

# **Matching Algorithm**

Simplified price-time priority:

```
Receive Order
      ↓
Crosses the opposite book?  (buy >= best ask, or sell <= best bid)
      ↓ yes
Execute trade at resting order's price
      ↓
Repeat against next best level until filled or no longer crossing
      ↓
Rest any remainder in the book
      ↓
Publish trade(s)
      ↓
Next order
```

- - -

# **Data Sources**

Generate realistic synthetic orders using configurable parameters:

* Symbols: `AAPL`, `MSFT`, `NVDA`, `GOOG`, `AMZN`
* Prices that follow a random walk (so they drift rather than jump randomly)
* Random order sizes (e.g., 1-500 shares)
* Random buy/sell ratios
* Bursts of activity to simulate volatile periods

This gives us complete control and is ideal for demonstrating the system.

- - -

# **Technologies**

| **Component** | **Technology** |
| --- | --- |
| Language | C++ |
| Messaging | Apache Kafka |
| Kafka client | librdkafka (+ modern-cpp-kafka header-only wrapper) |
| Serialization | Protocol Buffers |
| Build | CMake |
| Tests | GoogleTest |
| Logging | spdlog |
| Formatting | fmt |
| Package Management | vcpkg |
| Local Infrastructure | Docker Compose (single Kafka broker, KRaft mode — no ZooKeeper) |

- - -

# **Milestones**
*Do NOT write comments in any file - update the README as needed.*

### Phase 1 — Plumbing

docker-compose brings up Kafka (KRaft, single broker). Protobuf schemas compile via CMake. A hello-world producer/consumer pair round-trips one `OrderCreated` message through the `orders` topic.

### Phase 2 — Matching engine (single-threaded)

Order book with fixed-point prices and price-time priority. Poll → match → publish loop. **GoogleTest unit tests on the order book** — it's pure logic and the ideal test target: partial fills, price-time priority, resting remainders, trades at the passive price.

### Phase 3 — Risk + Analytics

Risk engine consuming `trades` + `orders` (positions, realized/unrealized PnL, exposure). Analytics engine with the once-per-second CLI dashboard.

### Phase 4 — Threading

Split the matching engine into consumer / matcher / publisher threads with mutex-guarded queues and condition variables.

### Phase 5 — Optional stretch

Lock-free queue in place of the mutex queues; order book depth reporting in analytics.
