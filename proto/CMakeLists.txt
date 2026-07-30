# Compiles proto/*.proto into C++ and exposes them as the `oms_proto` library.
# Generated sources land in this directory's build folder, which is added as a
# PUBLIC include dir so consumers can #include "order_match.pb.h".

add_library(oms_proto STATIC)

target_sources(oms_proto PRIVATE order_match.proto)

target_link_libraries(oms_proto PUBLIC protobuf::libprotobuf)

target_include_directories(oms_proto PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")

# protobuf_generate (shipped with the protobuf CMake config) runs protoc at
# build time and attaches the generated .pb.cc/.pb.h to the target.
protobuf_generate(
  TARGET oms_proto
  LANGUAGE cpp
  IMPORT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
  PROTOC_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
