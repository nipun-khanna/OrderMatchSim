# OrderMatchSim (oms)

An order-matching stock exchange simulator built on Kafka.

## Layout

```
CMakeLists.txt            top-level build entry point
CMakePresets.json         vcpkg toolchain wiring
vcpkg.json                dependency manifest
docker-compose.yml        local Kafka (KRaft, single broker)
proto/                    protobuf schema -> oms_proto library
src/
  common/kafka/           Producer / Consumer wrappers, topic constants
  common/utils/           time, id, env helpers, BlockingQueue
  matching_engine/        OrderBook + MatchingEngine (lib) and the threaded
                          consumer/matcher/publisher service main
  risk/                   RiskEngine (lib) and the risk service main
  analytics/              Analytics + Dashboard (lib) and the analytics service main
tests/                    GoogleTest (broker-free)
```

## Prerequisites

- CMake >= 3.21 and a C++17 compiler
- [vcpkg](https://github.com/microsoft/vcpkg) with `VCPKG_ROOT` exported
- Docker with the Compose v2 plugin. If `docker compose version` fails with
  `'compose' is not a docker command`, install the plugin (no sudo) with:
  ```bash
  mkdir -p ~/.docker/cli-plugins
  curl -sSL "https://github.com/docker/compose/releases/download/v2.32.4/docker-compose-linux-$(uname -m)" \
    -o ~/.docker/cli-plugins/docker-compose && chmod +x ~/.docker/cli-plugins/docker-compose
  ```

## Build

```bash
export VCPKG_ROOT=/path/to/vcpkg      # if not already set
cmake --preset default                # resolves deps via vcpkg, then configures
cmake --build build -j
```

## Run

```bash
# 1. Start Kafka and create the orders/trades topics
docker compose up -d

# 2. Run the matching engine (consumes orders, publishes trades)
./build/src/matching_engine
```

## Test

```bash
ctest --preset default 
```

