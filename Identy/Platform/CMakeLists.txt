# Platform-specific sources based on target OS
# These sources are added to the parent Identy library (not a separate library)
# to share precompiled headers and simplify the build

if(WIN32)
    set(IDENTY_PLATFORM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Identy_hwid_pltimpl_windows.cxx
        ${CMAKE_CURRENT_SOURCE_DIR}/Identy_vm_pltimpl_windows.cxx
        PARENT_SCOPE
    )
elseif(UNIX AND NOT APPLE)
    set(IDENTY_PLATFORM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Identy_hwid_pltimpl_linux.cxx
        ${CMAKE_CURRENT_SOURCE_DIR}/Identy_vm_pltimpl_linux.cxx
        PARENT_SCOPE
    )
endif()
