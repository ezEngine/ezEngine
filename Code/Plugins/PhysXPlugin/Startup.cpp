#include <PhysXPluginPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <PhysXPlugin/Resources/PxMeshResource.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(PhysX, PhysXPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezPhysX::GetSingleton()->Startup();

    ezResourceManager::RegisterResourceForAssetType("Collision Mesh", ezGetStaticRTTI<ezPxMeshResource>());
    ezResourceManager::RegisterResourceForAssetType("Collision Mesh (Convex)", ezGetStaticRTTI<ezPxMeshResource>());

    ezPxMeshResourceDescriptor desc;
    ezPxMeshResourceHandle hResource = ezResourceManager::CreateResource<ezPxMeshResource>("Missing PhysX Mesh", std::move(desc), "Empty collision mesh");
    ezResourceManager::SetResourceTypeMissingFallback<ezPxMeshResource>(hResource);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezResourceManager::SetResourceTypeMissingFallback<ezPxMeshResource>(ezPxMeshResourceHandle());
    ezPhysX::GetSingleton()->Shutdown();

    ezPxMeshResource::CleanupDynamicPluginReferences();
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



EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Startup);
