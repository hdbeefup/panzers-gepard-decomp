# third_party/rad — import libraries for the RAD Game Tools DLLs Panzers links
# dynamically: mss32.dll (Miles 6.5c) and binkw32.dll (Bink 1.5q).
#
# No SDK is used. The .lib files are generated at build time from the
# hand-made .def files (lib /def), and the headers are hand-written
# declarations. See README.md. The DLLs come from the game install at runtime
# and are never part of the repository.

option(PANZERS_RAD_DELAYLOAD "Delay-load mss32.dll and binkw32.dll" ON)

foreach(_rad mss32 binkw32)
    set(_lib "${CMAKE_CURRENT_BINARY_DIR}/${_rad}.lib")
    add_custom_command(
        OUTPUT "${_lib}"
        COMMAND "${CMAKE_AR}" /nologo "/def:${CMAKE_CURRENT_SOURCE_DIR}/${_rad}.def"
                /machine:x86 "/out:${_lib}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${_rad}.def"
        COMMENT "Generating ${_rad}.lib import library from ${_rad}.def"
        VERBATIM)
    add_custom_target(${_rad}_implib DEPENDS "${_lib}")

    add_library(rad_${_rad} INTERFACE)
    add_dependencies(rad_${_rad} ${_rad}_implib)
    target_include_directories(rad_${_rad} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(rad_${_rad} INTERFACE "${_lib}")
    # PANZERS.exe imports both DLLs directly. They are delay-loaded here so
    # that a build-tree exe without the game's DLLs next to it still starts
    # (the DLLs are resolved on the first call). Set PANZERS_RAD_DELAYLOAD=OFF
    # for plain imports like the original.
    if(PANZERS_RAD_DELAYLOAD)
        target_link_libraries(rad_${_rad} INTERFACE delayimp)
        target_link_options(rad_${_rad} INTERFACE "/DELAYLOAD:${_rad}.dll")
    endif()
endforeach()
