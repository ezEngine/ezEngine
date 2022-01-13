#pragma once

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_DLANGPLUGIN_LIB
#    define EZ_DLANGPLUGIN_DLL __declspec(dllexport)
#  else
#    define EZ_DLANGPLUGIN_DLL __declspec(dllimport)
#  endif
#else
#  define EZ_DLANGPLUGIN_DLL
#endif
