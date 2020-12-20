#include <GameEnginePCH.h>

#include <Core/Collection/CollectionResource.h>
#include <Core/Curves/ColorGradientResource.h>
#include <Core/Curves/Curve1DResource.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <GameEngine/Animation/PropertyAnimResource.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/Physics/SurfaceResource.h>
#include <GameEngine/VisualScript/VisualScriptResource.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/Decals/DecalAtlasResource.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererFoundation/Device/DeviceFactory.h>

void ezGameApplication::Init_ConfigureAssetManagement()
{
  const ezStringBuilder sAssetRedirFile("AssetCache/", m_PlatformProfile.m_sName, ".ezAidlt");

  // which redirection table to search
  ezDataDirectory::FolderType::s_sRedirectionFile = sAssetRedirFile;

  // which platform assets to use
  ezDataDirectory::FolderType::s_sRedirectionPrefix = "AssetCache/";

  ezResourceManager::RegisterResourceForAssetType("Collection", ezGetStaticRTTI<ezCollectionResource>());
  ezResourceManager::RegisterResourceForAssetType("Material", ezGetStaticRTTI<ezMaterialResource>());
  ezResourceManager::RegisterResourceForAssetType("Mesh", ezGetStaticRTTI<ezMeshResource>());
  ezResourceManager::RegisterResourceForAssetType("Prefab", ezGetStaticRTTI<ezPrefabResource>());
  ezResourceManager::RegisterResourceForAssetType("RenderPipeline", ezGetStaticRTTI<ezRenderPipelineResource>());
  ezResourceManager::RegisterResourceForAssetType("Surface", ezGetStaticRTTI<ezSurfaceResource>());
  ezResourceManager::RegisterResourceForAssetType("Texture 2D", ezGetStaticRTTI<ezTexture2DResource>());
  ezResourceManager::RegisterResourceForAssetType("Render Target", ezGetStaticRTTI<ezTexture2DResource>());
  ezResourceManager::RegisterResourceForAssetType("Texture Cube", ezGetStaticRTTI<ezTextureCubeResource>());
  ezResourceManager::RegisterResourceForAssetType("Color Gradient", ezGetStaticRTTI<ezColorGradientResource>());
  ezResourceManager::RegisterResourceForAssetType("Curve1D", ezGetStaticRTTI<ezCurve1DResource>());
}

void ezGameApplication::Init_SetupDefaultResources()
{
  SUPER::Init_SetupDefaultResources();

  ezResourceManager::SetIncrementalUnloadForResourceType<ezShaderPermutationResource>(false);

  // Shaders
  {
    ezShaderResourceDescriptor desc;
    ezShaderResourceHandle hFallbackShader = ezResourceManager::CreateResource<ezShaderResource>("FallbackShaderResource", std::move(desc), "FallbackShaderResource");
    ezShaderResourceHandle hMissingShader = ezResourceManager::CreateResource<ezShaderResource>("MissingShaderResource", std::move(desc), "MissingShaderResource");

    ezResourceManager::SetResourceTypeLoadingFallback<ezShaderResource>(hFallbackShader);
    ezResourceManager::SetResourceTypeMissingFallback<ezShaderResource>(hMissingShader);
  }

  // Shader Permutation
  {
    ezShaderPermutationResourceDescriptor desc;
    ezShaderPermutationResourceHandle hFallbackShaderPermutation = ezResourceManager::CreateResource<ezShaderPermutationResource>("FallbackShaderPermutationResource", std::move(desc), "FallbackShaderPermutationResource");

    ezResourceManager::SetResourceTypeLoadingFallback<ezShaderPermutationResource>(hFallbackShaderPermutation);
  }

  // 2D Textures
  {
    ezTexture2DResourceHandle hFallbackTexture = ezResourceManager::LoadResource<ezTexture2DResource>("Textures/LoadingTexture_D.dds");
    ezTexture2DResourceHandle hMissingTexture = ezResourceManager::LoadResource<ezTexture2DResource>("Textures/MissingTexture_D.dds");

    ezResourceManager::SetResourceTypeLoadingFallback<ezTexture2DResource>(hFallbackTexture);
    ezResourceManager::SetResourceTypeMissingFallback<ezTexture2DResource>(hMissingTexture);
  }

  // Render to 2D Textures
  {
    ezRenderToTexture2DResourceDescriptor desc;
    desc.m_uiWidth = 128;
    desc.m_uiHeight = 128;

    ezRenderToTexture2DResourceHandle hMissingTexture = ezResourceManager::CreateResource<ezRenderToTexture2DResource>("R22DT_Missing", std::move(desc));

    ezResourceManager::SetResourceTypeMissingFallback<ezRenderToTexture2DResource>(hMissingTexture);
  }

  // Cube Textures
  {
    /// \todo Loading Cubemap Texture

    ezTextureCubeResourceHandle hFallbackTexture = ezResourceManager::LoadResource<ezTextureCubeResource>("Textures/MissingCubeMap.dds");
    ezTextureCubeResourceHandle hMissingTexture = ezResourceManager::LoadResource<ezTextureCubeResource>("Textures/MissingCubeMap.dds");

    ezResourceManager::SetResourceTypeLoadingFallback<ezTextureCubeResource>(hFallbackTexture);
    ezResourceManager::SetResourceTypeMissingFallback<ezTextureCubeResource>(hMissingTexture);
  }

  // Materials
  {
    ezResourceManager::AllowResourceTypeAcquireDuringUpdateContent<ezMaterialResource, ezMaterialResource>();

    ezMaterialResourceHandle hMissingMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Materials/BaseMaterials/MissingMaterial.ezMaterial");
    ezMaterialResourceHandle hFallbackMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Materials/BaseMaterials/LoadingMaterial.ezMaterial");

    ezResourceManager::SetResourceTypeLoadingFallback<ezMaterialResource>(hFallbackMaterial);
    ezResourceManager::SetResourceTypeMissingFallback<ezMaterialResource>(hMissingMaterial);
  }

  // Meshes
  {
    ezResourceManager::AllowResourceTypeAcquireDuringUpdateContent<ezMeshResource, ezMeshBufferResource>();

    ezMeshResourceHandle hMissingMesh = ezResourceManager::LoadResource<ezMeshResource>("Meshes/MissingMesh.ezMesh");
    ezResourceManager::SetResourceTypeMissingFallback<ezMeshResource>(hMissingMesh);
  }

  // Prefabs
  {
    // ezPrefabResourceDescriptor emptyPrefab;
    // ezPrefabResourceHandle hMissingPrefab = ezResourceManager::CreateResource<ezPrefabResource>("MissingPrefabResource", emptyPrefab,
    // "MissingPrefabResource");

    ezPrefabResourceHandle hMissingPrefab = ezResourceManager::LoadResource<ezPrefabResource>("Prefabs/MissingPrefab.ezObjectGraph");
    ezResourceManager::SetResourceTypeMissingFallback<ezPrefabResource>(hMissingPrefab);
  }

  // Collections
  {
    ezCollectionResourceDescriptor desc;
    ezCollectionResourceHandle hMissingCollection = ezResourceManager::CreateResource<ezCollectionResource>("MissingCollectionResource", std::move(desc), "MissingCollectionResource");

    ezResourceManager::SetResourceTypeMissingFallback<ezCollectionResource>(hMissingCollection);
  }

  // Render Pipelines
  {
    ezRenderPipelineResourceHandle hMissingRenderPipeline = ezRenderPipelineResource::CreateMissingPipeline();
    ezResourceManager::SetResourceTypeMissingFallback<ezRenderPipelineResource>(hMissingRenderPipeline);
  }

  // Color Gradient
  {
    ezColorGradientResourceDescriptor cg;
    cg.m_Gradient.AddColorControlPoint(0, ezColor::RebeccaPurple);
    cg.m_Gradient.AddColorControlPoint(1, ezColor::LawnGreen);
    cg.m_Gradient.SortControlPoints();

    ezColorGradientResourceHandle hResource = ezResourceManager::CreateResource<ezColorGradientResource>("MissingColorGradient", std::move(cg), "Missing Color Gradient Resource");
    ezResourceManager::SetResourceTypeMissingFallback<ezColorGradientResource>(hResource);
  }

  // 1D Curve
  {
    ezCurve1DResourceDescriptor cd;
    auto& curve = cd.m_Curves.ExpandAndGetRef();
    curve.AddControlPoint(0);
    curve.AddControlPoint(1);
    curve.CreateLinearApproximation();

    ezCurve1DResourceHandle hResource = ezResourceManager::CreateResource<ezCurve1DResource>("MissingCurve1D", std::move(cd), "Missing Curve1D Resource");
    ezResourceManager::SetResourceTypeMissingFallback<ezCurve1DResource>(hResource);
  }

  // Visual Script
  {
    ezVisualScriptResourceDescriptor desc;

    ezVisualScriptResourceHandle hResource = ezResourceManager::CreateResource<ezVisualScriptResource>("MissingVisualScript", std::move(desc), "Missing Visual Script Resource");
    ezResourceManager::SetResourceTypeMissingFallback<ezVisualScriptResource>(hResource);
  }

  // Property Animations
  {
    ezPropertyAnimResourceDescriptor desc;
    desc.m_AnimationDuration = ezTime::Seconds(0.1);

    ezPropertyAnimResourceHandle hResource = ezResourceManager::CreateResource<ezPropertyAnimResource>("MissingPropertyAnim", std::move(desc), "Missing Property Animation Resource");
    ezResourceManager::SetResourceTypeMissingFallback<ezPropertyAnimResource>(hResource);
  }

  // Animation Skeleton
  {
    ezSkeletonResourceDescriptor desc;

    ezSkeletonResourceHandle hResource = ezResourceManager::CreateResource<ezSkeletonResource>("MissingSkeleton", std::move(desc), "Missing Skeleton Resource");
    ezResourceManager::SetResourceTypeMissingFallback<ezSkeletonResource>(hResource);
  }

  // Animation Clip
  {
    ezAnimationClipResourceDescriptor desc;

    ezAnimationClipResourceHandle hResource = ezResourceManager::CreateResource<ezAnimationClipResource>("MissingAnimationClip", std::move(desc), "Missing Animation Clip Resource");
    ezResourceManager::SetResourceTypeMissingFallback<ezAnimationClipResource>(hResource);
  }

  // Decal Atlas
  {
    ezResourceManager::AllowResourceTypeAcquireDuringUpdateContent<ezDecalAtlasResource, ezTexture2DResource>();
  }
}

const char* GetRendererNameFromCommandLine()
{
#ifdef BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
  constexpr const char* szDefaultRenderer = "Vulkan";
#else
  constexpr const char* szDefaultRenderer = "DX11";
#endif

  return ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, szDefaultRenderer);
}

void ezGameApplication::Init_SetupGraphicsDevice()
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  ezGALDeviceCreationDescription DeviceInit;
  DeviceInit.m_bCreatePrimarySwapChain = false;

#  if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  DeviceInit.m_bDebugDevice = true;
#  endif

  {
    ezGALDevice* pDevice = nullptr;

    if (s_DefaultDeviceCreator.IsValid())
    {
      pDevice = s_DefaultDeviceCreator(DeviceInit);
    }
    else
    {
      const char* szRendererName = GetRendererNameFromCommandLine();
      pDevice = ezGALDeviceFactory::CreateDevice(szRendererName, ezFoundation::GetDefaultAllocator(), DeviceInit);
      EZ_ASSERT_DEV(pDevice != nullptr, "Device implemention for '{}' not found", szRendererName);
    }

    EZ_VERIFY(pDevice->Init() == EZ_SUCCESS, "Graphics device creation failed!");
    ezGALDevice::SetDefaultDevice(pDevice);
  }

  // Create GPU resource pool
  ezGPUResourcePool* pResourcePool = EZ_DEFAULT_NEW(ezGPUResourcePool);
  ezGPUResourcePool::SetDefaultInstance(pResourcePool);

#endif
}

void ezGameApplication::Init_LoadRequiredPlugins()
{
  const char* szRendererName = GetRendererNameFromCommandLine();
  const char* szShaderModel = "";
  const char* szShaderCompiler = "";
  ezGALDeviceFactory::GetShaderModelAndCompiler(szRendererName, szShaderModel, szShaderCompiler);
  ezShaderManager::Configure(szShaderModel, true);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezPlugin::LoadPlugin("ezInspectorPlugin").IgnoreResult();

  EZ_VERIFY(ezPlugin::LoadPlugin(szShaderCompiler).Succeeded(), "Shader compiler '{}' plugin not found", szShaderCompiler);

#  ifdef BUILDSYSTEM_ENABLE_RENDERDOC_SUPPORT
  ezPlugin::LoadPlugin("ezRenderDocPlugin").IgnoreResult();
#  endif

  // on sandboxed platforms, we can only load data through fileserve, so enforce use of this plugin
#  if EZ_DISABLED(EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  ezPlugin::LoadPlugin("ezFileservePlugin").IgnoreResult(); // don't care if it fails to load
#  endif

#endif
}

void ezGameApplication::Deinit_ShutdownGraphicsDevice()
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  if (!ezGALDevice::HasDefaultDevice())
    return;

  // Cleanup resource pool
  ezGPUResourcePool::SetDefaultInstance(nullptr);

  ezResourceManager::FreeAllUnusedResources();

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  pDevice->Shutdown().IgnoreResult();
  EZ_DEFAULT_DELETE(pDevice);
  ezGALDevice::SetDefaultDevice(nullptr);
#endif
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_GameApplication_Implementation_GameApplicationInit);
