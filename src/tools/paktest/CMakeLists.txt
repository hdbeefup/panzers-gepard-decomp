# paktest — console check of the file system, ini parser and menu.ini string
# table against a game directory. Usage: paktest <rundir> [-log]
add_executable(paktest paktest.cpp)
target_link_libraries(paktest PRIVATE core)
if(MSVC)
    target_link_options(paktest PRIVATE /SUBSYSTEM:CONSOLE)
endif()
