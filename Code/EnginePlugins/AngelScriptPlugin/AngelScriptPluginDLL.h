#pragma once

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_ANGELSCRIPTPLUGIN_LIB
#    define EZ_ANGELSCRIPTPLUGIN_DLL EZ_DECL_EXPORT
#  else
#    define EZ_ANGELSCRIPTPLUGIN_DLL EZ_DECL_IMPORT
#  endif
#else
#  define EZ_ANGELSCRIPTPLUGIN_DLL
#endif

#define AS_CHECK(x)                                  \
  if (int res = (x); res < 0)                        \
  {                                                  \
    EZ_REPORT_FAILURE("AngelScript error: {}", res); \
  }

enum ezAsUserData
{
  ScriptInstancePtr = 0,
  RttiPtr = 1,
};
