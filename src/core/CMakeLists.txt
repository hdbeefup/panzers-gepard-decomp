# core — Engine core library
add_library(core STATIC
    logger.cpp
    properties.cpp
    stream.cpp
    string2.cpp
    timer.cpp
)

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/chaos
)
