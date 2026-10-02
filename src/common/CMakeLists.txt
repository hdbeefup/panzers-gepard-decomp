# common — Shared game settings
add_library(common STATIC
    options.cpp
    keybinds.cpp
    swineversion.cpp
    modmanager.cpp
    unitregistry.cpp
)
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(common PUBLIC core)
