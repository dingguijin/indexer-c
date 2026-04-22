cmake_minimum_required(VERSION 3.16)
project(indexer_c C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_compile_options(
    -Wall -Wextra -Wpedantic -Werror -Wformat=2 -Wshadow
    -Wstrict-prototypes -Wmissing-prototypes
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-fstack-protector-strong -D_FORTIFY_SOURCE=2)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(CURL    REQUIRED IMPORTED_TARGET libcurl)
pkg_check_modules(HIREDIS REQUIRED IMPORTED_TARGET hiredis)
find_package(OpenSSL REQUIRED)

add_executable(indexer
    src/main.c
    src/log.c
    src/config.c
    src/chain.c
    src/rpc.c
    src/redisio.c
    src/head_follower.c
    src/util.c
    third_party/yyjson/yyjson.c
    third_party/tomlc99/toml.c
)

target_include_directories(indexer PRIVATE
    include
    third_party/yyjson
    third_party/tomlc99
)

target_link_libraries(indexer PRIVATE
    PkgConfig::CURL
    PkgConfig::HIREDIS
    OpenSSL::SSL
    OpenSSL::Crypto
    pthread
)

set_target_properties(indexer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

enable_testing()
add_subdirectory(tests)