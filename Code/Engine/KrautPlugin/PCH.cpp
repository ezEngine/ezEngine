#include <PCH.h>

#include <KrautPlugin/KrautDeclarations.h>

#include <Foundation/Configuration/Startup.h>
#include <KrautPlugin/Resources/KrautTreeResource.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Kraut, KrautPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    ezResourceManager::RegisterResourceForAssetType("Kraut Tree", ezGetStaticRTTI<ezKrautTreeResource>());

    ezKrautTreeResourceDescriptor desc;
    desc.m_Bounds.SetInvalid();
    ezKrautTreeResourceHandle hResource = ezResourceManager::CreateResource<ezKrautTreeResource>("Missing Kraut Tree", desc, "Empty Kraut tree");
    ezKrautTreeResource::SetTypeMissingResource(hResource);
  }

  ON_CORE_SHUTDOWN
  {
    ezKrautTreeResource::SetTypeMissingResource(ezKrautTreeResourceHandle());

    ezKrautTreeResource::CleanupDynamicPluginReferences();
  }

  ON_ENGINE_STARTUP
  {
  }

  ON_ENGINE_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

void OnLoadPlugin(bool bReloading) {}
void OnUnloadPlugin(bool bReloading) {}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(EZ_KRAUTPLUGIN_DLL, ezKrautPlugin);
