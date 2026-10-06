# pzgame: the Panzers game logic for M2 (SGameLogic, triggers, units,
# drivers, unit animations), lifted from the HD exe. S.W.I.N.E. world/ and
# game/ code may be adapted where it matches HD (mark it
# "// SWINE-adapted, PANZERS 0xADDR"). See docs/M2_INTERFACES.md for owners.
add_library(pzgame STATIC
    # P0 (shared)
    m2common.h
    m2trace.cpp
    iunit.h
    idriver.h
    iunitanim.h
    gamelogic.h
    # L: logic + triggers
    gamelogic.cpp
    triggers.cpp
    movementgroup.cpp
    # U: units + prototypes
    unit.h
    unit.cpp
    punit.h
    punit.cpp
    # P: drivers + path finding
    driver.h
    driver.cpp
    # A: unit animation
    unitanim.h
    unitanim.cpp
)
target_include_directories(pzgame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(pzgame PRIVATE ${PANZERS_SRC_DIR}/stubs)
# pzworld and pzgame call each other (SGameLogic <-> SWorld); CMake repeats
# cyclic static libraries on the link line.
target_link_libraries(pzgame PUBLIC pz3d pzworld core)
