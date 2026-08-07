add_library(oms_proto STATIC)

target_sources(oms_proto PRIVATE order_match.proto)

target_link_libraries(oms_proto PUBLIC protobuf::libprotobuf)

target_include_directories(oms_proto PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")

protobuf_generate(
  TARGET oms_proto
  LANGUAGE cpp
  IMPORT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
  PROTOC_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
