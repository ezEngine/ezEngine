#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_EDITORFRAMEWORK_LIB
#    define EZ_EDITORFRAMEWORK_DLL EZ_DECL_EXPORT
#  else
#    define EZ_EDITORFRAMEWORK_DLL EZ_DECL_IMPORT
#  endif
#else
#  define EZ_EDITORFRAMEWORK_DLL
#endif
