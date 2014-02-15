cmake_minimum_required (VERSION 2.6)

project (comptest)

set(COMMON_FLAGS "-Wno-switch-enum")

include(FindProtobuf)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})
find_package(Protobuf REQUIRED)

include_directories(${PROTOBUF_INCLUDE_DIR})

file(GLOB SRC "*.cpp" "*.hpp")

# change c++ standard library to libc++ (llvm)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v -std=c++11 -stdlib=libc++")
add_executable(comptest ${SRC})

set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")

# specifically link against a protobuf and boost build with libc++
target_link_libraries(comptest "/opt/local/protobuf/lib/libprotobuf.a" "/opt/local/protobuf/lib/libprotoc.a" )