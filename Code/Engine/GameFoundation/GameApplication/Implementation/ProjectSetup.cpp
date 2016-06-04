#include <GameFoundation/PCH.h>
#include <GameFoundation/GameApplication/GameApplication.h>
#include <Core/Application/Config/PluginConfig.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#include <RendererDX11/Device/DeviceDX11.h>
typedef ezGALDeviceDX11 ezGALDeviceDefault;
#else
#include <RendererGL/Device/DeviceGL.h>
typedef ezGALDeviceGL ezGALDeviceDefault;
#endif
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/JSONReader.h>
#include <GameUtils/Prefabs/PrefabResource.h>

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
}

void ezGameApplication::DoSetupDefaultResources()
{
  {
    ezTextureResourceHandle hFallbackTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/LoadingTexture_D.dds");
    ezTextureResourceHandle hMissingTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/MissingTexture_D.dds");

    ezTextureResource::SetTypeFallbackResource(hFallbackTexture);
    ezTextureResource::SetTypeMissingResource(hMissingTexture);
  }

  {
    ezMaterialResourceHandle hMissingMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Materials/BaseMaterials/MissingMaterial.ezMaterial");
    ezMaterialResourceHandle hFallbackMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Materials/BaseMaterials/LoadingMaterial.ezMaterial");

    ezMaterialResource::SetTypeFallbackResource(hFallbackMaterial);
    ezMaterialResource::SetTypeMissingResource(hMissingMaterial);
  }

  {
    ezMeshResourceHandle hMissingMesh = ezResourceManager::LoadResource<ezMeshResource>("Meshes/MissingMesh.ezMesh");
    ezMeshResource::SetTypeMissingResource(hMissingMesh);
  }

  {
    //ezPrefabResourceDescriptor pd;
    //ezPrefabResourceHandle hMissingPrefab = ezResourceManager::CreateResource<ezPrefabResource>("MissingPrefabResource", pd);

    ezPrefabResourceHandle hMissingPrefab = ezResourceManager::LoadResource<ezPrefabResource>("Prefabs/MissingPrefab.ezObjectGraph");
    ezPrefabResource::SetTypeMissingResource(hMissingPrefab);
  }
}


void ezGameApplication::DoSetupGraphicsDevice()
{
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
    ezShaderManager::Configure("DX11_SM40", true);
#else
    ezShaderManager::Configure("GL3", true);
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

  pDevice->Shutdown();
  EZ_DEFAULT_DELETE(pDevice);
  ezGALDevice::SetDefaultDevice(nullptr);
}

void ezGameApplication::DoShutdownLogWriters()
{

}

