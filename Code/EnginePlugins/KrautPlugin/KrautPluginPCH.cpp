#include <KrautPlugin/KrautPluginPCH.h>

#include <KrautPlugin/KrautDeclarations.h>

#include <Foundation/Configuration/Startup.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>
#include <KrautPlugin/Resources/KrautTreeResource.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Kraut, KrautPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezResourceManager::RegisterResourceForAssetType("Kraut Tree", ezGetStaticRTTI<ezKrautGeneratorResource>());

    ezResourceManager::AllowResourceTypeAcquireDuringUpdateContent<ezKrautTreeResource, ezMaterialResource>();
    ezResourceManager::AllowResourceTypeAcquireDuringUpdateContent<ezKrautTreeResource, ezMeshResource>();

    {
      ezKrautTreeResourceDescriptor desc;
      desc.m_Details.m_Bounds.SetInvalid();

        ezKrautTreeResourceHandle hResource = ezResourceManager::CreateResource<ezKrautTreeResource>("Missing Kraut Tree Mesh", std::move(desc), "Empty Kraut Tree Mesh");
      ezResourceManager::SetResourceTypeMissingFallback<ezKrautTreeResource>(hResource);
    }

    //{
    //  ezKrautGeneratorResourceHandle hResource = ezResourceManager::LoadResource<ezKrautGeneratorResource>("Kraut/KrautFallback.tree");
    //  ezResourceManager::SetResourceTypeMissingFallback<ezKrautGeneratorResource>(hResource);
    //}
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezResourceManager::SetResourceTypeMissingFallback<ezKrautTreeResource>(ezKrautTreeResourceHandle());
    ezResourceManager::SetResourceTypeMissingFallback<ezKrautGeneratorResource>(ezKrautGeneratorResourceHandle());

    ezKrautTreeResource::CleanupDynamicPluginReferences();
    ezKrautGeneratorResource::CleanupDynamicPluginReferences();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on
