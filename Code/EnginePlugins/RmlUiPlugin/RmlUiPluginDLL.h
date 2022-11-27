#pragma once

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RMLUIPLUGIN_LIB
#    define EZ_RMLUIPLUGIN_DLL EZ_DECL_EXPORT
#  else
#    define EZ_RMLUIPLUGIN_DLL EZ_DECL_IMPORT
#  endif
#else
#  define EZ_RMLUIPLUGIN_DLL
#endif

#ifndef RMLUI_USE_CUSTOM_RTTI
#  define RMLUI_USE_CUSTOM_RTTI
#endif
