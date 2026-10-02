# stubs — explicit stand-ins for symbols the imported engine code references
# but whose real definitions live in SWINE gameplay code (world/, game/,
# network/) that is NOT imported, or in libraries we chose not to build.
# Every function stub must call STUB_LOG("Name") so it is counted by
# tools/census.py and logs its name once. See docs/IMPORT_NOTES.md.
add_library(stubs STATIC
    stubs.cpp
    stub_globals.cpp
    stub_window.cpp
)
target_include_directories(stubs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(stubs PRIVATE ${PANZERS_SRC_DIR}/sound)
target_link_libraries(stubs PUBLIC core window)
