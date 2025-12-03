#pragma once

#ifndef UNC_IDENTY_PLATFORM_IDENTITY_H
#define UNC_IDENTY_PLATFORM_IDENTITY_H

#ifdef _WIN32
#define IDENTY_WIN32
#elif defined(__linux__)
#define IDENTY_LINUX
#endif

#if defined(__clang__)
#define IDENTY_CLANG
#elif defined(__GNUC__)
#define IDENTY_GNUC
#elif defined(_MSC_VER)
#define IDENTY_MSVC
#endif

#endif
