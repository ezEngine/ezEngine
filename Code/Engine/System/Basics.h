#pragma once

#include <Foundation/Basics.h>
#include <System/PlatformFeatures.h>

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
  #ifdef BUILDSYSTEM_BUILDING_SYSTEM_LIB
    #define EZ_SYSTEM_DLL __declspec(dllexport)
    #define EZ_SYSTEM_TEMPLATE
  #else
    #define EZ_SYSTEM_DLL __declspec(dllimport)
    #define EZ_SYSTEM_TEMPLATE extern
  #endif
#else
  #define EZ_SYSTEM_DLL
  #define EZ_SYSTEM_TEMPLATE
#endif



