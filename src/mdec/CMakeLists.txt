# mdec — MP3/MPEG audio decoder
add_library(mdec STATIC
    decode.cpp
    huffman.cpp
    hybrid.cpp
    istream.cpp
    layer3.cpp
    mdec_data.cpp
    subband.cpp
)
target_include_directories(mdec PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(mdec PRIVATE ${PANZERS_SRC_DIR}/sound)
target_link_libraries(mdec PUBLIC core)
if(MSVC)
    target_compile_options(mdec PRIVATE /bigobj)
endif()
