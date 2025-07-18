cmake_minimum_required(VERSION 3.30)

set(CMAKE_TOOLCHAIN_FILE
        "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")

project(td365)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# clang has no documentation installed on MacOS
# https://releases.llvm.org/4.0.0/tools/clang/docs/DiagnosticsReference.html#wall
add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wconversion -fno-omit-frame-pointer)

file(GLOB_RECURSE SOURCES src/*.cpp)
add_library(td365 SHARED ${SOURCES})
add_library(td365_static STATIC ${SOURCES})
set_target_properties(td365_static PROPERTIES OUTPUT_NAME td365)

set(Boost_VERBOSE ON)
set(Boost_DEBUG ON)
set(Boost_USE_STATIC_LIBS ON) # only find static libs
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF) # do not look for boost libraries linked against static C++ std lib

find_package(Boost REQUIRED COMPONENTS system thread url iostreams charconv)
find_package(nlohmann_json REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

find_package(spdlog CONFIG REQUIRED)

target_include_directories(td365 PUBLIC src ${Boost_INCLUDE_DIRS})
target_link_libraries(td365 PUBLIC ${Boost_LIBRARIES} ZLIB::ZLIB OpenSSL::SSL OpenSSL::Crypto nlohmann_json::nlohmann_json spdlog::spdlog_header_only)

target_include_directories(td365_static PUBLIC src ${Boost_INCLUDE_DIRS})
target_link_libraries(td365_static PUBLIC ${Boost_LIBRARIES} ZLIB::ZLIB OpenSSL::SSL OpenSSL::Crypto nlohmann_json::nlohmann_json spdlog::spdlog_header_only)

add_executable(trade_test examples/trade_test.cpp)
target_include_directories(trade_test PUBLIC td365)
target_link_libraries(trade_test PUBLIC td365)

target_compile_options(trade_test PUBLIC -fsanitize=address -g -O1)
target_link_options(trade_test PUBLIC -fsanitize=address)


# ----------------------------------------------------------------------------
enable_testing()

# requires Catch2 to be installed via vcpkg: vcpkg install catch2
find_package(Catch2 CONFIG REQUIRED)

add_executable(td365_tests
        tests/test_parsing.cpp
        tests/test_ws_reconnect.cpp
)

target_link_libraries(td365_tests PRIVATE
        td365_static
        Catch2::Catch2WithMain
)

# auto-discover any Catch2 TEST_CASEs (optional)
include(CTest)
include(Catch)

catch_discover_tests(td365_tests
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        REPORT_QUIET
)
