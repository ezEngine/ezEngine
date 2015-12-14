#pragma once

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
  #ifdef BUILDSYSTEM_BUILDING_FMODPLUGIN_LIB
    #define EZ_FMODPLUGIN_DLL __declspec(dllexport)
  #else
    #define EZ_FMODPLUGIN_DLL __declspec(dllimport)
  #endif
#else
  #define EZ_FMODPLUGIN_DLL
#endif

#include <fmod_studio.hpp>

#define EZ_FMOD_ASSERT(res) EZ_VERIFY((res) == FMOD_OK, "Fmod failed with error code %u", res)

