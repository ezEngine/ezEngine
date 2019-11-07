#include <BakingPluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Startup.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Baking, BakingPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    //ezResourceManager::RegisterResourceForAssetType("ProcGen Graph", ezGetStaticRTTI<ezProcGenGraphResource>());

    //ezProcGenGraphResourceDescriptor desc;
    //ezProcGenGraphResourceHandle hResource = ezResourceManager::CreateResource<ezProcGenGraphResource>("ProcGenGraphMissing", std::move(desc), "Fallback for missing ProcGen Graph Resource");
    //ezResourceManager::SetResourceTypeMissingFallback<ezProcGenGraphResource>(hResource);

    //ezExpressionVM::Test();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    //ezProcGenGraphResource::CleanupDynamicPluginReferences();
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
