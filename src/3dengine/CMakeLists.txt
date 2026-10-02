# 3dengine — Renderer, terrain, effects, particles
add_library(3dengine STATIC
    animation.cpp
    blt_converters.cpp
    board.cpp
    effects.cpp
    fontbitmap.cpp
    gepard.cpp
    glow.cpp
    group.cpp
    mesh.cpp
    parcel.cpp
    particles.cpp
    particles2.cpp
    particles3.cpp
    particles4.cpp
    spline.cpp
    stb_image_stubs.cpp
    terrain.cpp
    texture.cpp
)

target_include_directories(3dengine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Fix sqrtf error in d3dx9math.inl with newer MSVC
# The June 2010 DirectX SDK predates the UCRT header reorganization.
# Force-include math.h so sqrtf/sinf/cosf are visible before d3dx9math.inl.
target_compile_definitions(3dengine PRIVATE _USE_MATH_DEFINES)
target_compile_options(3dengine PRIVATE /FI"dxsdk_math_fix.h")

target_include_directories(3dengine PRIVATE ${PANZERS_SRC_DIR}/sound)
target_link_libraries(3dengine PUBLIC core)

if(DXSDK_DIR)
    target_link_libraries(3dengine PUBLIC d3d9 d3dx9 dxguid dxerr)
endif()
