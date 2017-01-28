#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Threading/TaskSystem.h>

#include <Core/Basics.h>
#include <Core/Application/Application.h>
#include <Core/Input/InputManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/Application/Config/ApplicationConfig.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/Image/ImageConversion.h>

#include <System/Window/Window.h>

#include <RendererDX11/Device/DeviceDX11.h>

#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/TextureLoader.h>

// Constant buffer definition is shared between shader code and C++
#include <RendererCore/../../../Data/Samples/TextureSample/Shaders/SampleConstantBuffer.h>

class TextureSampleWindow : public ezWindow
{
public:

  TextureSampleWindow()
    : ezWindow()
  {
    m_bCloseRequested = false;
  }

  virtual void OnClickCloseMessage() override
  {
    m_bCloseRequested = true;
  }

  bool m_bCloseRequested;
};

static ezUInt32 g_uiWindowWidth = 1280;
static ezUInt32 g_uiWindowHeight = 720;

class CustomTextureResourceLoader : public ezTextureResourceLoader
{
public:
  virtual ezResourceLoadData OpenDataStream(const ezResourceBase* pResource) override;
};

const ezInt32 g_iMaxHalfExtent = 20;
const bool g_bForceImmediateLoading = false;
const bool g_bPreloadAllTextures = false;

class TextureSample : public ezApplication
{
  CustomTextureResourceLoader m_TextureResourceLoader;
  ezConstantBufferStorageHandle m_hSampleConstants;
  ezConstantBufferStorage<ezTextureSampleConstants>* m_pSampleConstantBuffer;

public:

  TextureSample()
  {
    m_vCameraPosition.SetZero();
  }

  void AfterCoreStartup() override
  {
    ezStringBuilder sBaseDir = BUILDSYSTEM_OUTPUT_FOLDER;
    sBaseDir.AppendPath("../../Data/Base/");

    ezStringBuilder sSharedDir = BUILDSYSTEM_OUTPUT_FOLDER;
    sSharedDir.AppendPath("../../Data/FreeContent/");

    ezStringBuilder sProjectDir = BUILDSYSTEM_OUTPUT_FOLDER;
    sProjectDir.AppendPath("../../Data/Samples/TextureSample");

    // setup the 'asset management system'
    {
      // which redirection table to search
      ezDataDirectory::FolderType::s_sRedirectionFile = "AssetCache/LookupTable.ezAsset";
      // which platform assets to use
      ezDataDirectory::FolderType::s_sRedirectionPrefix = "AssetCache/PC/";
    }

    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

    ezFileSystem::AddDataDirectory("", "", ":", ezFileSystem::AllowWrites);
    ezFileSystem::AddDataDirectory(ezOSFile::GetApplicationDirectory(), "AppBin", "bin", ezFileSystem::AllowWrites); // writing to the binary directory
    ezFileSystem::AddDataDirectory(ezOSFile::GetApplicationDirectory(), "ShaderCache", "shadercache", ezFileSystem::AllowWrites); // for shader files
    ezFileSystem::AddDataDirectory(ezOSFile::GetUserDataFolder("ezEngine Project/TextureSample"), "AppData", "appdata", ezFileSystem::AllowWrites); // app user data

    ezFileSystem::AddDataDirectory(sBaseDir.GetData(), "Base", "base");
    ezFileSystem::AddDataDirectory(sSharedDir.GetData(), "Shared", "shared");
    ezFileSystem::AddDataDirectory(sProjectDir.GetData(), "Project", "project", ezFileSystem::AllowWrites);

    ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
    ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

    ezTelemetry::CreateServer();
    ezPlugin::LoadPlugin("ezInspectorPlugin");

    EZ_VERIFY(ezPlugin::LoadPlugin("ezShaderCompilerHLSL").Succeeded(), "Compiler Plugin not found");

    // Register Input
    {
      ezInputActionConfig cfg;

      cfg = ezInputManager::GetInputActionConfig("Main", "CloseApp");
      cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyEscape;
      ezInputManager::SetInputActionConfig("Main", "CloseApp", cfg, true);

      cfg = ezInputManager::GetInputActionConfig("Main", "MovePosX");
      cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMovePosX;
      cfg.m_bApplyTimeScaling = false;
      ezInputManager::SetInputActionConfig("Main", "MovePosX", cfg, true);

      cfg = ezInputManager::GetInputActionConfig("Main", "MoveNegX");
      cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMoveNegX;
      cfg.m_bApplyTimeScaling = false;
      ezInputManager::SetInputActionConfig("Main", "MoveNegX", cfg, true);

      cfg = ezInputManager::GetInputActionConfig("Main", "MovePosY");
      cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMovePosY;
      cfg.m_bApplyTimeScaling = false;
      ezInputManager::SetInputActionConfig("Main", "MovePosY", cfg, true);

      cfg = ezInputManager::GetInputActionConfig("Main", "MoveNegY");
      cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMoveNegY;
      cfg.m_bApplyTimeScaling = false;
      ezInputManager::SetInputActionConfig("Main", "MoveNegY", cfg, true);

      cfg = ezInputManager::GetInputActionConfig("Main", "MouseDown");
      cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseButton0;
      cfg.m_bApplyTimeScaling = false;
      ezInputManager::SetInputActionConfig("Main", "MouseDown", cfg, true);
    }

    // Create a window for rendering
    {
      ezWindowCreationDesc WindowCreationDesc;
      WindowCreationDesc.m_Resolution.width = g_uiWindowWidth;
      WindowCreationDesc.m_Resolution.height = g_uiWindowHeight;
      m_pWindow = EZ_DEFAULT_NEW(TextureSampleWindow);
      m_pWindow->Initialize(WindowCreationDesc);
    }

    // Create a device
    {
      ezGALDeviceCreationDescription DeviceInit;
      DeviceInit.m_bCreatePrimarySwapChain = true;
      DeviceInit.m_bDebugDevice = true;
      DeviceInit.m_PrimarySwapChainDescription.m_pWindow = m_pWindow;
      DeviceInit.m_PrimarySwapChainDescription.m_SampleCount = ezGALMSAASampleCount::None;
      DeviceInit.m_PrimarySwapChainDescription.m_bAllowScreenshots = true;
      DeviceInit.m_PrimarySwapChainDescription.m_bVerticalSynchronization = true;

      m_pDevice = EZ_DEFAULT_NEW(ezGALDeviceDX11, DeviceInit);
      EZ_VERIFY(m_pDevice->Init() == EZ_SUCCESS, "Device init failed!");

      ezGALDevice::SetDefaultDevice(m_pDevice);
    }

    // now that we have a window and device, tell the engine to initialize the rendering infrastructure
    ezStartup::StartupEngine();


    // Get the primary swapchain (this one will always be created by device init except if the user instructs no swap chain creation explicitly)
    {
      ezGALSwapChainHandle hPrimarySwapChain = m_pDevice->GetPrimarySwapChain();
      const ezGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(hPrimarySwapChain);

      ezGALTextureCreationDescription texDesc;
      texDesc.m_uiWidth = g_uiWindowWidth;
      texDesc.m_uiHeight = g_uiWindowHeight;
      texDesc.m_Format = ezGALResourceFormat::D24S8;
      texDesc.m_bCreateRenderTarget = true;

      m_hDepthStencilTexture = m_pDevice->CreateTexture(texDesc);

      m_hBBRTV = m_pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetBackBufferTexture());
      m_hBBDSV = m_pDevice->GetDefaultRenderTargetView(m_hDepthStencilTexture);
    }

    // Create Rasterizer State
    {
      ezGALRasterizerStateCreationDescription RasterStateDesc;
      RasterStateDesc.m_CullMode = ezGALCullMode::Back;
      RasterStateDesc.m_bFrontCounterClockwise = true;
      m_hRasterizerState = m_pDevice->CreateRasterizerState(RasterStateDesc);
      EZ_ASSERT_DEV(!m_hRasterizerState.IsInvalidated(), "Couldn't create rasterizer state!");
    }

    // Create Depth Stencil state
    {
      ezGALDepthStencilStateCreationDescription DepthStencilStateDesc;
      DepthStencilStateDesc.m_bDepthTest = false;
      DepthStencilStateDesc.m_bDepthWrite = true;
      m_hDepthStencilState = m_pDevice->CreateDepthStencilState(DepthStencilStateDesc);
      EZ_ASSERT_DEV(!m_hDepthStencilState.IsInvalidated(), "Couldn't create depth-stencil state!");
    }

    // Setup Shaders and Materials
    {
      ezShaderManager::Configure("DX11_SM50", true);

      m_hMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Materials/Texture.ezMaterial");

      // Create the mesh that we use for rendering
      CreateSquareMesh();
    }

    // Setup default resources
    {
      ezTexture2DResourceHandle hFallback = ezResourceManager::LoadResource<ezTexture2DResource>("Textures/Reference_D.dds");
      ezTexture2DResourceHandle hMissing = ezResourceManager::LoadResource<ezTexture2DResource>("Textures/MissingTexture_D.dds");

      ezTexture2DResource::SetTypeFallbackResource(hFallback);
      ezTexture2DResource::SetTypeMissingResource(hMissing);

      // redirect all texture load operations through our custom loader, so that we can duplicate the single source texture
      // that we have as often as we like (to waste memory)
      ezResourceManager::SetResourceTypeLoader<ezTexture2DResource>(&m_TextureResourceLoader);
    }

    // Setup constant buffer that this sample uses
    {
      m_hSampleConstants = ezRenderContext::CreateConstantBufferStorage(m_pSampleConstantBuffer);
    }

    // Pre-allocate all textures
    {
      // we only do this to be able to see the unloaded resources in the ezInspector
      // this does NOT preload the resources

      ezStringBuilder sResourceName;
      for (ezInt32 y = -g_iMaxHalfExtent; y < g_iMaxHalfExtent; ++y)
      {
        for (ezInt32 x = -g_iMaxHalfExtent; x < g_iMaxHalfExtent; ++x)
        {
          sResourceName.Printf("Loaded_%+03i_%+03i_D",
                               x, y);

          ezTexture2DResourceHandle hTexture = ezResourceManager::LoadResource<ezTexture2DResource>(sResourceName);

          if (g_bPreloadAllTextures)
            ezResourceManager::PreloadResource(hTexture, ezTime::Seconds(1.0));
        }
      }
    }
  }


  ApplicationExecution Run() override
  {
    m_pWindow->ProcessWindowMessages();

    if (m_pWindow->m_bCloseRequested || ezInputManager::GetInputActionState("Main", "CloseApp") == ezKeyState::Pressed)
      return ApplicationExecution::Quit;

    // make sure time goes on
    ezClock::GetGlobalClock()->Update();

    if (ezInputManager::GetInputActionState("Main", "MouseDown") == ezKeyState::Down)
    {
      float fInputValue = 0.0f;
      const float fMouseSpeed = 0.5f;

      if (ezInputManager::GetInputActionState("Main", "MovePosX", &fInputValue) != ezKeyState::Up)
        m_vCameraPosition.x -= fInputValue * fMouseSpeed;
      if (ezInputManager::GetInputActionState("Main", "MoveNegX", &fInputValue) != ezKeyState::Up)
        m_vCameraPosition.x += fInputValue * fMouseSpeed;
      if (ezInputManager::GetInputActionState("Main", "MovePosY", &fInputValue) != ezKeyState::Up)
        m_vCameraPosition.y += fInputValue * fMouseSpeed;
      if (ezInputManager::GetInputActionState("Main", "MoveNegY", &fInputValue) != ezKeyState::Up)
        m_vCameraPosition.y -= fInputValue * fMouseSpeed;
    }

    // update all input state
    ezInputManager::Update(ezClock::GetGlobalClock()->GetTimeDiff());

    // make sure telemetry is sent out regularly
    ezTelemetry::PerFrameUpdate();

    // do the rendering
    {
      // Before starting to render in a frame call this function
      m_pDevice->BeginFrame();

      // The ezGALContext class is the main interaction point for draw / compute operations
      ezGALContext* pContext = m_pDevice->GetPrimaryContext();


      ezGALRenderTagetSetup RTS;
      RTS.SetRenderTarget(0, m_hBBRTV)
         .SetDepthStencilTarget(m_hBBDSV);

      pContext->SetRenderTargetSetup(RTS);
      pContext->SetViewport(ezRectFloat(0.0f, 0.0f, (float) g_uiWindowWidth, (float) g_uiWindowHeight), 0.0f, 1.0f);
      pContext->Clear(ezColor::Black);

      pContext->SetRasterizerState(m_hRasterizerState);
      pContext->SetDepthStencilState(m_hDepthStencilState);

      ezMat4 Proj;
      Proj.SetIdentity();
      Proj.SetOrthographicProjectionMatrix(m_vCameraPosition.x + -(float) g_uiWindowWidth * 0.5f, m_vCameraPosition.x + (float) g_uiWindowWidth * 0.5f, m_vCameraPosition.y + -(float) g_uiWindowHeight * 0.5f, m_vCameraPosition.y + (float) g_uiWindowHeight * 0.5f, -1.0f, 1.0f);

      ezRenderContext::GetDefaultInstance()->BindConstantBuffer("ezTextureSampleConstants", m_hSampleConstants);
      ezRenderContext::GetDefaultInstance()->BindMaterial(m_hMaterial);

      ezMat4 mTransform;
      mTransform.SetIdentity();

      ezInt32 iLeftBound = (ezInt32) ezMath::Floor((m_vCameraPosition.x - g_uiWindowWidth  * 0.5f) / 100.0f);
      ezInt32 iLowerBound = (ezInt32) ezMath::Floor((m_vCameraPosition.y - g_uiWindowHeight * 0.5f) / 100.0f);
      ezInt32 iRightBound = (ezInt32) ezMath::Ceil((m_vCameraPosition.x + g_uiWindowWidth  * 0.5f) / 100.0f) + 1;
      ezInt32 iUpperBound = (ezInt32) ezMath::Ceil((m_vCameraPosition.y + g_uiWindowHeight * 0.5f) / 100.0f) + 1;

      iLeftBound = ezMath::Max(iLeftBound, -g_iMaxHalfExtent);
      iRightBound = ezMath::Min(iRightBound, g_iMaxHalfExtent);
      iLowerBound = ezMath::Max(iLowerBound, -g_iMaxHalfExtent);
      iUpperBound = ezMath::Min(iUpperBound, g_iMaxHalfExtent);

      ezStringBuilder sResourceName;

      for (ezInt32 y = iLowerBound; y < iUpperBound; ++y)
      {
        for (ezInt32 x = iLeftBound; x < iRightBound; ++x)
        {
          mTransform.SetTranslationVector(ezVec3((float) x * 100.0f, (float) y * 100.0f, 0));

          // Update the constant buffer
          {
            ezTextureSampleConstants& cb = m_pSampleConstantBuffer->GetDataForWriting();
            cb.ModelMatrix = mTransform;
            cb.ViewProjectionMatrix = Proj;
          }

          sResourceName.Printf("Loaded_%+03i_%+03i_D",
                               x, y);

          ezTexture2DResourceHandle hTexture = ezResourceManager::LoadResource<ezTexture2DResource>(sResourceName, ezResourcePriority::Highest, ezTexture2DResourceHandle());

          // force immediate loading
          if (g_bForceImmediateLoading)
            ezResourceLock<ezTexture2DResource> l(hTexture, ezResourceAcquireMode::NoFallback);

          ezRenderContext::GetDefaultInstance()->BindTexture2D(ezGALShaderStage::PixelShader, "DiffuseTexture", hTexture);
          ezRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hQuadMeshBuffer);
          ezRenderContext::GetDefaultInstance()->DrawMeshBuffer();
        }
      }

      m_pDevice->Present(m_pDevice->GetPrimarySwapChain());

      m_pDevice->EndFrame();
    }

    // needs to be called once per frame
    ezResourceManager::PerFrameUpdate();

    // tell the task system to finish its work for this frame
    // this has to be done at the very end, so that the task system will only use up the time that is left in this frame for
    // uploading GPU data etc.
    ezTaskSystem::FinishFrameTasks();

    return ezApplication::Continue;
  }

  void BeforeCoreShutdown() override
  {
    // make sure that no textures are continue to be streamed in while the engine shuts down
    ezResourceManager::EngineAboutToShutdown();

    ezRenderContext::DeleteConstantBufferStorage(m_hSampleConstants);
    m_hSampleConstants.Invalidate();

    m_pDevice->DestroyTexture(m_hDepthStencilTexture);
    m_hDepthStencilTexture.Invalidate();

    m_hMaterial.Invalidate();
    m_hQuadMeshBuffer.Invalidate();

    // tell the engine that we are about to destroy window and graphics device,
    // and that it therefore needs to cleanup anything that depends on that
    ezStartup::ShutdownEngine();

    m_pDevice->DestroyRasterizerState(m_hRasterizerState);
    m_pDevice->DestroyDepthStencilState(m_hDepthStencilState);

    // now we can destroy the graphics device
    m_pDevice->Shutdown();

    EZ_DEFAULT_DELETE(m_pDevice);

    // finally destroy the window
    m_pWindow->Destroy();
    EZ_DEFAULT_DELETE(m_pWindow);
  }

  void CreateSquareMesh()
  {
    struct Vertex
    {
      ezVec3 Position;
      ezVec2 TexCoord0;
    };

    ezGeometry geom;
    geom.AddRectXY(ezVec2(100, 100), ezColor::Black);

    ezDynamicArray<Vertex> Vertices;
    ezDynamicArray<ezUInt16> Indices;

    Vertices.Reserve(geom.GetVertices().GetCount());
    Indices.Reserve(geom.GetPolygons().GetCount() * 6);

    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    desc.AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::UVFloat);

    desc.AllocateStreams(geom.GetVertices().GetCount(), ezGALPrimitiveTopology::Triangles, geom.GetPolygons().GetCount() * 2);

    for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
    {
      ezVec2 tc(geom.GetVertices()[v].m_vPosition.x / 100.0f, geom.GetVertices()[v].m_vPosition.y / -100.0f);
      tc += ezVec2(0.5f);

      desc.SetVertexData<ezVec3>(0, v, geom.GetVertices()[v].m_vPosition);
      desc.SetVertexData<ezVec2>(1, v, tc);
    }

    ezUInt32 t = 0;
    for (ezUInt32 p = 0; p < geom.GetPolygons().GetCount(); ++p)
    {
      for (ezUInt32 v = 0; v < geom.GetPolygons()[p].m_Vertices.GetCount() - 2; ++v)
      {
        desc.SetTriangleIndices(t, geom.GetPolygons()[p].m_Vertices[0], geom.GetPolygons()[p].m_Vertices[v + 1], geom.GetPolygons()[p].m_Vertices[v + 2]);

        ++t;
      }
    }

    m_hQuadMeshBuffer = ezResourceManager::GetExistingResource<ezMeshBufferResource>("{E692442B-9E15-46C5-8A00-1B07C02BF8F7}");

    if (!m_hQuadMeshBuffer.IsValid())
      m_hQuadMeshBuffer = ezResourceManager::CreateResource<ezMeshBufferResource>("{E692442B-9E15-46C5-8A00-1B07C02BF8F7}", desc);
  }

private:

  TextureSampleWindow* m_pWindow;
  ezGALDevice* m_pDevice;

  ezGALRenderTargetViewHandle m_hBBRTV;
  ezGALRenderTargetViewHandle m_hBBDSV;
  ezGALTextureHandle m_hDepthStencilTexture;

  ezGALRasterizerStateHandle m_hRasterizerState;
  ezGALDepthStencilStateHandle m_hDepthStencilState;

  ezMaterialResourceHandle m_hMaterial;
  ezMeshBufferResourceHandle m_hQuadMeshBuffer;

  ezVec2 m_vCameraPosition;
};

ezResourceLoadData CustomTextureResourceLoader::OpenDataStream(const ezResourceBase* pResource)
{
  ezString sFileToLoad = pResource->GetResourceID();

  if (sFileToLoad.StartsWith("Loaded"))
  {
    sFileToLoad = "Textures/Loaded_D.dds"; // redirect all "Loaded_XYZ" files to the same source file
  }

  // the entire rest is copied from ezTextureResourceLoader

  LoadedData* pData = EZ_DEFAULT_NEW(LoadedData);

  ezResourceLoadData res;

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)
  {
    ezFileReader File;
    if (File.Open(sFileToLoad).Failed())
      return res;

    ezFileStats stat;
    if (ezOSFile::GetFileStats(File.GetFilePathAbsolute(), stat).Succeeded())
    {
      res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
    }
  }
#endif


  if (pData->m_Image.LoadFrom(sFileToLoad).Failed())
    return res;

  if (pData->m_Image.GetImageFormat() == ezImageFormat::B8G8R8_UNORM)
  {
    ezImageConversion::Convert(pData->m_Image, pData->m_Image, ezImageFormat::B8G8R8A8_UNORM);
  }

  ezMemoryStreamWriter w(&pData->m_Storage);

  ezImage* pImage = &pData->m_Image;
  w.WriteBytes(&pImage, sizeof(ezImage*));

  /// This is a hack to get the SRGB information for the texture

  const ezStringBuilder sName = ezPathUtils::GetFileName(sFileToLoad);

  bool bSRGB = (sName.EndsWith_NoCase("_D") || sName.EndsWith_NoCase("_SRGB") || sName.EndsWith_NoCase("_diff"));

  w << bSRGB;

  res.m_pDataStream = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

  return res;
}

EZ_CONSOLEAPP_ENTRY_POINT(TextureSample);


