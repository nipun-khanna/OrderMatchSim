cmake_minimum_required(VERSION 3.21)

project(OrderMatchSim
        VERSION 0.1.0
        DESCRIPTION "An order-matching stock exchange simulator on Kafka"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Protobuf CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Threads REQUIRED)

find_package(unofficial-modern-cpp-kafka CONFIG QUIET)
if(NOT unofficial-modern-cpp-kafka_FOUND)
  find_package(modern-cpp-kafka CONFIG REQUIRED)
endif()


if(TARGET unofficial::modern-cpp-kafka::modern-cpp-kafka)
  set(MODERN_CPP_KAFKA_TARGET unofficial::modern-cpp-kafka::modern-cpp-kafka)
elseif(TARGET modern-cpp-kafka::modern-cpp-kafka)
  set(MODERN_CPP_KAFKA_TARGET modern-cpp-kafka::modern-cpp-kafka)
elseif(TARGET modern-cpp-kafka-api)
  set(MODERN_CPP_KAFKA_TARGET modern-cpp-kafka-api)
elseif(TARGET modern-cpp-kafka)
  set(MODERN_CPP_KAFKA_TARGET modern-cpp-kafka)
else()
  message(FATAL_ERROR "Could not find the modern-cpp-kafka interface target under a known name.")
endif()
message(STATUS "Using modern-cpp-kafka target: ${MODERN_CPP_KAFKA_TARGET}")


add_subdirectory(proto)
add_subdirectory(src)

enable_testing()
add_subdirectory(tests)
