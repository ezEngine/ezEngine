#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
  #ifdef BUILDSYSTEM_BUILDING_COREUTILS_LIB
    #define EZ_COREUTILS_DLL __declspec(dllexport)
  #else
    #define EZ_COREUTILS_DLL __declspec(dllimport)
  #endif
#else
  #define EZ_COREUTILS_DLL
#endif

#define EZ_EMBED_FONT_FILE EZ_ON

#if EZ_ENABLED(EZ_EMBED_FONT_FILE)

extern ezUInt64 g_uiFontFileSize;
extern const ezUInt8 g_FontFileTGA[];

#endif
