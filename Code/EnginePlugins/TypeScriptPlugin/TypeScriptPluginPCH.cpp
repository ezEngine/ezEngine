#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Startup.h>
#include <TypeScriptPlugin/Resources/ScriptCompendiumResource.h>
#include <TypeScriptPlugin/TypeScriptPluginDLL.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(TypeScript, TypeScriptPlugin)

BEGIN_SUBSYSTEM_DEPENDENCIES
"Foundation",
"Core"
END_SUBSYSTEM_DEPENDENCIES

ON_CORESYSTEMS_STARTUP
{
  // ezResourceManager::RegisterResourceForAssetType("TypeScript", nullptr);
}

ON_CORESYSTEMS_SHUTDOWN
{
  ezScriptCompendiumResource::CleanupDynamicPluginReferences();
}

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on
