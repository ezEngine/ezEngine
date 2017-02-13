#include "main.h"
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Core/Input/InputManager.h>
#include <System/Window/Window.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <Foundation/Configuration/Startup.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <Core/Graphics/Geometry.h>
#include <Foundation/Time/Clock.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <Core/Graphics/Camera.h>

static ezUInt32 g_uiWindowWidth = 640;
static ezUInt32 g_uiWindowHeight = 480;

class ezShaderExplorerWindow : public ezWindow
{
public:

  ezShaderExplorerWindow()
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

ezShaderExplorerApp::ezShaderExplorerApp()
  : ezApplication()
  , m_pWindow(nullptr)
  , m_pDevice(nullptr)
{
}

ezShaderExplorerApp::~ezShaderExplorerApp()
{

}

ezApplication::ApplicationExecution ezShaderExplorerApp::Run()
{
    m_pWindow->ProcessWindowMessages();

    if (m_pWindow->m_bCloseRequested || ezInputManager::GetInputActionState("Main", "CloseApp") == ezKeyState::Pressed)
      return ApplicationExecution::Quit;

    // make sure time goes on
    ezClock::GetGlobalClock()->Update();

    if (ezInputManager::GetInputActionState("Main", "Look") == ezKeyState::Down)
    {
      float fInputValue = 0.0f;
      const float fMouseSpeed = 0.01f;

      ezVec3 mouseMotion(0.0f);

      if (ezInputManager::GetInputActionState("Main", "LookPosX", &fInputValue) != ezKeyState::Up)
        mouseMotion.x += fInputValue * fMouseSpeed;
      if (ezInputManager::GetInputActionState("Main", "LookNegX", &fInputValue) != ezKeyState::Up)
        mouseMotion.x -= fInputValue * fMouseSpeed;
      if (ezInputManager::GetInputActionState("Main", "LookPosY", &fInputValue) != ezKeyState::Up)
        mouseMotion.y += fInputValue * fMouseSpeed;
      if (ezInputManager::GetInputActionState("Main", "LookNegY", &fInputValue) != ezKeyState::Up)
        mouseMotion.y -= fInputValue * fMouseSpeed;

      m_camera->RotateLocally(ezAngle::Radian(0.0), ezAngle::Radian(mouseMotion.y), ezAngle::Radian(0.0) );
      m_camera->RotateGlobally(ezAngle::Radian(0.0), ezAngle::Radian(mouseMotion.x), ezAngle::Radian(0.0));

      ezVec3 cameraMotion(0.0f);

      if (ezInputManager::GetInputActionState("Main", "MovePosX", &fInputValue) != ezKeyState::Up)
        cameraMotion.x += fInputValue;
      if (ezInputManager::GetInputActionState("Main", "MoveNegX", &fInputValue) != ezKeyState::Up)
        cameraMotion.x -= fInputValue;
      if (ezInputManager::GetInputActionState("Main", "MovePosY", &fInputValue) != ezKeyState::Up)
        cameraMotion.y += fInputValue;
      if (ezInputManager::GetInputActionState("Main", "MoveNegY", &fInputValue) != ezKeyState::Up)
        cameraMotion.y -= fInputValue;

      m_camera->MoveLocally(cameraMotion.y, cameraMotion.x, 0.0f);
    }

    // update all input state
    ezInputManager::Update(ezClock::GetGlobalClock()->GetTimeDiff());

    m_stuffChanged = false;
    m_directoryWatcher->EnumerateChanges(ezMakeDelegate(&ezShaderExplorerApp::OnFileChanged, this));
    if (m_stuffChanged)
    {
      ezResourceManager::ReloadAllResources(false);
    }

    // do the rendering
    {
      // Before starting to render in a frame call this function
      m_pDevice->BeginFrame();

      // The ezGALContext class is the main interaction point for draw / compute operations
      ezGALContext* pContext = m_pDevice->GetPrimaryContext();

      auto& gc = ezRenderContext::GetDefaultInstance()->WriteGlobalConstants();
      ezMemoryUtils::ZeroFill(&gc);

      m_camera->GetViewMatrix(gc.WorldToCameraMatrix);
      gc.CameraToWorldMatrix = gc.WorldToCameraMatrix.GetInverse();
      gc.Viewport = ezVec4(0, 0, (float)g_uiWindowWidth, (float)g_uiWindowHeight);
      // Wrap around to prevent floating point issues. Wrap around is dividable by all whole numbers up to 11.
      gc.GlobalTime = (float)ezMath::Mod(ezClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds(), 20790.0);
      gc.WorldTime = gc.GlobalTime;


      ezGALRenderTagetSetup RTS;
      RTS.SetRenderTarget(0, m_hBBRTV)
        .SetDepthStencilTarget(m_hBBDSV);

      pContext->SetRenderTargetSetup(RTS);
      pContext->SetViewport(ezRectFloat(0.0f, 0.0f, (float)g_uiWindowWidth, (float)g_uiWindowHeight), 0.0f, 1.0f);
      pContext->Clear(ezColor::Black);

      ezRenderContext::GetDefaultInstance()->BindMaterial(m_hMaterial);
      ezRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hQuadMeshBuffer);
      ezRenderContext::GetDefaultInstance()->DrawMeshBuffer();

      m_pDevice->Present(m_pDevice->GetPrimarySwapChain());

      m_pDevice->EndFrame();
      ezRenderContext::GetDefaultInstance()->ResetContextState();
    }

    // needs to be called once per frame
    ezResourceManager::PerFrameUpdate();

    // tell the task system to finish its work for this frame
    // this has to be done at the very end, so that the task system will only use up the time that is left in this frame for
    // uploading GPU data etc.
    ezTaskSystem::FinishFrameTasks();

    return ezApplication::Continue;
}

void ezShaderExplorerApp::AfterCoreStartup()
{
  m_camera = EZ_DEFAULT_NEW(ezCamera);
  m_camera->LookAt(ezVec3(3, 3, 1.5), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
  m_directoryWatcher = EZ_DEFAULT_NEW(ezDirectoryWatcher);

  ezStringBuilder sProjectDir = BUILDSYSTEM_OUTPUT_FOLDER;
  sProjectDir.AppendPath("../../Data/Samples/ShaderExplorer");

  ezFileSystem::SetSpecialDirectory("project", sProjectDir);

  EZ_VERIFY(m_directoryWatcher->OpenDirectory(sProjectDir, ezDirectoryWatcher::Watch::Writes | ezDirectoryWatcher::Watch::Subdirectories).Succeeded(), "Failed to watch project directory");

  // setup the 'asset management system'
  {
    // which redirection table to search
    ezDataDirectory::FolderType::s_sRedirectionFile = "AssetCache/LookupTable.ezAsset";
    // which platform assets to use
    ezDataDirectory::FolderType::s_sRedirectionPrefix = "AssetCache/PC/";
  }

  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

  ezFileSystem::AddDataDirectory("", "", ":", ezFileSystem::AllowWrites);
  ezFileSystem::AddDataDirectory(">appdir/", "AppBin", "bin", ezFileSystem::AllowWrites); // writing to the binary directory
  ezFileSystem::AddDataDirectory(">appdir/", "ShaderCache", "shadercache", ezFileSystem::AllowWrites); // for shader files
  ezFileSystem::AddDataDirectory(">user/ezEngine Project/ShaderExplorer", "AppData", "appdata", ezFileSystem::AllowWrites); // app user data

  ezFileSystem::AddDataDirectory(">sdk/Data/Base", "Base", "base");
  ezFileSystem::AddDataDirectory(">project/", "Project", "project", ezFileSystem::AllowWrites);

  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  ezPlugin::LoadPlugin("ezInspectorPlugin");

  EZ_VERIFY(ezPlugin::LoadPlugin("ezShaderCompilerHLSL").Succeeded(), "Compiler Plugin not found");

  // Register Input
  {
    ezInputActionConfig cfg;

    cfg = ezInputManager::GetInputActionConfig("Main", "CloseApp");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyEscape;
    ezInputManager::SetInputActionConfig("Main", "CloseApp", cfg, true);

    cfg = ezInputManager::GetInputActionConfig("Main", "LookPosX");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMovePosX;
    cfg.m_bApplyTimeScaling = false;
    ezInputManager::SetInputActionConfig("Main", "LookPosX", cfg, true);

    cfg = ezInputManager::GetInputActionConfig("Main", "LookNegX");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMoveNegX;
    cfg.m_bApplyTimeScaling = false;
    ezInputManager::SetInputActionConfig("Main", "LookNegX", cfg, true);

    cfg = ezInputManager::GetInputActionConfig("Main", "LookPosY");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMovePosY;
    cfg.m_bApplyTimeScaling = false;
    ezInputManager::SetInputActionConfig("Main", "LookPosY", cfg, true);

    cfg = ezInputManager::GetInputActionConfig("Main", "LookNegY");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMoveNegY;
    cfg.m_bApplyTimeScaling = false;
    ezInputManager::SetInputActionConfig("Main", "LookNegY", cfg, true);

    cfg = ezInputManager::GetInputActionConfig("Main", "Look");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseButton0;
    cfg.m_bApplyTimeScaling = false;
    ezInputManager::SetInputActionConfig("Main", "Look", cfg, true);

    cfg = ezInputManager::GetInputActionConfig("Main", "MovePosX");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyD;
    cfg.m_bApplyTimeScaling = true;
    ezInputManager::SetInputActionConfig("Main", "MovePosX", cfg, true);

    cfg = ezInputManager::GetInputActionConfig("Main", "MoveNegX");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyA;
    cfg.m_bApplyTimeScaling = true;
    ezInputManager::SetInputActionConfig("Main", "MoveNegX", cfg, true);

    cfg = ezInputManager::GetInputActionConfig("Main", "MovePosY");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyW;
    cfg.m_bApplyTimeScaling = true;
    ezInputManager::SetInputActionConfig("Main", "MovePosY", cfg, true);

    cfg = ezInputManager::GetInputActionConfig("Main", "MoveNegY");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyS;
    cfg.m_bApplyTimeScaling = true;
    ezInputManager::SetInputActionConfig("Main", "MoveNegY", cfg, true);
  }

  // Create a window for rendering
  {
    ezWindowCreationDesc WindowCreationDesc;
    WindowCreationDesc.m_Resolution.width = g_uiWindowWidth;
    WindowCreationDesc.m_Resolution.height = g_uiWindowHeight;
    m_pWindow = EZ_DEFAULT_NEW(ezShaderExplorerWindow);
    m_pWindow->Initialize(WindowCreationDesc);
  }

  // Create a device
  {
    ezGALDeviceCreationDescription DeviceInit;
    DeviceInit.m_bCreatePrimarySwapChain = true;
    DeviceInit.m_bDebugDevice = false; // On Windows 10 this makes device creation fail :-(
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

  // Setup Shaders and Materials
  {
    ezShaderManager::Configure("DX11_SM40", true);

    m_hMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Materials/screen.ezMaterial");

    // Create the mesh that we use for rendering
    CreateScreenQuad();
  }
}

void ezShaderExplorerApp::BeforeCoreShutdown()
{
  m_directoryWatcher->CloseDirectory();

  m_pDevice->DestroyTexture(m_hDepthStencilTexture);
  m_hDepthStencilTexture.Invalidate();

  m_hMaterial.Invalidate();
  m_hQuadMeshBuffer.Invalidate();

  // tell the engine that we are about to destroy window and graphics device,
  // and that it therefore needs to cleanup anything that depends on that
  ezStartup::ShutdownEngine();

  // now we can destroy the graphics device
  m_pDevice->Shutdown();

  EZ_DEFAULT_DELETE(m_pDevice);

  // finally destroy the window
  m_pWindow->Destroy();
  EZ_DEFAULT_DELETE(m_pWindow);

  m_camera.Reset();
  m_directoryWatcher.Reset();
}

void ezShaderExplorerApp::CreateScreenQuad()
{
    ezGeometry geom;
    geom.AddRectXY(ezVec2(2, 2), ezColor::Black);

    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);

    desc.AllocateStreams(geom.GetVertices().GetCount(), ezGALPrimitiveTopology::Triangles, geom.GetPolygons().GetCount() * 2);

    for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
    {
      desc.SetVertexData<ezVec3>(0, v, geom.GetVertices()[v].m_vPosition);
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

void ezShaderExplorerApp::OnFileChanged(const char* filename, ezDirectoryWatcherAction action)
{
  if (action == ezDirectoryWatcherAction::Modified)
  {
    ezLog::Info("The file {0} was modified", filename);
    m_stuffChanged = true;
  }
}

EZ_CONSOLEAPP_ENTRY_POINT(ezShaderExplorerApp);
