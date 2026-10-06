# pzworld: the Panzers world layer (SWorld, map loading, units, triggers),
# lifted from the HD exe. SGameLogic moved to src/game (pzgame) in M2-P0.
# S.W.I.N.E. world/ code may be adapted where it matches HD (M2 permission;
# mark it "// SWINE-adapted, PANZERS 0xADDR").
# See docs/MENU3D_INTERFACES.md and docs/M2_INTERFACES.md for owners.
add_library(pzworld STATIC
    worldapi.h
    worldapi.cpp
    world.h
    world.cpp
    doodad.h
    mapload.cpp
    pzunitregistry.h
    unitregistry.cpp
    unit.cpp
    worldunits.cpp
    trigger.h
    trigger.cpp
    # P: block map + A*
    blockmap.h
    blockmap.cpp
    astar.h
    astar.cpp
    blockmaprefresh.h
    blockmaprefresh.cpp
    # M3: selection (agent O), AI (agent C), load extras (agent F)
    selection.h
    selection.cpp
    ai.cpp
)
target_include_directories(pzworld PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(pzworld PUBLIC ${PANZERS_SRC_DIR}/game)
target_include_directories(pzworld PRIVATE ${PANZERS_SRC_DIR}/stubs)
target_link_libraries(pzworld PUBLIC pz3d core pzgame)
