#pragma once

#ifdef BUILDSYSTEM_COMPILE_ENGINE_AS_DLL
  #if BUILDSYSTEM_BUILDING_THIRDPARTY_LIB
    #define RECAST_API __declspec(dllexport)
  #else
    #define RECAST_API __declspec(dllimport)
  #endif
#else
  #define RECAST_API
#endif

