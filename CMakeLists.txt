cmake_minimum_required(VERSION 3.16)

if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(Identy)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Platform detection - defines IDENTY_WIN32 or IDENTY_LINUX
if(WIN32)
    add_compile_definitions(IDENTY_WIN32)
elseif(UNIX AND NOT APPLE)
    add_compile_definitions(IDENTY_LINUX)
endif()

add_subdirectory("Identy")

# Testing support
option(IDENTY_BUILD_TESTS "Build the Identy test suite" OFF)
option(IDENTY_TEST_EXPECT_VM "Expect tests to run in a virtual machine (CI environment)" OFF)
option(IDENTY_TEST_EXPECT_BAREMETAL "Expect tests to run on bare metal hardware" OFF)

if(IDENTY_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
