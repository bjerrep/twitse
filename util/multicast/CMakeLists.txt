cmake_minimum_required(VERSION 2.8.12)

project(multicast)

find_package(Qt5Core)
find_package(Qt5Network)

include_directories(
    ../../util/src
    ../../network/src
    ../../../external/spdlog/include
    )

add_executable(
    ${PROJECT_NAME}_tx
    transmitter.cpp
    )

target_link_libraries(
    ${PROJECT_NAME}_tx
    network
    util
    Qt5::Core
    Qt5::Network
    pthread
    )

add_executable(
    ${PROJECT_NAME}_rx
    receiver.cpp
    )

target_link_libraries(
    ${PROJECT_NAME}_rx
    network
    util
    Qt5::Core
    Qt5::Network
    pthread
    )

add_custom_command(
    TARGET ${PROJECT_NAME}_tx POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}_tx> ${TWITSEROOT}/bin/${PROJECT_NAME}_tx
    )

add_custom_command(
    TARGET ${PROJECT_NAME}_rx POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}_rx> ${TWITSEROOT}/bin/${PROJECT_NAME}_rx
    )
