cmake_minimum_required(VERSION 2.8.12)

project(twitse_client)

find_package(Qt5Core)
find_package(Qt5Network)

file(GLOB sources src/*)

include_directories(
    ../util/src
    ../network/src
    ../external/spdlog/include
    )

add_executable(
    ${PROJECT_NAME}
    ${sources}
    )

target_link_libraries(
    ${PROJECT_NAME}
    network
    util
    Qt5::Core
    Qt5::Network
    pthread
    )

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${TWITSEROOT}/bin/${PROJECT_NAME}
    )
