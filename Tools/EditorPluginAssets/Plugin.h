#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Plugin.h>

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
  #ifdef BUILDSYSTEM_BUILDING_EDITORPLUGINASSETS
    #define EZ_EDITORPLUGINASSETS_DLL __declspec(dllexport)
  #else
    #define EZ_EDITORPLUGINASSETS_DLL __declspec(dllimport)
  #endif
#else
  #define EZ_EDITORPLUGINASSETS_DLL
#endif

// this is required so that this DLL exports anything at all and thus a .lib and .exp file is generated (otherwise it isn't)
// which in turn is necessary to statically link this library into some application
EZ_DYNAMIC_PLUGIN_DECLARATION(EZ_EDITORPLUGINASSETS_DLL, ezEditorPluginAssets);
