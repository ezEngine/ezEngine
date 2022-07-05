#pragma once

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_FMODPLUGIN_LIB
#    define EZ_FMODPLUGIN_DLL EZ_DECL_EXPORT
#  else
#    define EZ_FMODPLUGIN_DLL EZ_DECL_IMPORT
#  endif
#else
#  define EZ_FMODPLUGIN_DLL
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
  } // namespace Studio

  class System;
} // namespace FMOD
