# panzers — the lifted Panzers game layer: app shell (SSuperWindow, GameMain,
# SSettings) and the main menu. bink.* is built separately as panzers_bink
# (top-level CMakeLists.txt).
add_library(panzersgame STATIC
    credits.cpp
    credits.h
    gamemain.cpp
    mainmenu.cpp
    mainmenu.h
    menuskin.cpp
    optionsmenu.cpp
    optionsmenu.h
    pzboard.h
    pzwidgets.cpp
    pzwidgets.h
    settings.cpp
    settings.h
    superwindow.cpp
    superwindow.h
    # M3 skeleton (docs/M3_INTERFACES.md)
    superwindow_m3.cpp
    gameview.h
    gameview.cpp
    hud.h
    hud.cpp
    ingamemenu.h
    ingamemenu.cpp
    market.h
    market.cpp
    trainingmenu.h
    trainingmenu.cpp
)
target_include_directories(panzersgame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(panzersgame PRIVATE ${PANZERS_SRC_DIR}/sound ${PANZERS_SRC_DIR}/stubs)
target_link_libraries(panzersgame PUBLIC core window common sound panzers_bink pz3d pzworld)
target_compile_definitions(panzersgame PRIVATE PANZERS_HAVE_BINK=1)
# MOD_WIDESCREEN (src/core/mods.h): only in the mod build.
if(PANZERS_MOD_WIDESCREEN)
    target_sources(panzersgame PRIVATE mod_widescreen.cpp mod_widescreen.h)
endif()
