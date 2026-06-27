cmake_minimum_required(VERSION 3.15)

project(backtest_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build commands:
#   Linux/macOS: mkdir build && cd build && cmake .. && make
#   Windows:     mkdir build && cd build && cmake .. && cmake --build . --config Release
#
# pybind11 installation (choose one):
#   pip install pybind11
#     Then ensure CMake can find it, e.g.:
#     cmake .. -Dpybind11_DIR=$(python3 -m pybind11 --cmakedir)
#   macOS:  brew install pybind11
#   Ubuntu/Debian: sudo apt install pybind11-dev
#   vcpkg:  vcpkg install pybind11
#
# If pybind11 is not installed system-wide, this project falls back to FetchContent
# (see below) and downloads pybind11 automatically during configure.

# ---------------------------------------------------------------------------
# Python (required for pybind11 module; helps on Windows)
# ---------------------------------------------------------------------------
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

# ---------------------------------------------------------------------------
# pybind11
#
# Option A (recommended when installed): find_package(pybind11 CONFIG REQUIRED)
#   cmake .. -Dpybind11_DIR=$(python3 -m pybind11 --cmakedir)
#
# Option B (fallback): FetchContent downloads pybind11 when Option A is not found
# ---------------------------------------------------------------------------
find_package(pybind11 CONFIG QUIET)

if(NOT pybind11_FOUND)
    message(STATUS "pybind11 not found via find_package; fetching with FetchContent")
    include(FetchContent)
    FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG v2.13.6
    )
    FetchContent_MakeAvailable(pybind11)
endif()

# ---------------------------------------------------------------------------
# Shared sources
# ---------------------------------------------------------------------------
set(CORE_ENGINE_SOURCES
    src/data/DataLoader.cpp
    src/engine/CoreEngine.cpp
    src/engine/EventScheduler.cpp
    src/engine/OrderManager.cpp
    src/engine/PortfolioManager.cpp
    src/engine/FixedLatencyModel.cpp
    src/engine/GaussianLatencyModel.cpp
    src/engine/FixedSlippageModel.cpp
    src/engine/VolumeProportionalSlippageModel.cpp
    src/engine/RealisticMatchingEngine.cpp
    src/engine/IdealMatchingEngine.cpp
    src/analytics/PerformanceAnalytics.cpp
)

set(PROJECT_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/data
    ${CMAKE_CURRENT_SOURCE_DIR}/include/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/include/analytics
)

function(backtest_set_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -pedantic)
    endif()
endfunction()

# ---------------------------------------------------------------------------
# Python extension module -> ${PROJECT_SOURCE_DIR}/python/
# ---------------------------------------------------------------------------
set(PYTHON_MODULE_DIR ${CMAKE_SOURCE_DIR}/python)
file(MAKE_DIRECTORY ${PYTHON_MODULE_DIR})

pybind11_add_module(_backtest_core
    src/binding/pybind_module.cpp
    ${CORE_ENGINE_SOURCES}
)

target_include_directories(_backtest_core PRIVATE ${PROJECT_INCLUDE_DIRS})

set_target_properties(_backtest_core PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PYTHON_MODULE_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${PYTHON_MODULE_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${PYTHON_MODULE_DIR}
)

foreach(config Debug Release RelWithDebInfo MinSizeRel)
    set_target_properties(_backtest_core PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY_${config} ${PYTHON_MODULE_DIR}
        RUNTIME_OUTPUT_DIRECTORY_${config} ${PYTHON_MODULE_DIR}
        ARCHIVE_OUTPUT_DIRECTORY_${config} ${PYTHON_MODULE_DIR}
    )
endforeach()

backtest_set_warnings(_backtest_core)

# ---------------------------------------------------------------------------
# Optional C++ test executables (disable with -DBUILD_TESTS=OFF)
# ---------------------------------------------------------------------------
option(BUILD_TESTS "Build C++ test executables" ON)

if(BUILD_TESTS)
    add_executable(test_dataloader
        src/data/DataLoader.cpp
        tests/test_dataloader.cpp
    )
    target_include_directories(test_dataloader PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/data
    )
    backtest_set_warnings(test_dataloader)

    add_executable(test_core_engine
        ${CORE_ENGINE_SOURCES}
        tests/test_core_engine.cpp
    )
    target_include_directories(test_core_engine PRIVATE ${PROJECT_INCLUDE_DIRS})
    target_link_libraries(test_core_engine PRIVATE pybind11::embed)
    backtest_set_warnings(test_core_engine)
endif()
