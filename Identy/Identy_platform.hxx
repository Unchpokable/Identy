#pragma once

#ifndef UNC_IDENTY_PLATFORM_IDENTITY_H
#define UNC_IDENTY_PLATFORM_IDENTITY_H

#ifndef _WIN32
#error "Identy currently supports only windows platforms!"
#endif

#ifdef _WIN32
#define IDENTY_WIN32
#endif

#endif
