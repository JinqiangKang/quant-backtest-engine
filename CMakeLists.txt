cmake_minimum_required(VERSION 3.15)

project(backtest_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build commands:
#   Linux/macOS: mkdir build && cd build && cmake .. && make
#   Windows:     mkdir build && cd build && cmake ..  (then open in Visual Studio)

add_executable(test_dataloader
    src/data/DataLoader.cpp
    tests/test_dataloader.cpp
)

target_include_directories(test_dataloader PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/data
)

set(CORE_ENGINE_SOURCES
    src/data/DataLoader.cpp
    src/engine/CoreEngine.cpp
    src/engine/EventScheduler.cpp
    src/engine/OrderManager.cpp
    src/engine/PortfolioManager.cpp
)

add_executable(test_core_engine
    ${CORE_ENGINE_SOURCES}
    tests/test_core_engine.cpp
)

target_include_directories(test_core_engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/data
    ${CMAKE_CURRENT_SOURCE_DIR}/include/engine
)

if(MSVC)
    target_compile_options(test_core_engine PRIVATE /W4)
else()
    target_compile_options(test_core_engine PRIVATE -Wall -Wextra -pedantic)
endif()

if(MSVC)
    target_compile_options(test_dataloader PRIVATE /W4)
else()
    target_compile_options(test_dataloader PRIVATE -Wall -Wextra -pedantic)
endif()
