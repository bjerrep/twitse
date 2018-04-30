cmake_minimum_required(VERSION 2.8.12)

project(dataanalyse)

find_package(Qt5Core)
    
file(GLOB_RECURSE sources src/*)

set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g")

include_directories(
    ../util/src
    ../client/src
    ../network/src
    ../external/spdlog/include
    )

add_executable(
    ${PROJECT_NAME}
    ${sources}
    )

target_link_libraries(
    ${PROJECT_NAME}
    util
    Qt5::Core
    pthread
    )
