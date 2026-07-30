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
  common/utils/           time, id, env helpers
  hello_world/            phase-1 producer_main / consumer_main
tests/                    GoogleTest (broker-free)
```

## Prerequisites

- CMake >= 3.21 and a C++17 compiler
- [vcpkg](https://github.com/microsoft/vcpkg) with `VCPKG_ROOT` exported
- Docker + Docker Compose

## Build

```bash
export VCPKG_ROOT=/path/to/vcpkg      # if not already set
cmake --preset default                # resolves deps via vcpkg, then configures
cmake --build build -j
```

## Run the phase-1 round-trip

```bash
# 1. Start Kafka and create topics
docker compose up -d

# 2. In one terminal, wait for the message
./build/src/hello_consumer

# 3. In another terminal, publish one order
./build/src/hello_producer
```

The consumer prints the received `OrderCreated` and exits. Override the broker
with `KAFKA_BOOTSTRAP_SERVERS=host:port` (default `localhost:9092`).

## Test

```bash
ctest --preset default 
```

The phase-1 tests need no running broker — they exercise protobuf
serialization and the shared utilities.

## Toolchain notes

- **protobuf is pinned to 3.21.12** (via the `overrides` block in `vcpkg.json`).
  Newer protobuf (4.x/6.x) depends on abseil, and current abseil does not
  compile on the host's default `gcc 7.3.1`. 3.21.12 is the last release before
  that dependency and builds cleanly on gcc 7. If you move to `gcc >= 10` or
  clang, you can delete the `overrides` block to use modern protobuf.
- **vcpkg must be a full clone** for the version pin to resolve. A shallow clone
  (`git clone --depth 1`) lacks historical port trees and fails with
  `failed to unpack tree object ...`. Fix with:
  ```bash
  git -C "$VCPKG_ROOT" fetch --unshallow
  ```
- The modern-cpp-kafka vcpkg port exports the package as
  `unofficial-modern-cpp-kafka` (target
  `unofficial::modern-cpp-kafka::modern-cpp-kafka`); the top-level
  `CMakeLists.txt` resolves this and the upstream name automatically.

