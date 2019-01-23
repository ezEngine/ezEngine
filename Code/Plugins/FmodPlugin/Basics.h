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

// Forward declarations

namespace FMOD
{
  namespace Studio
  {
    class Bank;
    class System;
    class EventInstance;
    class EventDescription;
  }

  class System;
}
