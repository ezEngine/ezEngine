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

#include <CoreUtils/Geometry/GeomUtils.h>

#include <System/Window/Window.h>

#include <RendererDX11/Device/DeviceDX11.h>

#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Context/Context.h>

#include <RendererCore/RendererCore.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/ConstantBuffers/ConstantBufferResource.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>

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

class TextureSample : public ezApplication
{
public:

  void AfterEngineInit() override
  {
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
    ezFileSystem::AddDataDirectory("");

    ezStringBuilder sReadDir = BUILDSYSTEM_OUTPUT_FOLDER;
    sReadDir.AppendPath("../../Shared/Samples/TextureSample/");

    ezStringBuilder sBaseDir = BUILDSYSTEM_OUTPUT_FOLDER;
    sBaseDir.AppendPath("../../Shared/Data/");

    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
    ezFileSystem::AddDataDirectory(sBaseDir.GetData(), ezFileSystem::ReadOnly, "Shared");
    ezFileSystem::AddDataDirectory(sReadDir.GetData(), ezFileSystem::AllowWrites, "Sample");

    ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
    ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

    ezTelemetry::CreateServer();
    ezPlugin::LoadPlugin("ezInspectorPlugin");

    EZ_VERIFY(ezPlugin::LoadPlugin("ezShaderCompilerHLSL").Succeeded(), "Compiler Plugin not found");

    ezClock::SetNumGlobalClocks();

    // Register Input
    {
      ezInputActionConfig cfg;

      cfg = ezInputManager::GetInputActionConfig("Main", "CloseApp");
      cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyEscape;
      ezInputManager::SetInputActionConfig("Main", "CloseApp", cfg, true);
    }

    // Create a window for rendering
    {
      ezWindowCreationDesc WindowCreationDesc;
      WindowCreationDesc.m_ClientAreaSize.width = g_uiWindowWidth;
      WindowCreationDesc.m_ClientAreaSize.height = g_uiWindowHeight;
      m_pWindow = EZ_DEFAULT_NEW(TextureSampleWindow)();
      m_pWindow->Initialize(WindowCreationDesc);
    }

    // Create a device
    {
      ezGALDeviceCreationDescription DeviceInit;
      DeviceInit.m_bCreatePrimarySwapChain = true;
      DeviceInit.m_bDebugDevice = true;
      DeviceInit.m_PrimarySwapChainDescription.m_pWindow = m_pWindow;
      DeviceInit.m_PrimarySwapChainDescription.m_SampleCount = ezGALMSAASampleCount::None;
      DeviceInit.m_PrimarySwapChainDescription.m_bCreateDepthStencilBuffer = true;
      DeviceInit.m_PrimarySwapChainDescription.m_DepthStencilBufferFormat = ezGALResourceFormat::D24S8;
      DeviceInit.m_PrimarySwapChainDescription.m_bAllowScreenshots = true;
      DeviceInit.m_PrimarySwapChainDescription.m_bVerticalSynchronization = true;

      m_pDevice = EZ_DEFAULT_NEW(ezGALDeviceDX11)(DeviceInit);
      EZ_VERIFY(m_pDevice->Init() == EZ_SUCCESS, "Device init failed!");

      ezGALDevice::SetDefaultDevice(m_pDevice);
    }

    // now that we have a window and device, tell the engine to initialize the rendering infrastructure
    ezStartup::StartupEngine();


    // Get the primary swapchain (this one will always be created by device init except if the user instructs no swap chain creation explicitly)
    {
      ezGALSwapChainHandle hPrimarySwapChain = m_pDevice->GetPrimarySwapChain();
      const ezGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(hPrimarySwapChain);

      m_hBBRT = pPrimarySwapChain->GetRenderTargetViewConfig();
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
      ezRendererCore::SetShaderPlatform("DX11_SM40", true);

      m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Texture.shader");
      m_hMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Materials/Texture.material");

      ezRendererCore::SetActiveShader(m_hShader);

      // Create the mesh that we use for rendering
      CreateSquareMesh();
    }
  }


  ApplicationExecution Run() override
  {
    m_pWindow->ProcessWindowMessages();

    if (m_pWindow->m_bCloseRequested || ezInputManager::GetInputActionState("Main", "CloseApp") == ezKeyState::Pressed)
      return ApplicationExecution::Quit;

    // make sure time goes on
    ezClock::UpdateAllGlobalClocks();

    // update all input state
    ezInputManager::Update(ezClock::Get()->GetTimeDiff());

    // make sure telemetry is sent out regularly
    ezTelemetry::PerFrameUpdate();

    // do the rendering
    {
      // Before starting to render in a frame call this function
      m_pDevice->BeginFrame();

      // The ezGALContext class is the main interaction point for draw / compute operations
      ezGALContext* pContext = m_pDevice->GetPrimaryContext();

      pContext->SetRenderTargetConfig(m_hBBRT);
      pContext->SetViewport(0.0f, 0.0f, (float) g_uiWindowWidth, (float) g_uiWindowHeight, 0.0f, 1.0f);
      pContext->Clear(ezColor::Black);

      pContext->SetRasterizerState(m_hRasterizerState);
      pContext->SetDepthStencilState(m_hDepthStencilState);

      ezMat4 Proj;
      Proj.SetIdentity();
      Proj.SetOrthographicProjectionMatrix((float) g_uiWindowWidth, (float) g_uiWindowHeight, -1.0f, 1.0f, ezProjectionDepthRange::ZeroToOne);

      ezRendererCore::SetMaterialParameter("ModelViewProjection", Proj);

      ezRendererCore::SetMaterialState(pContext, m_hMaterial);

      ezRendererCore::DrawMeshBuffer(pContext, m_hQuadMeshBuffer);

      m_pDevice->Present(m_pDevice->GetPrimarySwapChain());

      m_pDevice->EndFrame();
    }

    // dump all errors that might have occurred during rendering
    ezRendererCore::OutputErrors();

    // tell the task system to finish its work for this frame
    // this has to be done at the very end, so that the task system will only use up the time that is left in this frame for
    // uploading GPU data etc.
    ezTaskSystem::FinishFrameTasks();

    return ezApplication::Continue;
  }

  void BeforeEngineShutdown() override
  {
    m_hMaterial.Invalidate();
    m_hShader.Invalidate();
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

    desc.AllocateStreams(geom.GetVertices().GetCount(), geom.GetPolygons().GetCount() * 2);

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

  ezGALRenderTargetConfigHandle m_hBBRT;

  ezGALRasterizerStateHandle m_hRasterizerState;
  ezGALDepthStencilStateHandle m_hDepthStencilState;

  ezShaderResourceHandle m_hShader;
  ezMaterialResourceHandle m_hMaterial;
  ezMeshBufferResourceHandle m_hQuadMeshBuffer;
};

EZ_CONSOLEAPP_ENTRY_POINT(TextureSample);
