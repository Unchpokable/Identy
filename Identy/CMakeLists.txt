add_library(Identy "Identy_hwid.cxx" "Identy_vm.cxx" "Identy_hash.cxx" "Identy_io.cxx" "Identy_sha256.cxx")

if(CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Identy PROPERTY CXX_STANDARD 20)
endif()

# Указываем, где искать заголовочные файлы библиотеки
target_include_directories(Identy PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

if(WIN32)
target_link_libraries(Identy advapi32 iphlpapi)
endif()
