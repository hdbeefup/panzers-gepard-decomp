# platform — platform abstraction layer (GOG Galaxy, Steam, or none)
set(PLATFORM_BACKEND "NONE" CACHE STRING "Platform backend (NONE, GOG, STEAM)")
set_property(CACHE PLATFORM_BACKEND PROPERTY STRINGS NONE GOG STEAM)

set(PLATFORM_SOURCES platform.cpp)

if(PLATFORM_BACKEND STREQUAL "GOG")
    list(APPEND PLATFORM_SOURCES gogmanager.cpp)
endif()

add_library(platform STATIC ${PLATFORM_SOURCES})
target_include_directories(platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(platform PUBLIC core)

if(NOT PLATFORM_BACKEND STREQUAL "NONE")
    target_compile_definitions(platform PUBLIC PLATFORM_${PLATFORM_BACKEND}=1)
endif()
