# pzgame: the Panzers game logic for M2 (SGameLogic, triggers, units,
# drivers, unit animations), lifted from the HD exe. S.W.I.N.E. world/ and
# game/ code may be adapted where it matches HD (mark it
# "// SWINE-adapted, PANZERS 0xADDR"). See docs/M2_INTERFACES.md for owners.
add_library(pzgame STATIC
    # P0 (shared)
    m2common.h
    m2trace.cpp
    # M3-P0 (shared): docs/M3_INTERFACES.md
    m3common.h
    m3trace.cpp
    m3deepcap.cpp
    # M3 F: campaign, save; O: packets
    campaign.h
    campaign.cpp
    packets.h
    packets.cpp
    packets_rec.h
    packets_rec.cpp
    unitsave.h
    unitsave.cpp
    # M4 S: save / load
    savedesc.cpp
    unit_afterload.cpp
    gamelogic_save.cpp
    world_save.cpp
    campaign_save.cpp
    iunit.h
    idriver.h
    iunitanim.h
    gamelogic.h
    # L: logic + triggers
    gamelogic.cpp
    gamelogic_mission.cpp
    triggers.cpp
    triggers_script.cpp
    cutscene.h
    cutscene.cpp
    movementgroup.cpp
    logicextra.cpp
    # U: units + prototypes
    unit.h
    unit.cpp
    punit.h
    punit.cpp
    unitprops.h
    unitprops.cpp
    unitschema.cpp
    unitextern.h
    unitextern.cpp
    unitbase.cpp
    unitai.cpp
    singleunit.h
    singleunit.cpp
    squadunit.h
    squadunit.cpp
    squadrefresh.cpp
    squadcombat.cpp
    buildingunit.h
    buildingunit.cpp
    gunner.h
    gunner.cpp
    gunnermath.cpp
    # P: drivers + path finding
    driver.h
    driver.cpp
    drivermath.h
    drivermath.cpp
    drivertypes.h
    driverunit.h
    driverunit.cpp
    driverpath.cpp
    driverghost.cpp
    target.h
    target.cpp
    manoeuvre.h
    driverextern.cpp
    manoeuvre.cpp
    # A: unit animation
    unitanim.h
    unitanim.cpp
    unitanim_vehicle.cpp
    unitanim_model.h
    unitanim_model.cpp
    unitanim_props.cpp
    unitanim_world.cpp
    # M3 C: combat + AI (docs/M3_INTERFACES.md row C). C0 integrator, C1 gunner /
    # projectile, C2 damage / death, C3 air support / waster, C4 AI / squads / buildings.
    combat_speech.cpp
    combat.cpp
    combat_support.cpp
    projectile.cpp
    projectile_driver.cpp
    projectile_anim.cpp
    waster.cpp
    flying.cpp
    flyingdriver.cpp
    # M4 TR: trains (class 10) on the ROD2 roads
    rail.h
    rail.cpp
    trainunit.h
    trainunit.cpp
    traindriver.h
    traindriver.cpp
    trainanim.h
    trainanim.cpp
    parachute.cpp
)
target_include_directories(pzgame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(pzgame PRIVATE ${PANZERS_SRC_DIR}/stubs)
# pzworld and pzgame call each other (SGameLogic <-> SWorld); CMake repeats
# cyclic static libraries on the link line.
target_link_libraries(pzgame PUBLIC pz3d pzworld core)
