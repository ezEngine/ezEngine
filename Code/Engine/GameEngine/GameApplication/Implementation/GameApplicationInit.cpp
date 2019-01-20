#include <PCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <GameEngine/Collection/CollectionResource.h>
#include <GameEngine/Curves/ColorGradientResource.h>
#include <GameEngine/Curves/Curve1DResource.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/Prefabs/PrefabResource.h>
#include <GameEngine/Resources/PropertyAnimResource.h>
#include <GameEngine/Surfaces/SurfaceResource.h>
#include <GameEngine/VisualScript/VisualScriptResource.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <RendererDX11/Device/DeviceDX11.h>
typedef ezGALDeviceDX11 ezGALDeviceDefault;
#endif

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#  include <WindowsMixedReality/Graphics/MixedRealityDX11Device.h>
#endif

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
}

void ezGameApplication::Init_SetupDefaultResources()
{
  // Shaders
  {
    ezShaderResourceDescriptor desc;
    ezShaderResourceHandle hFallbackShader =
        ezResourceManager::CreateResource<ezShaderResource>("FallbackShaderResource", std::move(desc), "FallbackShaderResource");
    ezShaderResourceHandle hMissingShader =
        ezResourceManager::CreateResource<ezShaderResource>("MissingShaderResource", std::move(desc), "MissingShaderResource");

    ezShaderResource::SetTypeFallbackResource(hFallbackShader);
    ezShaderResource::SetTypeMissingResource(hMissingShader);
  }

  // Shader Permutation
  {
    ezShaderPermutationResourceDescriptor desc;
    ezShaderPermutationResourceHandle hFallbackShaderPermutation = ezResourceManager::CreateResource<ezShaderPermutationResource>(
        "FallbackShaderPermutationResource", std::move(desc), "FallbackShaderPermutationResource");

    ezShaderPermutationResource::SetTypeFallbackResource(hFallbackShaderPermutation);
  }

  // 2D Textures
  {
    ezTexture2DResourceHandle hFallbackTexture = ezResourceManager::LoadResource<ezTexture2DResource>(
        "Textures/LoadingTexture_D.dds", ezResourcePriority::Highest, ezTexture2DResourceHandle());
    ezTexture2DResourceHandle hMissingTexture = ezResourceManager::LoadResource<ezTexture2DResource>("Textures/MissingTexture_D.dds");

    ezTexture2DResource::SetTypeFallbackResource(hFallbackTexture);
    ezTexture2DResource::SetTypeMissingResource(hMissingTexture);
  }

  // Cube Textures
  {
    /// \todo Loading Cubemap Texture

    ezTextureCubeResourceHandle hFallbackTexture = ezResourceManager::LoadResource<ezTextureCubeResource>("Textures/MissingCubeMap.dds");
    ezTextureCubeResourceHandle hMissingTexture = ezResourceManager::LoadResource<ezTextureCubeResource>("Textures/MissingCubeMap.dds");

    ezTextureCubeResource::SetTypeFallbackResource(hFallbackTexture);
    ezTextureCubeResource::SetTypeMissingResource(hMissingTexture);
  }

  // Materials
  {
    ezMaterialResourceHandle hMissingMaterial =
        ezResourceManager::LoadResource<ezMaterialResource>("Materials/BaseMaterials/MissingMaterial.ezMaterial");
    ezMaterialResourceHandle hFallbackMaterial = ezResourceManager::LoadResource<ezMaterialResource>(
        "Materials/BaseMaterials/LoadingMaterial.ezMaterial", ezResourcePriority::Highest, ezMaterialResourceHandle());

    ezMaterialResource::SetTypeFallbackResource(hFallbackMaterial);
    ezMaterialResource::SetTypeMissingResource(hMissingMaterial);
  }

  // Meshes
  {
    ezMeshResourceHandle hMissingMesh = ezResourceManager::LoadResource<ezMeshResource>("Meshes/MissingMesh.ezMesh");
    ezMeshResource::SetTypeMissingResource(hMissingMesh);
  }

  // Prefabs
  {
    // ezPrefabResourceDescriptor emptyPrefab;
    // ezPrefabResourceHandle hMissingPrefab = ezResourceManager::CreateResource<ezPrefabResource>("MissingPrefabResource", emptyPrefab,
    // "MissingPrefabResource");

    ezPrefabResourceHandle hMissingPrefab = ezResourceManager::LoadResource<ezPrefabResource>("Prefabs/MissingPrefab.ezObjectGraph");
    ezPrefabResource::SetTypeMissingResource(hMissingPrefab);
  }

  // Collections
  {
    ezCollectionResourceDescriptor desc;
    ezCollectionResourceHandle hMissingCollection =
        ezResourceManager::CreateResource<ezCollectionResource>("MissingCollectionResource", std::move(desc), "MissingCollectionResource");

    ezCollectionResource::SetTypeMissingResource(hMissingCollection);
  }

  // Render Pipelines
  {
    ezRenderPipelineResourceHandle hMissingRenderPipeline = ezRenderPipelineResource::CreateMissingPipeline();
    ezRenderPipelineResource::SetTypeMissingResource(hMissingRenderPipeline);
  }

  // Color Gradient
  {
    ezColorGradientResourceDescriptor cg;
    cg.m_Gradient.AddColorControlPoint(0, ezColor::RebeccaPurple);
    cg.m_Gradient.AddColorControlPoint(1, ezColor::LawnGreen);
    cg.m_Gradient.SortControlPoints();

    ezColorGradientResourceHandle hResource =
        ezResourceManager::CreateResource<ezColorGradientResource>("MissingColorGradient", std::move(cg), "Missing Color Gradient Resource");
    ezColorGradientResource::SetTypeMissingResource(hResource);
  }

  // 1D Curve
  {
    ezCurve1DResourceDescriptor cd;
    auto& curve = cd.m_Curves.ExpandAndGetRef();
    curve.AddControlPoint(0);
    curve.AddControlPoint(1);
    curve.CreateLinearApproximation();

    ezCurve1DResourceHandle hResource =
        ezResourceManager::CreateResource<ezCurve1DResource>("MissingCurve1D", std::move(cd), "Missing Curve1D Resource");
    ezCurve1DResource::SetTypeMissingResource(hResource);
  }

  // Visual Script
  {
    ezVisualScriptResourceDescriptor desc;

    ezVisualScriptResourceHandle hResource =
        ezResourceManager::CreateResource<ezVisualScriptResource>("MissingVisualScript", std::move(desc), "Missing Visual Script Resource");
    ezVisualScriptResource::SetTypeMissingResource(hResource);
  }

  // Property Animations
  {
    ezPropertyAnimResourceDescriptor desc;
    desc.m_AnimationDuration = ezTime::Seconds(0.1);

    ezPropertyAnimResourceHandle hResource =
        ezResourceManager::CreateResource<ezPropertyAnimResource>("MissingPropertyAnim", std::move(desc), "Missing Property Animation Resource");
    ezPropertyAnimResource::SetTypeMissingResource(hResource);
  }

  // Animation Skeleton
  {
    ezSkeletonResourceDescriptor desc;

    ezSkeletonResourceHandle hResource =
        ezResourceManager::CreateResource<ezSkeletonResource>("MissingSkeleton", std::move(desc), "Missing Skeleton Resource");
    ezSkeletonResource::SetTypeMissingResource(hResource);
  }

  // Animation Clip
  {
    ezAnimationClipResourceDescriptor desc;

    ezAnimationClipResourceHandle hResource =
        ezResourceManager::CreateResource<ezAnimationClipResource>("MissingAnimationClip", std::move(desc), "Missing Animation Clip Resource");
    ezAnimationClipResource::SetTypeMissingResource(hResource);
  }
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
      pDevice = s_DefaultDeviceCreator(DeviceInit);
    else
      pDevice = EZ_DEFAULT_NEW(ezGALDeviceDefault, DeviceInit);

    EZ_VERIFY(pDevice->Init() == EZ_SUCCESS, "Graphics device creation failed!");
    ezGALDevice::SetDefaultDevice(pDevice);
  }

  // Create GPU resource pool
  ezGPUResourcePool* pResourcePool = EZ_DEFAULT_NEW(ezGPUResourcePool);
  ezGPUResourcePool::SetDefaultInstance(pResourcePool);

#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  ezShaderManager::Configure("DX11_SM50", true);
#  else
  EZ_ASSERT_NOT_IMPLEMENTED;
  ezShaderManager::Configure("GL3", true);
#  endif

#endif
}

void ezGameApplication::Init_LoadRequiredPlugins()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezPlugin::LoadPlugin("ezInspectorPlugin");

#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  ezPlugin::LoadPlugin("ezShaderCompilerHLSL");
  ezPlugin::LoadPlugin("ezRenderDocPlugin");
#  endif

  // on sandboxed platforms, we can only load data through fileserve, so enforce use of this plugin
#  if EZ_DISABLED(EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  ezPlugin::LoadPlugin("ezFileservePlugin"); // don't care if it fails to load
#  endif

#endif
}

void ezGameApplication::Deinit_ShutdownGraphicsDevice()
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  if (!ezGALDevice::HasDefaultDevice())
    return;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  for (ezUInt32 i = 0; i < m_Windows.GetCount(); ++i)
  {
    DestroyWindowOutputTarget(std::move(m_Windows[i].m_pOutputTarget));
  }

  // Cleanup resource pool
  ezGPUResourcePool::SetDefaultInstance(nullptr);

  ezResourceManager::FreeUnusedResources(true);

  pDevice->Shutdown();
  EZ_DEFAULT_DELETE(pDevice);
  ezGALDevice::SetDefaultDevice(nullptr);
#endif
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_GameApplication_Implementation_ProjectSetup);
