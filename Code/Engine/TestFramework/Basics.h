#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
  #ifdef BUILDSYSTEM_BUILDING_TESTFRAMEWORK_LIB
    #define EZ_TEST_DLL __declspec(dllexport)
    #define EZ_TEST_TEMPLATE
  #else
    #define EZ_TEST_DLL __declspec(dllimport)
    #define EZ_TEST_TEMPLATE extern
  #endif
#else
  #define EZ_TEST_DLL
  #define EZ_TEST_TEMPLATE
#endif

