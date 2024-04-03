project(util)

find_package(Qt5Core)
find_package(Qt5Network)

file(GLOB sources src/*)

if ("${VCTCXO}" STREQUAL "OFF")
    message("note that building for software mode will require i2c (allthough it wont be used)")
endif()

include_directories(
    ../external/spdlog/include
    )

add_library(
    ${PROJECT_NAME}
    ${sources}
    )

target_link_libraries(
    ${PROJECT_NAME}
    Qt5::Core
    Qt5::Network
    )
