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


add_executable(hello_producer hello_world/producer_main.cpp)
target_link_libraries(hello_producer PRIVATE oms_common spdlog::spdlog)

add_executable(hello_consumer hello_world/consumer_main.cpp)
target_link_libraries(hello_consumer PRIVATE oms_common spdlog::spdlog)
