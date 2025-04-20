cmake_minimum_required(VERSION 3.15)
project(C-Matchday C)
set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(MBEDTLS REQUIRED mbedtls)
find_package(Threads REQUIRED)

# Includes globales para el proyecto
include_directories(
    include
    vendor/cJSON
    vendor/mongoose
    ${CURL_INCLUDE_DIRS}
    ${MBEDTLS_INCLUDE_DIRS}
)

link_directories(
    ${CURL_LIBRARY_DIRS}
    ${MBEDTLS_LIBRARY_DIRS}
)

# Fuentes principales
set(MAIN_SOURCES
    src/main.c
    src/server.c
    src/auth.c
    src/football_api.c
    vendor/cJSON/cJSON.c
    vendor/mongoose/mongoose.c
)

# Ejecutable principal
add_executable(cmatchday ${MAIN_SOURCES})
target_link_libraries(cmatchday
    ${CURL_LIBRARIES}
    ${MBEDTLS_LIBRARIES}
    Threads::Threads
)

# Habilitar tests
enable_testing()

# Test de autenticación
add_executable(test_auth
    tests/test_auth.c
    src/auth.c
    vendor/cJSON/cJSON.c
)
target_include_directories(test_auth PRIVATE
    include
    vendor/cJSON
)
target_link_libraries(test_auth
    ${CURL_LIBRARIES}
)
add_test(NAME AuthTests COMMAND test_auth)

# Test de API de fútbol
add_executable(test_api
    tests/test_api.c
    src/football_api.c
    vendor/cJSON/cJSON.c
)
target_include_directories(test_api PRIVATE
    include
    vendor/cJSON
)
target_link_libraries(test_api
    ${CURL_LIBRARIES}
)
add_test(NAME ApiTests COMMAND test_api)