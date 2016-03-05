#include <GameFoundation/PCH.h>
#include <GameFoundation/GameApplication/GameApplication.h>
#include <Core/Application/Config/PluginConfig.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#include <RendererDX11/Device/DeviceDX11.h>
typedef ezGALDeviceDX11 ezGALDeviceDefault;
#else
#include <RendererGL/Device/DeviceGL.h>
typedef ezGALDeviceGL ezGALDeviceDefault;
#endif

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
  DoSetupDefaultResources();
  DoSetupGraphicsDevice();
  DoConfigureInput();

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
  ezFileSystem::AddDataDirectory("");// for absolute paths
}


void ezGameApplication::DoSetupDataDirectories()
{
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
  ezTextureResourceHandle hFallbackTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/LoadingTexture_D.dds");
  ezTextureResourceHandle hMissingTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/MissingTexture_D.dds");
  ezMaterialResourceHandle hMissingMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Materials/BaseMaterials/MissingMaterial.ezMaterial");
  ezMaterialResourceHandle hFallbackMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Materials/BaseMaterials/LoadingMaterial.ezMaterial");
  ezMeshResourceHandle hMissingMesh = ezResourceManager::LoadResource<ezMeshResource>("Meshes/MissingMesh/MissingMesh.ezMesh");

  ezTextureResource::SetTypeFallbackResource(hFallbackTexture);
  ezTextureResource::SetTypeMissingResource(hMissingTexture);
  ezMaterialResource::SetTypeFallbackResource(hFallbackMaterial);
  ezMaterialResource::SetTypeMissingResource(hMissingMaterial);
  ezMeshResource::SetTypeMissingResource(hMissingMesh);
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
    ezRenderContext::ConfigureShaderSystem("DX11_SM40", true);
#else
    ezRenderContext::ConfigureShaderSystem("GL3", true);
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

