#include <GameFoundation/PCH.h>
#include <GameFoundation/GameApplication/GameApplication.h>
#include <Core/Application/Config/PluginConfig.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#include <RendererDX11/Device/DeviceDX11.h>
typedef ezGALDeviceDX11 ezGALDeviceDefault;
#else
/// \todo We might need a dummy graphics device type
//#include <RendererGL/Device/DeviceGL.h>
//typedef ezGALDeviceGL ezGALDeviceDefault;
#endif
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/JSONReader.h>
#include <GameUtils/Prefabs/PrefabResource.h>
#include <GameUtils/Collection/CollectionResource.h>
#include <RendererCore/Material/MaterialResource.h>
#include <GameUtils/Surfaces/SurfaceResource.h>
#include <GameUtils/Curves/ColorGradientResource.h>

void ezGameApplication::DoProjectSetup()
{
  DoSetupLogWriters();

  ezTelemetry::CreateServer();

  ezApplicationConfig::SetProjectDirectory(FindProjectDirectory());

  DoConfigureFileSystem();
  DoConfigureAssetManagement();
  DoSetupDataDirectories();
  DoLoadCustomPlugins();
  DoLoadPluginsFromConfig();
  DoSetupGraphicsDevice();
  DoSetupDefaultResources();
  DoConfigureInput(false);
  DoLoadTags();

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
  const ezString sUserData = ezOSFile::GetUserDataFolder(m_sAppName);

  ezOSFile::CreateDirectoryStructure(sUserData);

  ezFileSystem::AddDataDirectory("", "GameApplication", ":", ezFileSystem::ReadOnly); // for absolute paths
  ezFileSystem::AddDataDirectory(ezOSFile::GetApplicationDirectory(), "GameApplication", "bin", ezFileSystem::AllowWrites); // writing to the binary directory
  ezFileSystem::AddDataDirectory(ezOSFile::GetApplicationDirectory(), "GameApplication", "shadercache", ezFileSystem::AllowWrites); // for shader files
  ezFileSystem::AddDataDirectory(sUserData, "GameApplication", "appdata", ezFileSystem::AllowWrites); // for writing app user data

  ezApplicationFileSystemConfig appFileSystemConfig;
  appFileSystemConfig.Load();
  appFileSystemConfig.Apply();
}

void ezGameApplication::DoConfigureAssetManagement()
{
  // which redirection table to search
  ezDataDirectory::FolderType::s_sRedirectionFile = "AssetCache/PC.ezAidlt";

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
  ezResourceManager::RegisterResourceForAssetType("Texture 2D", ezGetStaticRTTI<ezTextureResource>());
  ezResourceManager::RegisterResourceForAssetType("Texture 3D", ezGetStaticRTTI<ezTextureResource>());
  ezResourceManager::RegisterResourceForAssetType("Texture Cube", ezGetStaticRTTI<ezTextureResource>());
}

void ezGameApplication::DoSetupDefaultResources()
{
  // Textures
  {
    ezTextureResourceHandle hFallbackTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/LoadingTexture_D.dds");
    ezTextureResourceHandle hMissingTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/MissingTexture_D.dds");

    ezTextureResource::SetTypeFallbackResource(hFallbackTexture);
    ezTextureResource::SetTypeMissingResource(hMissingTexture);
  }

  // Materials
  {
    ezMaterialResourceHandle hMissingMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Materials/BaseMaterials/MissingMaterial.ezMaterial");
    ezMaterialResourceHandle hFallbackMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Materials/BaseMaterials/LoadingMaterial.ezMaterial");

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
    //ezPrefabResourceDescriptor emptyPrefab;
    //ezPrefabResourceHandle hMissingPrefab = ezResourceManager::CreateResource<ezPrefabResource>("MissingPrefabResource", emptyPrefab, "MissingPrefabResource");

    ezPrefabResourceHandle hMissingPrefab = ezResourceManager::LoadResource<ezPrefabResource>("Prefabs/MissingPrefab.ezObjectGraph");
    ezPrefabResource::SetTypeMissingResource(hMissingPrefab);
  }

  // Collections
  {
    ezCollectionResourceDescriptor desc;
    ezCollectionResourceHandle hMissingCollection = ezResourceManager::CreateResource<ezCollectionResource>("MissingCollectionResource", desc, "MissingCollectionResource");

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

    ezColorGradientResourceHandle hResource = ezResourceManager::CreateResource<ezColorGradientResource>("MissingColorGradient", cg, "Missing Color Gradient Resource");
    ezColorGradientResource::SetTypeMissingResource(hResource);
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

    ezGALDevice* pDevice = EZ_DEFAULT_NEW(ezGALDeviceDefault, DeviceInit);
    EZ_VERIFY(pDevice->Init() == EZ_SUCCESS, "Graphics device creation failed!");

    ezGALDevice::SetDefaultDevice(pDevice);

    // Create GPU resource pool
    ezGPUResourcePool* pResourcePool = EZ_DEFAULT_NEW(ezGPUResourcePool);
    ezGPUResourcePool::SetDefaultInstance(pResourcePool);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    ezShaderManager::Configure("DX11_SM50", true);
#else
    ezShaderManager::Configure("GL3", true);
#endif

#endif
}

void ezGameApplication::DoLoadCustomPlugins()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  EZ_VERIFY(ezPlugin::LoadPlugin("ezInspectorPlugin").Succeeded(), "Could not load Inspector Plugin.");

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  EZ_VERIFY(ezPlugin::LoadPlugin("ezShaderCompilerHLSL").Succeeded(), "Could not load HLSL Shader Compiler Plugin.");
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

void ezGameApplication::DoLoadTags()
{
  EZ_LOG_BLOCK("Reading Tags", "Tags.ezManifest");

  ezFileReader file;
  if (file.Open("Tags.ezManifest").Failed())
  {
    ezLog::Dev("'Tags.ezManifest' does not exist");
    return;
  }

  ezJSONReader reader;
  if (reader.Parse(file).Failed())
  {
    ezLog::Error("Failed to read JSON data from tags file");
    return;
  }

  const ezVariantDictionary& dict = reader.GetTopLevelObject();

  ezVariant* pTags = nullptr;
  if (!dict.TryGetValue("Tags", pTags))
  {
    ezLog::Error("Failed to find 'Tags' JSON object in JSON root object");
    return;
  }

  if (!pTags->CanConvertTo<ezVariantArray>())
  {
    ezLog::Error("Failed to cast 'Tags' JSON object into a JSON array");
    return;
  }

  const ezVariantArray& tags = pTags->Get<ezVariantArray>();

  for (const ezVariant& value : tags)
  {
    if (!value.CanConvertTo<ezVariantDictionary>())
    {
      ezLog::Error("Tag is not a JSON object!");
      continue;
    }

    const ezVariantDictionary& tagDict = value.Get<ezVariantDictionary>();

    ezVariant* pName = nullptr;
    tagDict.TryGetValue("Name", pName);
    if (!pName || !pName->IsA<ezString>())
    {
      ezLog::Error("Incomplete tag declaration!");
      continue;
    }

    const char* szTagString = pName->Get<ezString>();

    ezTagRegistry::GetGlobalRegistry().RegisterTag(szTagString);
  }
}

void ezGameApplication::DoShutdownGraphicsDevice()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  for (ezUInt32 i = 0; i < m_Windows.GetCount(); ++i)
  {
    pDevice->DestroySwapChain(m_Windows[i].m_hSwapChain);
  }

  // Cleanup resource pool
  ezGPUResourcePool::SetDefaultInstance(nullptr);

  ezResourceManager::FreeUnusedResources(true);

  pDevice->Shutdown();
  EZ_DEFAULT_DELETE(pDevice);
  ezGALDevice::SetDefaultDevice(nullptr);
}

void ezGameApplication::DoShutdownLogWriters()
{

}

