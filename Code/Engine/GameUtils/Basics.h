#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
  #ifdef BUILDSYSTEM_BUILDING_GAMEUTILS_LIB
    #define EZ_GAMEUTILS_DLL __declspec(dllexport)
  #else
    #define EZ_GAMEUTILS_DLL __declspec(dllimport)
  #endif
#else
  #define EZ_GAMEUTILS_DLL
#endif

