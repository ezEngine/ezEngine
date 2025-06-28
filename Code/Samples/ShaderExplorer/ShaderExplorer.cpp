#include <ShaderExplorer/ShaderExplorer.h>

#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Core/Input/InputManager.h>
#include <Core/Input/VirtualThumbStick.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/System/Window.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Time/Clock.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

static ezUInt32 g_uiWindowWidth = 640;
static ezUInt32 g_uiWindowHeight = 480;
static bool g_bWindowResized = false;

class ezShaderExplorerWindow : public ezWindow
{
public:
  ezShaderExplorerWindow()
    : ezWindow()
  {
    m_bCloseRequested = false;
  }

  virtual void OnClickClose() override { m_bCloseRequested = true; }
  virtual ezSizeU32 GetClientAreaSize() const override
  {
    return m_CreationDescription.m_Resolution;
  }
  virtual void OnResize(const ezSizeU32& newWindowSize) override
  {
    ezWindow::OnResize(newWindowSize);
    if (g_uiWindowWidth != newWindowSize.width || g_uiWindowHeight != newWindowSize.height)
    {
      g_uiWindowWidth = newWindowSize.width;
      g_uiWindowHeight = newWindowSize.height;
      g_bWindowResized = true;
    }
  }

  bool m_bCloseRequested;
};

ezShaderExplorerApp::ezShaderExplorerApp()
  : ezApplication("Shader Explorer")
{
}

void ezShaderExplorerApp::Run()
{
  m_pWindow->ProcessWindowMessages();
  if (!m_pWindow->IsVisible())
  {
    ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(16));
    return;
  }

  if (g_bWindowResized)
  {
    g_bWindowResized = false;
    UpdateSwapChain();
  }

  if (m_pWindow->m_bCloseRequested || ezInputManager::GetInputActionState("Main", "CloseApp") == ezKeyState::Pressed)
  {
    RequestApplicationQuit();
    return;
  }

  // make sure time goes on
  ezClock::GetGlobalClock()->Update();

  // update all input state
  ezInputManager::Update(ezClock::GetGlobalClock()->GetTimeDiff());

  // mouse look
  if (ezInputManager::GetInputActionState("Main", "Look") == ezKeyState::Down)
  {
    m_pWindow->GetInputDevice()->SetShowMouseCursor(false);
    m_pWindow->GetInputDevice()->SetClipMouseCursor(ezMouseCursorClipMode::ClipToPosition);

    float fInputValue = 0.0f;
    const float fMouseSpeed = 0.01f;

    ezVec3 mouseMotion(0.0f);

    if (ezInputManager::GetInputActionState("Main", "LookPosX", &fInputValue) != ezKeyState::Up)
      mouseMotion.x += fInputValue * fMouseSpeed;
    if (ezInputManager::GetInputActionState("Main", "LookNegX", &fInputValue) != ezKeyState::Up)
      mouseMotion.x -= fInputValue * fMouseSpeed;
    if (ezInputManager::GetInputActionState("Main", "LookPosY", &fInputValue) != ezKeyState::Up)
      mouseMotion.y -= fInputValue * fMouseSpeed;
    if (ezInputManager::GetInputActionState("Main", "LookNegY", &fInputValue) != ezKeyState::Up)
      mouseMotion.y += fInputValue * fMouseSpeed;

    m_pCamera->RotateLocally(ezAngle::MakeFromRadian(0.0), ezAngle::MakeFromRadian(mouseMotion.y), ezAngle::MakeFromRadian(0.0));
    m_pCamera->RotateGlobally(ezAngle::MakeFromRadian(0.0), ezAngle::MakeFromRadian(mouseMotion.x), ezAngle::MakeFromRadian(0.0));
  }
  else
  {
    m_pWindow->GetInputDevice()->SetShowMouseCursor(true);
    m_pWindow->GetInputDevice()->SetClipMouseCursor(ezMouseCursorClipMode::NoClip);
  }

  // turn camera with keys
  {
    float fInputValue = 0.0f;
    const float fTurnSpeed = 1.0f;

    ezVec3 mouseMotion(0.0f);

    if (ezInputManager::GetInputActionState("Main", "TurnPosX", &fInputValue) != ezKeyState::Up)
      mouseMotion.x += fInputValue * fTurnSpeed;
    if (ezInputManager::GetInputActionState("Main", "TurnNegX", &fInputValue) != ezKeyState::Up)
      mouseMotion.x -= fInputValue * fTurnSpeed;
    if (ezInputManager::GetInputActionState("Main", "TurnPosY", &fInputValue) != ezKeyState::Up)
      mouseMotion.y += fInputValue * fTurnSpeed;
    if (ezInputManager::GetInputActionState("Main", "TurnNegY", &fInputValue) != ezKeyState::Up)
      mouseMotion.y -= fInputValue * fTurnSpeed;

    m_pCamera->RotateLocally(ezAngle::MakeFromRadian(0.0), ezAngle::MakeFromRadian(mouseMotion.y), ezAngle::MakeFromRadian(0.0));
    m_pCamera->RotateGlobally(ezAngle::MakeFromRadian(0.0), ezAngle::MakeFromRadian(mouseMotion.x), ezAngle::MakeFromRadian(0.0));
  }

  // movement
  {
    float fInputValue = 0.0f;
    ezVec3 cameraMotion(0.0f);

    if (ezInputManager::GetInputActionState("Main", "MovePosX", &fInputValue) != ezKeyState::Up)
      cameraMotion.x += fInputValue;
    if (ezInputManager::GetInputActionState("Main", "MoveNegX", &fInputValue) != ezKeyState::Up)
      cameraMotion.x -= fInputValue;
    if (ezInputManager::GetInputActionState("Main", "MovePosY", &fInputValue) != ezKeyState::Up)
      cameraMotion.y += fInputValue;
    if (ezInputManager::GetInputActionState("Main", "MoveNegY", &fInputValue) != ezKeyState::Up)
      cameraMotion.y -= fInputValue;

    m_pCamera->MoveLocally(cameraMotion.y, cameraMotion.x, 0.0f);
  }

#if EZ_ENABLED(USE_DIRECTORY_WATCHER)
  {
    m_bStuffChanged = false;
    m_pDirectoryWatcher->EnumerateChanges(ezMakeDelegate(&ezShaderExplorerApp::OnFileChanged, this));

    if (m_bStuffChanged)
    {
      ezResourceManager::ReloadAllResources(false);
    }
  }
#endif

  // do the rendering
  {
    // Before starting to render in a frame call this function
    m_pDevice->EnqueueFrameSwapChain(m_hSwapChain);
    m_pDevice->BeginFrame();

    ezGALCommandEncoder* pCommandEncoder = m_pDevice->BeginCommands("ezShaderExplorerMainPass");
    const ezGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapChain);
    ezGALRenderTargetViewHandle hBBRTV = m_pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetRenderTargets().m_hRTs[0]);
    ezGALRenderTargetViewHandle hBBDSV = m_pDevice->GetDefaultRenderTargetView(m_hDepthStencilTexture);

    ezGALRenderingSetup renderingSetup;
    renderingSetup.SetColorTarget(0, hBBRTV).SetDepthStencilTarget(hBBDSV);
    renderingSetup.SetClearColor(0).SetClearDepth().SetClearStencil();

    ezRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, ezRectFloat(0.0f, 0.0f, (float)g_uiWindowWidth, (float)g_uiWindowHeight));

    auto& gc = ezRenderContext::GetDefaultInstance()->WriteGlobalConstants();
    ezMemoryUtils::ZeroFill(&gc, 1);

    ezMat4 m0, m1;
    m0 = m_pCamera->GetViewMatrix(ezCameraEye::Left);
    m1 = m_pCamera->GetViewMatrix(ezCameraEye::Right);
    gc.WorldToCameraMatrix[0] = m0;
    gc.WorldToCameraMatrix[1] = m1;
    gc.CameraToWorldMatrix[0] = m0.GetInverse();
    gc.CameraToWorldMatrix[1] = m1.GetInverse();
    gc.ViewportSize = ezVec4((float)g_uiWindowWidth, (float)g_uiWindowHeight, 1.0f / (float)g_uiWindowWidth, 1.0f / (float)g_uiWindowHeight);
    // Wrap around to prevent floating point issues. Wrap around is dividable by all whole numbers up to 11.
    gc.GlobalTime = (float)ezMath::Mod(ezClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds(), 20790.0);
    gc.WorldTime = gc.GlobalTime;

    ezRenderContext::GetDefaultInstance()->BindMaterial(m_hMaterial);
    ezRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hQuadMeshBuffer);
    ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().IgnoreResult();

    ezRenderContext::GetDefaultInstance()->EndRendering();
    m_pDevice->EndCommands(pCommandEncoder);

    m_pDevice->EndFrame();
  }

  // needs to be called once per frame
  ezResourceManager::PerFrameUpdate();

  // tell the task system to finish its work for this frame
  // this has to be done at the very end, so that the task system will only use up the time that is left in this frame for
  // uploading GPU data etc.
  ezTaskSystem::FinishFrameTasks();

  // for plugins (like FileServe) that need to hook into the game update
  EZ_BROADCAST_EVENT(GameApp_UpdatePlugins);
}

void ezShaderExplorerApp::AfterCoreSystemsStartup()
{
#if EZ_ENABLED(USE_FILESERVE)
  ezPlugin::LoadPlugin("ezFileservePlugin").AssertSuccess("Failed to load FileServe plugin");
#endif

  m_pCamera = EZ_DEFAULT_NEW(ezCamera);
  m_pCamera->LookAt(ezVec3(3, 3, 1.5), ezVec3(0, 0, 0), ezVec3(0, 1, 0));

  ezStringBuilder sProjectDir = ">sdk/Data/Samples/ShaderExplorer";
  ezStringBuilder sProjectDirResolved;
  ezFileSystem::ResolveSpecialDirectory(sProjectDir, sProjectDirResolved).IgnoreResult();
  ezFileSystem::SetSpecialDirectory("project", sProjectDirResolved);

#if EZ_ENABLED(USE_DIRECTORY_WATCHER)
  m_pDirectoryWatcher = EZ_DEFAULT_NEW(ezDirectoryWatcher);
  m_pDirectoryWatcher->OpenDirectory(sProjectDirResolved, ezDirectoryWatcher::Watch::Writes | ezDirectoryWatcher::Watch::Subdirectories).AssertSuccess("Failed to watch project directory");
#endif

  ezFileSystem::AddDataDirectory(">sdk/Output/", "ShaderCache", "shadercache", ezDataDirUsage::AllowWrites).AssertSuccess();
  ezFileSystem::AddDataDirectory(">sdk/Data/Base", "Base", "base").AssertSuccess();
  ezFileSystem::AddDataDirectory(">project/", "Project", "project").AssertSuccess();

  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  ezTelemetry::CreateServer();
  ezPlugin::LoadPlugin("ezInspectorPlugin", ezPluginLoadFlags::PluginIsOptional).IgnoreResult();

#ifdef BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
  constexpr const char* szDefaultRenderer = "Vulkan";
#else
  constexpr const char* szDefaultRenderer = "DX11";
#endif

  ezStringView sRendererName = ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, szDefaultRenderer);
  const char* szShaderModel = "";
  const char* szShaderCompiler = "";
  ezGALDeviceFactory::GetShaderModelAndCompiler(sRendererName, szShaderModel, szShaderCompiler);

  ezShaderManager::Configure(szShaderModel, true);
  ezPlugin::LoadPlugin(szShaderCompiler).IgnoreResult();

  // Register Input
  {
    {
      m_pLeftStick = EZ_DEFAULT_NEW(ezVirtualThumbStick);
      m_pLeftStick->SetInputArea(ezVec2(0, 0), ezVec2(0.5, 1), 0.25f, 1.0f, ezVirtualThumbStick::CenterMode::ActivationPoint);
      m_pLeftStick->SetTriggerInputSlot(ezVirtualThumbStick::Input::Touchpoint);
      m_pLeftStick->SetThumbstickOutput(ezVirtualThumbStick::Output::Controller0_LeftStick);
      m_pLeftStick->SetAreaFocusMode(ezInputActionConfig::OnEnterArea::ActivateImmediately, ezInputActionConfig::OnLeaveArea::KeepFocus);
      m_pLeftStick->SetEnabled(true);
    }
    {
      m_pRightStick = EZ_DEFAULT_NEW(ezVirtualThumbStick);
      m_pRightStick->SetInputArea(ezVec2(0.5, 0), ezVec2(1, 1), 0.25f, 1.0f, ezVirtualThumbStick::CenterMode::ActivationPoint);
      m_pRightStick->SetTriggerInputSlot(ezVirtualThumbStick::Input::Touchpoint);
      m_pRightStick->SetThumbstickOutput(ezVirtualThumbStick::Output::Controller0_RightStick);
      m_pRightStick->SetAreaFocusMode(ezInputActionConfig::OnEnterArea::ActivateImmediately, ezInputActionConfig::OnLeaveArea::KeepFocus);
      m_pRightStick->SetEnabled(true);
    }


    ezInputActionConfig cfg;

    cfg = ezInputManager::GetInputActionConfig("Main", "CloseApp");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyEscape;
    ezInputManager::SetInputActionConfig("Main", "CloseApp", cfg, true);

    cfg = ezInputManager::GetInputActionConfig("Main", "LookPosX");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMovePosX;
    cfg.m_bApplyTimeScaling = true;
    ezInputManager::SetInputActionConfig("Main", "LookPosX", cfg, true);

    cfg = ezInputManager::GetInputActionConfig("Main", "LookNegX");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMoveNegX;
    cfg.m_bApplyTimeScaling = true;
    ezInputManager::SetInputActionConfig("Main", "LookNegX", cfg, true);

    cfg = ezInputManager::GetInputActionConfig("Main", "LookPosY");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMovePosY;
    cfg.m_bApplyTimeScaling = true;
    ezInputManager::SetInputActionConfig("Main", "LookPosY", cfg, true);

    cfg = ezInputManager::GetInputActionConfig("Main", "LookNegY");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMoveNegY;
    cfg.m_bApplyTimeScaling = true;
    ezInputManager::SetInputActionConfig("Main", "LookNegY", cfg, true);

    cfg = ezInputManager::GetInputActionConfig("Main", "TurnPosX");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyRight;
    cfg.m_sInputSlotTrigger[1] = ezInputSlot_Controller0_RightStick_PosX;
    cfg.m_bApplyTimeScaling = true;
    ezInputManager::SetInputActionConfig("Main", "TurnPosX", cfg, true);

    cfg = ezInputManager::GetInputActionConfig("Main", "TurnNegX");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyLeft;
    cfg.m_sInputSlotTrigger[1] = ezInputSlot_Controller0_RightStick_NegX;
    cfg.m_bApplyTimeScaling = true;
    ezInputManager::SetInputActionConfig("Main", "TurnNegX", cfg, true);

    cfg = ezInputManager::GetInputActionConfig("Main", "TurnPosY");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyDown;
    cfg.m_sInputSlotTrigger[1] = ezInputSlot_Controller0_RightStick_PosY;
    cfg.m_bApplyTimeScaling = true;
    ezInputManager::SetInputActionConfig("Main", "TurnPosY", cfg, true);

    cfg = ezInputManager::GetInputActionConfig("Main", "TurnNegY");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyUp;
    cfg.m_sInputSlotTrigger[1] = ezInputSlot_Controller0_RightStick_NegY;
    cfg.m_bApplyTimeScaling = true;
    ezInputManager::SetInputActionConfig("Main", "TurnNegY", cfg, true);

    cfg = ezInputManager::GetInputActionConfig("Main", "Look");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseButton0;
    cfg.m_bApplyTimeScaling = false;
    ezInputManager::SetInputActionConfig("Main", "Look", cfg, true);

    cfg = ezInputManager::GetInputActionConfig("Main", "MovePosX");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyD;
    cfg.m_sInputSlotTrigger[1] = ezInputSlot_Controller0_LeftStick_PosX;
    cfg.m_bApplyTimeScaling = true;
    ezInputManager::SetInputActionConfig("Main", "MovePosX", cfg, true);

    cfg = ezInputManager::GetInputActionConfig("Main", "MoveNegX");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyA;
    cfg.m_sInputSlotTrigger[1] = ezInputSlot_Controller0_LeftStick_NegX;
    cfg.m_bApplyTimeScaling = true;
    ezInputManager::SetInputActionConfig("Main", "MoveNegX", cfg, true);

    cfg = ezInputManager::GetInputActionConfig("Main", "MovePosY");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyW;
    cfg.m_sInputSlotTrigger[1] = ezInputSlot_Controller0_LeftStick_PosY;
    cfg.m_bApplyTimeScaling = true;
    ezInputManager::SetInputActionConfig("Main", "MovePosY", cfg, true);

    cfg = ezInputManager::GetInputActionConfig("Main", "MoveNegY");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyS;
    cfg.m_sInputSlotTrigger[1] = ezInputSlot_Controller0_LeftStick_NegY;
    cfg.m_bApplyTimeScaling = true;
    ezInputManager::SetInputActionConfig("Main", "MoveNegY", cfg, true);
  }

  // Create a window for rendering
  {
    ezWindowCreationDesc WindowCreationDesc;
    WindowCreationDesc.m_Resolution.width = g_uiWindowWidth;
    WindowCreationDesc.m_Resolution.height = g_uiWindowHeight;
    WindowCreationDesc.m_Title = "Shader Explorer";
    WindowCreationDesc.m_bShowMouseCursor = true;
    WindowCreationDesc.m_bClipMouseCursor = false;
    WindowCreationDesc.m_WindowMode = ezWindowMode::WindowResizable;
    m_pWindow = EZ_DEFAULT_NEW(ezShaderExplorerWindow);
    m_pWindow->Initialize(WindowCreationDesc).IgnoreResult();
    g_uiWindowWidth = m_pWindow->GetClientAreaSize().width;
    g_uiWindowHeight = m_pWindow->GetClientAreaSize().height;
  }

  // Create a device
  {
    ezGALDeviceCreationDescription DeviceInit;
    DeviceInit.m_bDebugDevice = true;

    m_pDevice = ezGALDeviceFactory::CreateDevice(sRendererName, ezFoundation::GetDefaultAllocator(), DeviceInit);
    EZ_ASSERT_DEV(m_pDevice != nullptr, "Device implemention for '{}' not found", sRendererName);
    EZ_VERIFY(m_pDevice->Init() == EZ_SUCCESS, "Device init failed!");

    ezGALDevice::SetDefaultDevice(m_pDevice);
  }

  // now that we have a window and device, tell the engine to initialize the rendering infrastructure
  ezStartup::StartupHighLevelSystems();

  UpdateSwapChain();

  // Setup Shaders and Materials
  {
    m_hMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Materials/screen.ezMaterial");

    // Create the mesh that we use for rendering
    CreateScreenQuad();
  }
}

void ezShaderExplorerApp::BeforeHighLevelSystemsShutdown()
{
  ezTelemetry::CloseConnection();
  m_pLeftStick.Clear();
  m_pRightStick.Clear();
#if EZ_ENABLED(USE_DIRECTORY_WATCHER)
  m_pDirectoryWatcher->CloseDirectory();
  m_pDirectoryWatcher.Clear();
#endif
  m_pDevice->DestroyTexture(m_hDepthStencilTexture);
  m_hDepthStencilTexture.Invalidate();

  m_hMaterial.Invalidate();
  m_hQuadMeshBuffer.Invalidate();
  m_pDevice->DestroySwapChain(m_hSwapChain);

  // tell the engine that we are about to destroy window and graphics device,
  // and that it therefore needs to cleanup anything that depends on that
  ezStartup::ShutdownHighLevelSystems();

  // now we can destroy the graphics device
  m_pDevice->Shutdown().IgnoreResult();

  EZ_DEFAULT_DELETE(m_pDevice);

  // finally destroy the window
  m_pWindow->DestroyWindow();
  EZ_DEFAULT_DELETE(m_pWindow);

  m_pCamera.Clear();
}

void ezShaderExplorerApp::UpdateSwapChain()
{
  // Create a Swapchain
  if (m_hSwapChain.IsInvalidated())
  {
    ezGALWindowSwapChainCreationDescription swapChainDesc;
    swapChainDesc.m_pWindow = m_pWindow;
    swapChainDesc.m_SampleCount = ezGALMSAASampleCount::None;
    swapChainDesc.m_InitialPresentMode = ezGALPresentMode::VSync;
    m_hSwapChain = ezGALWindowSwapChain::Create(swapChainDesc);
  }
  else
  {
    m_pDevice->UpdateSwapChain(m_hSwapChain, ezGALPresentMode::VSync).IgnoreResult();
  }

  if (!m_hSwapChain.IsInvalidated())
  {
    m_pDevice->DestroyTexture(m_hDepthStencilTexture);
    m_hDepthStencilTexture.Invalidate();
  }
  // Create depth texture
  {
    ezGALTextureCreationDescription texDesc;
    texDesc.m_uiWidth = g_uiWindowWidth;
    texDesc.m_uiHeight = g_uiWindowHeight;
    texDesc.m_Format = ezGALResourceFormat::D24S8;
    texDesc.m_bAllowRenderTargetView = true;

    m_hDepthStencilTexture = m_pDevice->CreateTexture(texDesc);
  }
}

void ezShaderExplorerApp::CreateScreenQuad()
{
  ezGeometry geom;
  ezGeometry::GeoOptions opt;
  opt.m_Color = ezColor::Black;
  geom.AddRect(ezVec2(2, 2), 1, 1, opt);

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
    m_hQuadMeshBuffer = ezResourceManager::GetOrCreateResource<ezMeshBufferResource>("{E692442B-9E15-46C5-8A00-1B07C02BF8F7}", std::move(desc));
}

#if EZ_ENABLED(USE_DIRECTORY_WATCHER)
void ezShaderExplorerApp::OnFileChanged(ezStringView sFilename, ezDirectoryWatcherAction action, ezDirectoryWatcherType type)
{
  if (action == ezDirectoryWatcherAction::Modified && type == ezDirectoryWatcherType::File)
  {
    ezLog::Info("The file {0} was modified", sFilename);
    m_bStuffChanged = true;
  }
}
#endif

EZ_APPLICATION_ENTRY_POINT(ezShaderExplorerApp);
