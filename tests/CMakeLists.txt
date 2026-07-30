find_package(GTest CONFIG REQUIRED)

add_executable(oms_tests
  proto_roundtrip_test.cpp
  utils_test.cpp)

target_link_libraries(oms_tests PRIVATE
  oms_common
  oms_proto
  GTest::gtest
  GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(oms_tests)
