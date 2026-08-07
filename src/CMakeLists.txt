add_library(oms_common
  common/kafka/Producer.cpp
  common/kafka/Consumer.cpp)

target_include_directories(oms_common PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(oms_common
  PUBLIC
    oms_proto
    ${MODERN_CPP_KAFKA_TARGET}
    protobuf::libprotobuf
  PRIVATE
    spdlog::spdlog
    fmt::fmt
    Threads::Threads)

add_library(oms_matching
  matching_engine/OrderBook.cpp
  matching_engine/MatchingEngine.cpp)

target_link_libraries(oms_matching
  PUBLIC
    oms_proto
    oms_common)

add_executable(matching_engine matching_engine/main.cpp)
target_link_libraries(matching_engine PRIVATE oms_matching oms_common spdlog::spdlog Threads::Threads)

add_library(oms_risk risk/RiskEngine.cpp)

target_include_directories(oms_risk PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(oms_risk
  PUBLIC
    oms_proto
  PRIVATE
    fmt::fmt)

add_executable(risk_engine risk/main.cpp)
target_link_libraries(risk_engine PRIVATE oms_risk oms_common spdlog::spdlog fmt::fmt)

add_library(oms_analytics
  analytics/Analytics.cpp
  analytics/Dashboard.cpp)

target_include_directories(oms_analytics PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(oms_analytics
  PUBLIC
    oms_proto
  PRIVATE
    fmt::fmt)

add_executable(analytics_engine analytics/main.cpp)
target_link_libraries(analytics_engine PRIVATE oms_analytics oms_common spdlog::spdlog fmt::fmt)
