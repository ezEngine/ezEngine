#include <PCH.h>

#include <Core/Application/Config/FileSystemConfig.h>
#include <Core/Application/Config/PluginConfig.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <GameEngine/Collection/CollectionResource.h>
#include <GameEngine/Curves/ColorGradientResource.h>
#include <GameEngine/Curves/Curve1DResource.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/Prefabs/PrefabResource.h>
#include <GameEngine/Resources/PropertyAnimResource.h>
#include <GameEngine/Surfaces/SurfaceResource.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererFoundation/Device/Device.h>
#include <VisualScript/VisualScriptResource.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#include <RendererDX11/Device/DeviceDX11.h>
typedef ezGALDeviceDX11 ezGALDeviceDefault;
#else
/// \todo We might need a dummy graphics device type
//#include <RendererGL/Device/DeviceGL.h>
// typedef ezGALDeviceGL ezGALDeviceDefault;
#endif

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#include <WindowsMixedReality/Graphics/MixedRealityDX11Device.h>
#endif


void ezGameApplication::DoProjectSetup()
{
  m_PlatformProfile.m_sName = ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-profile", 0, GetPreferredPlatformProfile());
  m_PlatformProfile.AddMissingConfigs();

  DoSetupLogWriters();

  ezTelemetry::CreateServer();

  ezFileSystem::SetSpecialDirectory("project", FindProjectDirectory());

  DoConfigureFileSystem();
  DoConfigureAssetManagement();
  DoLoadCustomPlugins();
  DoSetupDataDirectories();
  DoLoadPluginsFromConfig();
  DoLoadPlatformProfile();
  DoConfigureInput(false);
  DoLoadTags();

  ezCVar::SetStorageFolder(":appdata/CVars");
  ezCVar::LoadCVars();

  ezTaskSystem::SetTargetFrameTime(1000.0 / 20.0);
}

void ezGameApplication::DoSetupLogWriters()
{
  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
}

void ezGameApplication::DoConfigureFileSystem()
{
  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
}


void ezGameApplication::DoSetupDataDirectories()
{
  const ezStringBuilder sUserData(">user/", m_sAppName);

  ezFileSystem::CreateDirectoryStructure(sUserData);

  // TODO: Application directory is not writable in UWP (and probably other platforms). We need a more elegant solution than this.
  ezString writableBinRoot = ">appdir/";
  ezString shaderCacheRoot = ">appdir/";
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  writableBinRoot = sUserData;
  shaderCacheRoot = sUserData;
#endif

  ezFileSystem::AddDataDirectory("", "GameApplication", ":", ezFileSystem::ReadOnly);                   // for absolute paths
  ezFileSystem::AddDataDirectory(writableBinRoot, "GameApplication", "bin", ezFileSystem::AllowWrites); // writing to the binary directory
  ezFileSystem::AddDataDirectory(shaderCacheRoot, "GameApplication", "shadercache", ezFileSystem::AllowWrites); // for shader files
  ezFileSystem::AddDataDirectory(sUserData, "GameApplication", "appdata", ezFileSystem::AllowWrites);           // for writing app user data

  // setup the main data directories ":base/" and ":project/"
  {
    ezFileSystem::AddDataDirectory(">sdk/Data/Base", "GameApplication", "base", ezFileSystem::DataDirUsage::ReadOnly);
    ezFileSystem::AddDataDirectory(">project/", "GameApplication", "project", ezFileSystem::DataDirUsage::ReadOnly);
  }

  ezApplicationFileSystemConfig appFileSystemConfig;
  appFileSystemConfig.Load();

  // get rid of duplicates that we already hardcoded above
  for (ezUInt32 i = appFileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const ezString name = appFileSystemConfig.m_DataDirs[i - 1].m_sRootName;
    if (name.IsEqual_NoCase("base") || name.IsEqual_NoCase("project") || name.IsEqual_NoCase("bin") || name.IsEqual_NoCase("shadercache") ||
        name.IsEqual_NoCase(":") || name.IsEqual_NoCase("appdata"))
    {
      appFileSystemConfig.m_DataDirs.RemoveAtAndCopy(i - 1);
    }
  }

  appFileSystemConfig.Apply();
}

void ezGameApplication::DoConfigureAssetManagement()
{
  const ezStringBuilder sAssetRedirFile("AssetCache/", m_PlatformProfile.m_sName, ".ezAidlt");

  // which redirection table to search
  ezDataDirectory::FolderType::s_sRedirectionFile = sAssetRedirFile;

  // which platform assets to use
  ezDataDirectory::FolderType::s_sRedirectionPrefix = "AssetCache/";

  ezResourceManager::RegisterResourceForAssetType("Collection", ezGetStaticRTTI<ezCollectionResource>());
  // collision mesh resources registered by PhysX plugin
  ezResourceManager::RegisterResourceForAssetType("Material", ezGetStaticRTTI<ezMaterialResource>());
  ezResourceManager::RegisterResourceForAssetType("Mesh", ezGetStaticRTTI<ezMeshResource>());
  ezResourceManager::RegisterResourceForAssetType("Prefab", ezGetStaticRTTI<ezPrefabResource>());
  ezResourceManager::RegisterResourceForAssetType("RenderPipeline", ezGetStaticRTTI<ezRenderPipelineResource>());
  // sound bank resource registered by Fmod plugin
  ezResourceManager::RegisterResourceForAssetType("Surface", ezGetStaticRTTI<ezSurfaceResource>());
  ezResourceManager::RegisterResourceForAssetType("Texture 2D", ezGetStaticRTTI<ezTexture2DResource>());
  ezResourceManager::RegisterResourceForAssetType("Render Target", ezGetStaticRTTI<ezTexture2DResource>());
  // ezResourceManager::RegisterResourceForAssetType("Texture 3D", ezGetStaticRTTI<ezTexture3DResource>());
  ezResourceManager::RegisterResourceForAssetType("Texture Cube", ezGetStaticRTTI<ezTextureCubeResource>());
}

void ezGameApplication::DoSetupDefaultResources()
{
  // Shaders
  {
    ezShaderResourceDescriptor desc;
    ezShaderResourceHandle hFallbackShader =
        ezResourceManager::CreateResource<ezShaderResource>("FallbackShaderResource", desc, "FallbackShaderResource");
    ezShaderResourceHandle hMissingShader =
        ezResourceManager::CreateResource<ezShaderResource>("MissingShaderResource", desc, "MissingShaderResource");

    ezShaderResource::SetTypeFallbackResource(hFallbackShader);
    ezShaderResource::SetTypeMissingResource(hMissingShader);
  }

  // Shader Permutation
  {
    ezShaderPermutationResourceDescriptor desc;
    ezShaderPermutationResourceHandle hFallbackShaderPermutation = ezResourceManager::CreateResource<ezShaderPermutationResource>(
        "FallbackShaderPermutationResource", desc, "FallbackShaderPermutationResource");

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
        ezResourceManager::CreateResource<ezCollectionResource>("MissingCollectionResource", desc, "MissingCollectionResource");

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

    ezColorGradientResourceHandle hResource =
        ezResourceManager::CreateResource<ezColorGradientResource>("MissingColorGradient", cg, "Missing Color Gradient Resource");
    ezColorGradientResource::SetTypeMissingResource(hResource);
  }

  // 1D Curve
  {
    ezCurve1DResourceDescriptor cd;
    auto& curve = cd.m_Curves.ExpandAndGetRef();
    curve.AddControlPoint(0);
    curve.AddControlPoint(1);

    ezCurve1DResourceHandle hResource =
        ezResourceManager::CreateResource<ezCurve1DResource>("MissingCurve1D", cd, "Missing Curve1D Resource");
    ezCurve1DResource::SetTypeMissingResource(hResource);
  }

  // Visual Script
  {
    ezVisualScriptResourceDescriptor desc;

    ezVisualScriptResourceHandle hResource =
        ezResourceManager::CreateResource<ezVisualScriptResource>("MissingVisualScript", desc, "Missing Visual Script Resource");
    ezVisualScriptResource::SetTypeMissingResource(hResource);
  }

  // Property Animations
  {
    ezPropertyAnimResourceDescriptor desc;
    desc.m_AnimationDuration = ezTime::Seconds(0.1);

    ezPropertyAnimResourceHandle hResource =
        ezResourceManager::CreateResource<ezPropertyAnimResource>("MissingPropertyAnim", desc, "Missing Property Animation Resource");
    ezPropertyAnimResource::SetTypeMissingResource(hResource);
  }

  // Animation Skeleton
  {
    ezSkeletonResourceDescriptor desc;

    ezSkeletonResourceHandle hResource =
        ezResourceManager::CreateResource<ezSkeletonResource>("MissingSkeleton", desc, "Missing Skeleton Resource");
    ezSkeletonResource::SetTypeMissingResource(hResource);
  }

  // Animation Clip
  {
    ezAnimationClipResourceDescriptor desc;

    ezAnimationClipResourceHandle hResource =
        ezResourceManager::CreateResource<ezAnimationClipResource>("MissingAnimationClip", desc, "Missing Animation Clip Resource");
    ezAnimationClipResource::SetTypeMissingResource(hResource);
  }
}


void ezGameApplication::DoSetupGraphicsDevice()
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  ezGALDeviceCreationDescription DeviceInit;
  DeviceInit.m_bCreatePrimarySwapChain = false;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  DeviceInit.m_bDebugDevice = true;
#endif

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

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  ezShaderManager::Configure("DX11_SM50", true);
#else
  EZ_ASSERT_NOT_IMPLEMENTED;
  ezShaderManager::Configure("GL3", true);
#endif

#endif
}

void ezGameApplication::DoLoadCustomPlugins()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezPlugin::LoadPlugin("ezInspectorPlugin");

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  ezPlugin::LoadPlugin("ezShaderCompilerHLSL");
#endif

  // on sandboxed platforms, we can only load data through fileserve, so enforce use of this plugin
#if EZ_DISABLED(EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  ezPlugin::LoadPlugin("ezFileservePlugin"); // don't care if it fails to load
#endif

#endif
}

void ezGameApplication::DoLoadPluginsFromConfig()
{
  ezApplicationPluginConfig appPluginConfig;
  appPluginConfig.Load();
  appPluginConfig.SetOnlyLoadManualPlugins(true);
  appPluginConfig.Apply();
}

void ezGameApplication::DoLoadPlatformProfile()
{
  // re-load this after we know all the custom config types
  const ezStringBuilder sRuntimeProfileFile(":project/RuntimeConfigs/", m_PlatformProfile.m_sName, ".ezProfile");
  m_PlatformProfile.AddMissingConfigs();
  m_PlatformProfile.LoadForRuntime(sRuntimeProfileFile);
}

void ezGameApplication::DoLoadTags()
{
  EZ_LOG_BLOCK("Reading Tags", "Tags.ddl");

  ezFileReader file;
  if (file.Open(":project/Tags.ddl").Failed())
  {
    ezLog::Dev("'Tags.ddl' does not exist");
    return;
  }

  ezStringBuilder tmp;

  ezOpenDdlReader reader;
  if (reader.ParseDocument(file).Failed())
  {
    ezLog::Error("Failed to parse DDL data in tags file");
    return;
  }

  const ezOpenDdlReaderElement* pRoot = reader.GetRootElement();

  for (const ezOpenDdlReaderElement* pTags = pRoot->GetFirstChild(); pTags != nullptr; pTags = pTags->GetSibling())
  {
    if (!pTags->IsCustomType("Tag"))
      continue;

    const ezOpenDdlReaderElement* pName = pTags->FindChildOfType(ezOpenDdlPrimitiveType::String, "Name");

    if (!pName)
    {
      ezLog::Error("Incomplete tag declaration!");
      continue;
    }

    tmp = pName->GetPrimitivesString()[0];
    ezTagRegistry::GetGlobalRegistry().RegisterTag(tmp);
  }
}

void ezGameApplication::DoShutdownGraphicsDevice()
{
  if (!ezGALDevice::HasDefaultDevice())
    return;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  for (ezUInt32 i = 0; i < m_Windows.GetCount(); ++i)
  {
    if (m_Windows[i].m_hSwapChain != pDevice->GetPrimarySwapChain())
      pDevice->DestroySwapChain(m_Windows[i].m_hSwapChain);
  }

  // Cleanup resource pool
  ezGPUResourcePool::SetDefaultInstance(nullptr);

  ezResourceManager::FreeUnusedResources(true);

  pDevice->Shutdown();
  EZ_DEFAULT_DELETE(pDevice);
  ezGALDevice::SetDefaultDevice(nullptr);
}

void ezGameApplication::DoShutdownLogWriters() {}



EZ_STATICLINK_FILE(GameEngine, GameEngine_GameApplication_Implementation_ProjectSetup);
