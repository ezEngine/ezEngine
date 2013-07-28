#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
  #ifdef BUILDSYSTEM_BUILDING_INPUTWINDOWS_LIB
    #define EZ_INPUTWINDOWS_DLL __declspec(dllexport)
    #define EZ_INPUTWINDOWS_TEMPLATE
  #else
    #define EZ_INPUTWINDOWS_DLL __declspec(dllimport)
    #define EZ_INPUTWINDOWS_TEMPLATE extern
  #endif
#else
  #define EZ_INPUTWINDOWS_DLL
  #define EZ_INPUTWINDOWS_TEMPLATE
#endif
