cmake_minimum_required(VERSION 3.15)
project(C-Matchday C)
set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)

# libcurl
pkg_check_modules(CURL REQUIRED libcurl)

# Mbed TLS via pkg-config
pkg_check_modules(MBEDTLS REQUIRED mbedtls)

# Include paths
include_directories(
    ${CURL_INCLUDE_DIRS}
    ${MBEDTLS_INCLUDE_DIRS}
    include
)

# Library search paths
link_directories(
    ${CURL_LIBRARY_DIRS}
    ${MBEDTLS_LIBRARY_DIRS}
)

# Main executable
add_executable(cmatchday
    src/main.c
    src/server.c
    src/auth.c
    src/football_api.c
)

target_link_libraries(cmatchday
    ${CURL_LIBRARIES}
    ${MBEDTLS_LIBRARIES}
)

# Enable and configure tests
enable_testing()

# Auth tests
add_executable(test_auth
    tests/test_auth.c
    src/auth.c
)
target_link_libraries(test_auth
    ${CURL_LIBRARIES}
)

add_test(NAME AuthTests COMMAND test_auth)

# API tests
add_executable(test_api
    tests/test_api.c
    src/football_api.c
)
target_link_libraries(test_api
    ${CURL_LIBRARIES}
)

add_test(NAME ApiTests COMMAND test_api)