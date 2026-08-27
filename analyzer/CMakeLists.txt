cmake_minimum_required(VERSION 3.20)
project(zephyr-crash-analyzer-static C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Found Clang in: ${CLANG_CMAKE_DIR}")

include_directories(${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES
    src/main.cpp
    src/AnalysisUtils.cpp
    src/IPCCheckers.cpp
    src/Visitors.cpp
    src/Model.cpp
    src/GraphBuilder.cpp
    src/ImpactAnalyzer.cpp
    src/ReportGenerator.cpp
)

add_executable(zephyr-crash-analyzer-static ${SOURCES})

target_compile_options(zephyr-crash-analyzer-static PRIVATE
    ${LLVM_COMPILE_FLAGS}
    -Wall -Wextra
)

target_link_libraries(zephyr-crash-analyzer-static
    clang-cpp
    LLVM-22
    pthread
)

install(TARGETS zephyr-crash-analyzer-static DESTINATION bin)
