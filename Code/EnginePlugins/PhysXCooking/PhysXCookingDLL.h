#pragma once

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_PHYSXCOOKING_LIB
#    define EZ_PHYSXCOOKING_DLL EZ_DECL_EXPORT
#  else
#    define EZ_PHYSXCOOKING_DLL EZ_DECL_IMPORT
#  endif
#else
#  define EZ_PHYSXCOOKING_DLL
#endif
