#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_CORE_LIB
#    define EZ_CORE_DLL EZ_DECL_EXPORT
#    define EZ_CORE_DLL_FRIEND EZ_DECL_EXPORT_FRIEND
#  else
#    define EZ_CORE_DLL EZ_DECL_IMPORT
#    define EZ_CORE_DLL_FRIEND EZ_DECL_IMPORT_FRIEND
#  endif
#else
#  define EZ_CORE_DLL
#  define EZ_CORE_DLL_FRIEND
#endif
