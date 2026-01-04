cmake_minimum_required(VERSION 3.10)
project(ServiceManagementSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Флаги компиляции без -Werror
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Поиск Boost с необходимыми компонентами
find_package(Boost 1.66 REQUIRED COMPONENTS system thread random date_time)

# Поиск libpqxx
find_path(PQXX_INCLUDE_DIR pqxx/pqxx
    PATHS
    /usr/include
    /usr/local/include
    REQUIRED
)

find_library(PQXX_LIBRARY NAMES pqxx
    PATHS
    /usr/lib
    /usr/lib/x86_64-linux-gnu
    /usr/local/lib
    REQUIRED
)

# Поиск потоков
find_package(Threads REQUIRED)

# Находим Crow в системе или используем локальный
find_path(CROW_INCLUDE_DIR crow.h
    PATHS
    /usr/include
    /usr/local/include
    /usr/include/crow
    /usr/local/include/crow
    ${CMAKE_SOURCE_DIR}/../crow/Crow/include
)

if(NOT CROW_INCLUDE_DIR)
    message(FATAL_ERROR "Crow not found! Please install Crow or place it in ../crow/Crow/include")
endif()

# nlohmann/json (header-only, use find_path)
find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp
    PATHS
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

# Исполняемый файл
add_executable(service_system ${SOURCES})

# Подключение заголовочных файлов
target_include_directories(service_system PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CROW_INCLUDE_DIR}
    ${PQXX_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${NLOHMANN_JSON_INCLUDE_DIR}
)

# Для Crow нужно определить макрос CROW_USE_BOOST
target_compile_definitions(service_system PRIVATE
    CROW_USE_BOOST
    _GLIBCXX_USE_CXX11_ABI=1
)

# Подключение библиотек
target_link_libraries(service_system PRIVATE
    Threads::Threads
    ${PQXX_LIBRARY}
    pq
    Boost::system
    Boost::thread
    Boost::random
    Boost::date_time
)

# Копирование статических файлов
configure_file(config.json ${CMAKE_CURRENT_BINARY_DIR}/config.json COPYONLY)

# Копируем www папку если она существует
if(EXISTS ${CMAKE_SOURCE_DIR}/www)
    file(COPY ${CMAKE_SOURCE_DIR}/www DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
else()
    # Создаем простую www папку
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/www)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/www/index.html "<html><body><h1>Service Management System</h1><p>Server is running!</p></body></html>")
endif()

message(STATUS "Crow include dir: ${CROW_INCLUDE_DIR}")
message(STATUS "Boost include dir: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost libraries: ${Boost_LIBRARIES}")
message(STATUS "PQXX library: ${PQXX_LIBRARY}")