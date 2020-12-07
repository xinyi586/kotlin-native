# Disclaimer: this is a straw-man attempt to add navigation to Clion or similar tools.
# At the moment it can't be used for real building or debugging. Feel free to contribute

cmake_minimum_required(VERSION 3.14)
project(runtime)

set(CMAKE_CXX_STANDARD 14)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

set(GOOGLETEST_DIR ${CMAKE_SOURCE_DIR}/googletest)

include_directories(${SRC_DIR}/main/cpp)
include_directories(${SRC_DIR}/debug/headers)
include_directories(${CMAKE_SOURCE_DIR}/../common/src/hash/headers)

include_directories(${GOOGLETEST_DIR}/googletest/include)
include_directories(${GOOGLETEST_DIR}/googlemock/include)

file(GLOB_RECURSE SOURCES *.cpp *.c)

add_executable(runtime ${SOURCES})

target_compile_definitions(runtime PUBLIC "-DKONAN_OSX=1")
target_compile_definitions(runtime PUBLIC "-DKONAN_MACOSX=1")
target_compile_definitions(runtime PUBLIC "-DKONAN_X64=1")
target_compile_definitions(runtime PUBLIC "-DKONAN_OBJC_INTEROP")
target_compile_definitions(runtime PUBLIC "-DKONAN_CORE_SYMBOLICATION=1")
target_compile_definitions(runtime PUBLIC "-DKONAN_HAS_CXX11_EXCEPTION_FUNCTIONS=1")
target_compile_definitions(runtime PUBLIC "-DKONAN_REPORT_BACKTRACE_TO_IOS_CRASH_LOG=1")


