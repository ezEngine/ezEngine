#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Plugin.h>

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_CPPPROJECTPLUGIN_LIB
#    define EZ_CPPPROJECTPLUGIN_DLL __declspec(dllexport)
#  else
#    define EZ_CPPPROJECTPLUGIN_DLL __declspec(dllimport)
#  endif
#else
#  define EZ_CPPPROJECTPLUGIN_DLL
#endif
