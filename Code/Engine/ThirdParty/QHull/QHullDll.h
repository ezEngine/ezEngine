#pragma once

#ifdef BUILDSYSTEM_COMPILE_ENGINE_AS_DLL
  #if BUILDSYSTEM_BUILDING_THIRDPARTY_LIB
    #define QHULL_API __declspec(dllexport)
  #else
    #define QHULL_API __declspec(dllimport)
  #endif
#else
  #define QHULL_API
#endif

