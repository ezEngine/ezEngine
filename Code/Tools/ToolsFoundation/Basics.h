#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
  #ifdef BUILDSYSTEM_BUILDING_TOOLSFOUNDATION_LIB
    #define EZ_TOOLSFOUNDATION_DLL __declspec(dllexport)
    #define EZ_TOOLSFOUNDATION_TEMPLATE
  #else
    #define EZ_TOOLSFOUNDATION_DLL __declspec(dllimport)
    #define EZ_TOOLSFOUNDATION_TEMPLATE extern
  #endif
#else
  #define EZ_TOOLSFOUNDATION_DLL
  #define EZ_TOOLSFOUNDATION_TEMPLATE
#endif
