#pragma once

// Cross-platform export: Windows dllimport/export, ELF default visibility.
#if defined(_WIN32)
    #if defined(MTPOOL_SHARED)
        #if defined(MTPOOL_BUILDING)
            #define MTPOOL_API __declspec(dllexport)
        #else
            #define MTPOOL_API __declspec(dllimport)
        #endif
    #else
        #define MTPOOL_API
    #endif
#else
    #if defined(MTPOOL_SHARED) && defined(MTPOOL_BUILDING)
        #define MTPOOL_API __attribute__((visibility("default")))
    #else
        #define MTPOOL_API
    #endif
#endif
