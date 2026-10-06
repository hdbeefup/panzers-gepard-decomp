# recdump — parses a -packetrec recording (.rec) with the game's frame reader
# and record table (src/game/packets_rec.cpp), dumps every packet as text and
# writes the file back field by field (round-trip check, docs/M3_REPLAY.md).
# Usage: recdump <in.rec> [<dump.txt> [<out.rec>]]
add_executable(recdump recdump.cpp ${PANZERS_SRC_DIR}/game/packets_rec.cpp)
target_include_directories(recdump PRIVATE ${PANZERS_SRC_DIR}/game)
target_link_libraries(recdump PRIVATE core)
if(MSVC)
    target_link_options(recdump PRIVATE /SUBSYSTEM:CONSOLE)
endif()
