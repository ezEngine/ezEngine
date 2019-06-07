#pragma once

#include <Foundation/Basics.h>
#include <System/PlatformFeatures.h>

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_XBOXCONTROLLERPLUGIN_LIB
#    define EZ_XBOXCONTROLLER_DLL __declspec(dllexport)
#  else
#    define EZ_XBOXCONTROLLER_DLL __declspec(dllimport)
#  endif
#else
#  define EZ_XBOXCONTROLLER_DLL
#endif
