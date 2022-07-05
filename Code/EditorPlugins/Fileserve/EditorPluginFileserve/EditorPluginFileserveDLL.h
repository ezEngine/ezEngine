#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Plugin.h>

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_EDITORPLUGINFILESERVE_LIB
#    define EZ_EDITORPLUGINFILESERVE_DLL EZ_DECL_EXPORT
#  else
#    define EZ_EDITORPLUGINFILESERVE_DLL EZ_DECL_IMPORT
#  endif
#else
#  define EZ_EDITORPLUGINFILESERVE_DLL
#endif
