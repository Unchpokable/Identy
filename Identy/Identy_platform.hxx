#pragma once

#ifndef UNC_IDENTY_PLATFORM_IDENTITY_H
#define UNC_IDENTY_PLATFORM_IDENTITY_H

// Platform detection (IDENTY_WIN32, IDENTY_LINUX) is now handled by CMake
// See root CMakeLists.txt for add_compile_definitions()

// Compiler detection
#if defined(__clang__)
#define IDENTY_CLANG
#elif defined(__GNUC__)
#define IDENTY_GNUC
#elif defined(_MSC_VER)
#define IDENTY_MSVC
#endif

#endif
