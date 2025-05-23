cmake_minimum_required(VERSION 3.30)
project(td365)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# clang has no documentation installed on MacOS
# https://releases.llvm.org/4.0.0/tools/clang/docs/DiagnosticsReference.html#wall
add_compile_options(-Wall -Wextra -Wpedantic -fno-omit-frame-pointer)

file(GLOB_RECURSE SOURCES src/*.cpp)
add_library(td365 SHARED ${SOURCES})
add_library(td365_static STATIC ${SOURCES})
set_target_properties(td365_static PROPERTIES OUTPUT_NAME td365)

set(Boost_VERBOSE ON)
set(Boost_DEBUG ON)
set(Boost_USE_STATIC_LIBS ON) # only find static libs
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF) # do not look for boost libraries linked against static C++ std lib

find_package(Boost REQUIRED COMPONENTS system thread)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(ZLIB REQUIRED)

target_include_directories(td365 PUBLIC src ${Boost_INCLUDE_DIRS})
target_link_libraries(td365 PUBLIC ${Boost_LIBRARIES} ZLIB::ZLIB OpenSSL::SSL OpenSSL::Crypto nlohmann_json::nlohmann_json)

target_include_directories(td365_static PUBLIC src ${Boost_INCLUDE_DIRS})
target_link_libraries(td365_static PUBLIC ${Boost_LIBRARIES} ZLIB::ZLIB OpenSSL::SSL OpenSSL::Crypto nlohmann_json::nlohmann_json)

add_executable(subscribe_test examples/subscribe_test.cpp)
target_include_directories(subscribe_test PUBLIC td365)
target_link_libraries(subscribe_test PUBLIC td365)
