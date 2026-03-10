#pragma once

#if defined(_WIN32)
#define SPLINTER3D_API
#else
#define SPLINTER3D_API __attribute__((visibility("default")))
#endif

#if defined(__GNUC__) || defined(__clang__)
#define SPLINTER3D_HIDE_FROM_ABI __attribute__((visibility("hidden")))
#else
#define SPLINTER3D_HIDE_FROM_ABI
#endif
