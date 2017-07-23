#pragma once

#include <Foundation/Basics/Platform/uwp/UWPUtils.h>

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
  #ifdef BUILDSYSTEM_BUILDING_WINDOWSMIXEDREALITY_LIB
    #define EZ_WINDOWSMIXEDREALITY_DLL __declspec(dllexport)
  #else
    #define EZ_WINDOWSMIXEDREALITY_DLL __declspec(dllimport)
  #endif
#else
  #define EZ_WINDOWSMIXEDREALITY_DLL
#endif

#include <WindowsMixedReality/Declarations.h>