cmake_minimum_required(VERSION 3.10)
project(ServiceSystemTests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включение тестов
enable_testing()

# Поиск GoogleTest в системе
find_package(GTest REQUIRED)

# Простой тест - только структуры данных, без внешних зависимостей
add_executable(test_simple test_simple.cpp)
target_link_libraries(test_simple
    GTest::GTest
    pthread
)
gtest_discover_tests(test_simple)

# Копирование тестовой конфигурации
configure_file(config_test.json ${CMAKE_CURRENT_BINARY_DIR}/config_test.json COPYONLY)

