# pzworld: the Panzers world layer (SWorld, map loading, SGameLogic, units),
# lifted from the HD exe only. The S.W.I.N.E. decomp's world/ and game/ code
# is private and must never be imported or copied here.
# See docs/MENU3D_INTERFACES.md (owner: agent D).
add_library(pzworld STATIC
    worldapi.h
    worldapi.cpp
    world.h
    world.cpp
    gamelogic.h
    gamelogic.cpp
)
target_include_directories(pzworld PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(pzworld PRIVATE ${PANZERS_SRC_DIR}/stubs)
target_link_libraries(pzworld PUBLIC pz3d core)
