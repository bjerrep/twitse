cmake_minimum_required(VERSION 2.8.12)

project(util)

find_package(Qt5Core)
find_package(Qt5Network)

file(GLOB sources src/*.cpp src/*.h)

if (VCTCXO)
    file(GLOB vctcxo_sources src/systemtime/systemtime_vctcxo.cpp)
else()
    file(GLOB vctcxo_sources src/systemtime/systemtime_std.cpp)
endif()

include_directories(
    ../external/spdlog/include
    )

add_library(
    ${PROJECT_NAME}
    ${sources}
    ${vctcxo_sources}
    )

target_link_libraries(
    ${PROJECT_NAME}
    Qt5::Core
    Qt5::Network
    )

