#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Plugin.h>

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_ENGINEPLUGINJOLT_LIB
#    define EZ_ENGINEPLUGINJOLT_DLL __declspec(dllexport)
#  else
#    define EZ_ENGINEPLUGINJOLT_DLL __declspec(dllimport)
#  endif
#else
#  define EZ_ENGINEPLUGINJOLT_DLL
#endif
