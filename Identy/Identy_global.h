#pragma once
#ifndef UNC_IDENTY_GLOBAL_H
#define UNC_IDENTY_GLOBAL_H

#if defined(IDENTY_STATIC)
#define IDENTY_EXPORT
#elif defined(IDENTY_BUILD_SHARED)
#if defined(_WIN32) || defined(_WIN64)
#define IDENTY_EXPORT __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
#define IDENTY_EXPORT __attribute__((visibility("default")))
#else
#define IDENTY_EXPORT
#endif
#else
#define IDENTY_EXPORT
#endif

#endif
