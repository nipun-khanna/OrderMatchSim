find_package(GTest CONFIG REQUIRED)

add_executable(oms_tests
  proto_roundtrip_test.cpp
  utils_test.cpp
  order_book_test.cpp
  risk_engine_test.cpp
  analytics_test.cpp
  blocking_queue_test.cpp)

target_link_libraries(oms_tests PRIVATE
  oms_common
  oms_proto
  oms_matching
  oms_risk
  oms_analytics
  GTest::gtest
  GTest::gtest_main
  Threads::Threads)

include(GoogleTest)
gtest_discover_tests(oms_tests)
