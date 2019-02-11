#include <ProceduralPlacementPluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Startup.h>
#include <ProceduralPlacementPlugin/Resources/ProceduralPlacementResource.h>
#include <ProceduralPlacementPlugin/VM/ExpressionVM.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(ProceduralPlacement, ProceduralPlacementPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezResourceManager::RegisterResourceForAssetType("Procedural Placement", ezGetStaticRTTI<ezProceduralPlacementResource>());

    ezProceduralPlacementResourceDescriptor desc;
    ezProceduralPlacementResourceHandle hResource = ezResourceManager::CreateResource<ezProceduralPlacementResource>("ProceduralPlacementMissing", std::move(desc), "Fallback for missing Procedural Placement Resource");
    ezResourceManager::SetResourceTypeMissingFallback<ezProceduralPlacementResource>(hResource);

    //ezExpressionVM::Test();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezProceduralPlacementResource::CleanupDynamicPluginReferences();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////

ezPlugin g_Plugin(false);
