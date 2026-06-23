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

if(MSVC)
    target_compile_options(test_dataloader PRIVATE /W4)
else()
    target_compile_options(test_dataloader PRIVATE -Wall -Wextra -pedantic)
endif()
