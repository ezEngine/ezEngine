#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
  #ifdef BUILDSYSTEM_BUILDING_CORE_LIB
    #define EZ_CORE_DLL __declspec(dllexport)
    #define EZ_CORE_TEMPLATE
  #else
    #define EZ_CORE_DLL __declspec(dllimport)
    #define EZ_CORE_TEMPLATE extern
  #endif
#else
  #define EZ_CORE_DLL
  #define EZ_CORE_TEMPLATE
#endif
