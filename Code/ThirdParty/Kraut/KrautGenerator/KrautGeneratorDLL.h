#pragma once

#include <KrautFoundation/Defines.h>

#ifdef AE_COMPILE_ENGINE_AS_DLL
#  ifdef BUILDSYSTEM_BUILDING_KRAUTGENERATOR_LIB
#    define KRAUT_DLL __declspec(dllexport)
#  else
#    define KRAUT_DLL __declspec(dllimport)
#  endif
#else
#  define KRAUT_DLL
#endif
