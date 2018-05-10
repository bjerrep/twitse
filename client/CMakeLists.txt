cmake_minimum_required(VERSION 2.8.12)

project(client)

find_package(Qt5Core)
find_package(Qt5Network)

option(VCTCXO "DAC and voltage controlled processor crystal" OFF)

if (VCTCXO)
    message(STATUS "client DAC/VCTCXO enabled")
    add_definitions(-DVCTCXO)
    file(GLOB vctcxo_sources src/i2c_access*)
endif()

file(GLOB sources src/client* src/main.cpp)

include_directories(
    ../util/src
    ../network/src
    ../external/spdlog/include
    )

add_executable(
    ${PROJECT_NAME}
    ${sources}
    ${vctcxo_sources}
    )

target_link_libraries(
    ${PROJECT_NAME}
    network
    util
    Qt5::Core
    Qt5::Network
    pthread
    )
