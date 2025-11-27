add_executable (Identy "Identy_hwid.cxx")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Identy PROPERTY CXX_STANDARD 20)
endif()

target_link_libraries(Identy advapi32)

