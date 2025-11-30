add_library(Identy "Identy_hwid.cxx" "Identy_vm.cxx" "Identy_hash.cxx" "Identy_io.cxx" "Identy_sha256.cxx")

if(CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Identy PROPERTY CXX_STANDARD 20)
endif()

if(WIN32)
target_link_libraries(Identy advapi32 iphlpapi)
endif()
