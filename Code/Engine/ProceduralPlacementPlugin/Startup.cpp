#include <PCH.h>
#include <ProceduralPlacementPlugin/Resources/ProceduralPlacementResource.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Configuration/Plugin.h>

#include <ProceduralPlacementPlugin/VM/ExpressionVM.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(ProceduralPlacement, ProceduralPlacementPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    ezResourceManager::RegisterResourceForAssetType("Procedural Placement", ezGetStaticRTTI<ezProceduralPlacementResource>());

    ezProceduralPlacementResourceDescriptor desc;
    ezProceduralPlacementResourceHandle hResource = ezResourceManager::CreateResource<ezProceduralPlacementResource>("ProceduralPlacementMissing", desc, "Fallback for missing Procedural Placement Resource");
    ezProceduralPlacementResource::SetTypeMissingResource(hResource);

    //ezExpressionVM::Test();
  }

  ON_CORE_SHUTDOWN
  {
    ezProceduralPlacementResource::CleanupDynamicPluginReferences();
  }

  ON_ENGINE_STARTUP
  {
  }

  ON_ENGINE_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION

//////////////////////////////////////////////////////////////////////////

void OnLoadPlugin(bool bReloading) { }
void OnUnloadPlugin(bool bReloading) { }

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(EZ_PROCEDURALPLACEMENTPLUGIN_DLL, ezProceduralPlacementPlugin);


