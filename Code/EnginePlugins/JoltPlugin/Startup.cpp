#include <JoltPlugin/JoltPluginPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>
#include <JoltPlugin/System/JoltCore.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Jolt, JoltPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezResourceManager::RegisterResourceForAssetType("Jolt_Colmesh_Triangle", ezGetStaticRTTI<ezJoltMeshResource>());
    ezResourceManager::RegisterResourceForAssetType("Jolt_Colmesh_Convex", ezGetStaticRTTI<ezJoltMeshResource>());

    ezJoltMeshResourceDescriptor desc;
    ezJoltMeshResourceHandle hResource = ezResourceManager::CreateResource<ezJoltMeshResource>("Missing Jolt Mesh", std::move(desc), "Empty collision mesh");
    ezResourceManager::SetResourceTypeMissingFallback<ezJoltMeshResource>(hResource);

    ezJoltCore::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezResourceManager::SetResourceTypeMissingFallback<ezJoltMeshResource>(ezJoltMeshResourceHandle());
    ezJoltCore::Shutdown();

    ezJoltMeshResource::CleanupDynamicPluginReferences();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on


EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Startup);
