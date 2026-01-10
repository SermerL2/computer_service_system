cmake_minimum_required(VERSION 3.10)
project(ServiceManagementSystem)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Флаги компиляции - warnings as errors disabled для libpqxx совместимости
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-error")

# Поиск Boost с необходимыми компонентами
find_package(Boost 1.67 REQUIRED COMPONENTS system thread random date_time)

# Поиск libcurl для отправки логов в Loki
find_package(CURL REQUIRED)

# Поиск libpqxx - сначала через pkg-config, затем локальная версия
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX pqxx)

if(PQXX_FOUND)
    message(STATUS "Using system libpqxx from pkg-config")
    set(PQXX_INCLUDE_DIRS "${PQXX_INCLUDE_DIRS}")
    set(PQXX_LIBRARIES "${PQXX_LIBRARIES}")
else()
    message(STATUS "System libpqxx not found, using local version from project")
    set(LIBPQXX_ROOT "${CMAKE_SOURCE_DIR}/libpqxx-7.7.5")
    set(LIBPQXX_INCLUDE_DIR "${LIBPQXX_ROOT}/include")
    
    # Проверяем, установлена ли локальная версия в /usr/local
    if(EXISTS "/usr/local/lib/libpqxx.a")
        message(STATUS "Found local libpqxx at: /usr/local/lib/libpqxx.a")
        set(PQXX_INCLUDE_DIRS "/usr/local/include")
        set(PQXX_LIBRARIES "/usr/local/lib/libpqxx.a")
    elseif(EXISTS "${LIBPQXX_ROOT}/src/.libs/libpqxx.a")
        message(STATUS "Found local libpqxx at: ${LIBPQXX_ROOT}/src/.libs/libpqxx.a")
        set(PQXX_INCLUDE_DIRS "${LIBPQXX_INCLUDE_DIR}")
        set(PQXX_LIBRARIES "${LIBPQXX_ROOT}/src/.libs/libpqxx.a")
    else()
        message(FATAL_ERROR "libpqxx not found! Install libpqxx-dev or ensure local version is built.")
    endif()
endif()

# Для libpq (PostgreSQL C interface) используем системный
pkg_check_modules(PQ REQUIRED libpq)

message(STATUS "PQXX include dir: ${PQXX_INCLUDE_DIRS}")
message(STATUS "PQXX library: ${PQXX_LIBRARIES}")
message(STATUS "PQ include dir: ${PQ_INCLUDE_DIRS}")
message(STATUS "PQ library: ${PQ_LIBRARIES}")

# Поиск потоков
find_package(Threads REQUIRED)

# Находим Crow
set(CROW_PATHS
    ${CMAKE_SOURCE_DIR}/Crow/include
    /usr/include
    /usr/local/include
)

foreach(path IN LISTS CROW_PATHS)
    if(EXISTS "${path}/crow.h")
        set(CROW_INCLUDE_DIR "${path}")
        break()
    endif()
endforeach()

if(NOT CROW_INCLUDE_DIR)
    message(FATAL_ERROR "Crow not found!")
endif()

# nlohmann/json
find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp
    PATHS
    ${CMAKE_SOURCE_DIR}/include
    /usr/include
    /usr/local/include
    REQUIRED
)

# Исходные файлы
set(SOURCES
    src/main.cpp
    src/database.cpp
    src/webserver.cpp
)

add_executable(service_system ${SOURCES})

target_include_directories(service_system PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CROW_INCLUDE_DIR}
    ${PQXX_INCLUDE_DIRS}
    ${PQ_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${NLOHMANN_JSON_INCLUDE_DIR}
)

target_compile_definitions(service_system PRIVATE
    CROW_USE_BOOST
    _GLIBCXX_USE_CXX11_ABI=1
    DBL_DECIMAL_DIG=17
)

target_link_libraries(service_system PRIVATE
    Threads::Threads
    ${PQXX_LIBRARIES}
    ${PQ_LIBRARIES}
    Boost::system
    Boost::thread
    Boost::random
    Boost::date_time
    ${CURL_LIBRARY}
)

configure_file(config.json ${CMAKE_CURRENT_BINARY_DIR}/config.json COPYONLY)

if(EXISTS ${CMAKE_SOURCE_DIR}/www)
    file(COPY ${CMAKE_SOURCE_DIR}/www DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
else()
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/www)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/www/index.html "<html><body><h1>Service Management System</h1><p>Server is running!</p></body></html>")
endif()

message(STATUS "Build configuration complete")

