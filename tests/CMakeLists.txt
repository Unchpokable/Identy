# Identy Test Suite Configuration
# Uses GoogleTest via FetchContent - no manual installation required

include(FetchContent)

# Fetch GoogleTest
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Test configuration defines
if(IDENTY_TEST_EXPECT_VM)
    add_compile_definitions(IDENTY_TEST_EXPECT_VM)
endif()

if(IDENTY_TEST_EXPECT_BAREMETAL)
    add_compile_definitions(IDENTY_TEST_EXPECT_BAREMETAL)
endif()

# Detect CI environment
if(DEFINED ENV{CI} OR DEFINED ENV{GITHUB_ACTIONS})
    add_compile_definitions(CI GITHUB_ACTIONS)
endif()

# Test executable
add_executable(identy_tests
    test_main.cxx
    test_hwid.cxx
    test_vm_detection.cxx
    test_hash.cxx
    test_io.cxx
    test_strings.cxx
    test_integration.cxx
)

target_link_libraries(identy_tests
    PRIVATE
        Identy
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(identy_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Set C++20 standard for tests
set_target_properties(identy_tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(identy_tests)
