# pz3d: the HD (Panzers) 3D path for the main-menu background: interfaces in
# HD vtable order (i*.h) and their implementations (pz*.cpp). See
# docs/MENU3D_INTERFACES.md for owners and rules.
add_library(pz3d STATIC
    pzcommon.h
    pztrace.cpp
    igepardhd.h
    iviewport.h
    iscene.h
    imodel.h
    imesh.h
    iterrain.h
    ipixie.h
    pzgepard.h
    pzgepard.cpp
    pzviewport.h
    pzviewport.cpp
    pzscene.h
    pzscene.cpp
    pzmodel.h
    pzmodel.cpp
    pzterrain.h
    pzterrain.cpp
    parcel.h
    parcel.cpp
    road.cpp
    decal.cpp
    pzpixie.h
    pzpixie.cpp
)
target_include_directories(pz3d PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(pz3d PRIVATE ${PANZERS_SRC_DIR}/stubs)
target_compile_definitions(pz3d PRIVATE _USE_MATH_DEFINES PZ_MENU_WORLD=$<BOOL:${PZ_MENU_WORLD}>)
target_compile_options(pz3d PRIVATE /FI"${PANZERS_SRC_DIR}/3dengine/dxsdk_math_fix.h")
target_link_libraries(pz3d PUBLIC 3dengine core)
