#include <RasterizerExplorer/RasterizerExplorer.h>

#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Core/Input/DeviceTypes/MouseKeyboard.h>
#include <Core/Input/InputManager.h>
#include <Core/System/Window.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Clock.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>
#include <RendererCore/Rasterizer/RasterizerView.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

static bool g_bWindowResized = false;

ezRasterizerExplorerApp::ezRasterizerExplorerApp()
  : ezApplication("Rasterizer Explorer")
{
}

void ezRasterizerExplorerApp::PrintTestCode()
{
  const ezVec3 vPos = m_pCamera->GetCenterPosition();
  const ezVec3 vDir = m_pCamera->GetCenterDirForwards();
  const ezVec3 vTarget = vPos + vDir;

  ezLog::Info("");
  ezLog::Info("// --- Paste this into RasterizerTest.cpp ---");
  ezLog::Info("EZ_TEST_BLOCK(ezTestBlock::Enabled, \"Custom orientation\")");
  ezLog::Info("{{");
  ezLog::Info("  constexpr ezUInt32 uiSize = 128;");
  ezLog::Info("  ezRasterizerView view;");
  ezLog::Info("  view.SetResolution(uiSize, uiSize, 0.0f);");
  ezLog::Info("  ezCamera camera;");
  ezLog::Info("  camera.LookAt(ezVec3({:.4f}f, {:.4f}f, {:.4f}f), ezVec3({:.4f}f, {:.4f}f, {:.4f}f), ezVec3(0, 1, 0));",
    vPos.x, vPos.y, vPos.z, vTarget.x, vTarget.y, vTarget.z);
  ezLog::Info("  camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);");
  ezLog::Info("  view.SetCamera(&camera);");
  ezLog::Info("  auto pBox = ezRasterizerObject::CreateBox(ezVec3(4, 4, 4));");
  ezLog::Info("  view.BeginScene();");
  ezLog::Info("  view.AddObject(pBox.Borrow(), ezTransform::MakeIdentity());");
  ezLog::Info("  view.EndScene();");
  ezLog::Info("  ezRasterizerTestGroup::SetImage(view);");
  ezLog::Info("  EZ_TEST_LINE_IMAGE(99, 200);");
  ezLog::Info("}}");
  ezLog::Info("// --- End of test code ---");
  ezLog::Info("");
}

void ezRasterizerExplorerApp::RenderRasterizer()
{
  m_pRasterizerView->SetResolution(RasterizerSize, RasterizerSize, 0.0f);
  m_pRasterizerView->SetCamera(m_pCamera.Borrow());

  auto pBox = ezRasterizerObject::CreateBox(ezVec3(4, 4, 4));

  m_pRasterizerView->BeginScene();
  m_pRasterizerView->AddObject(pBox.Borrow(), ezTransform::MakeIdentity());
  m_pRasterizerView->EndScene();
}

void ezRasterizerExplorerApp::Run()
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

  if (ezInputManager::GetInputActionState("Main", "CloseApp") == ezKeyState::Pressed)
  {
    QuitApplication();
    return;
  }

  if (ezInputManager::GetInputActionState("Main", "PrintTest") == ezKeyState::Pressed)
  {
    PrintTestCode();
  }

  ezClock::GetGlobalClock()->Update();
  ezInputManager::Update(ezClock::GetGlobalClock()->GetTimeDiff());

  // Mouse look (right click held)
  if (ezInputManager::GetInputActionState("Main", "Look") == ezKeyState::Down)
  {
    if (auto pInput = ezDynamicCast<ezInputDeviceMouseKeyboard*>(m_pWindow->GetInputDevice()))
    {
      pInput->SetShowMouseCursor(false);
      pInput->SetClipMouseCursor(ezMouseCursorClipMode::ClipToPosition);
    }

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
    if (auto pInput = ezDynamicCast<ezInputDeviceMouseKeyboard*>(m_pWindow->GetInputDevice()))
    {
      pInput->SetShowMouseCursor(true);
      pInput->SetClipMouseCursor(ezMouseCursorClipMode::NoClip);
    }
  }

  // WASD movement
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

    if (ezInputManager::GetInputActionState("Main", "Sprint") != ezKeyState::Up)
      cameraMotion *= 10.0f;

    m_pCamera->MoveLocally(cameraMotion.y, cameraMotion.x, 0.0f);
  }

  // Run the software rasterizer
  RenderRasterizer();

  // Read back the rasterizer output
  ezDynamicArray<ezColorLinearUB> rasterizerBuffer;
  rasterizerBuffer.SetCount(RasterizerSize * RasterizerSize);
  m_pRasterizerView->ReadBackFrame(rasterizerBuffer);

  // Upload to GPU texture and render
  {
    m_pDevice->EnqueueFrameSwapChain(m_hSwapChain);
    m_pDevice->BeginFrame();

    // Upload rasterizer output to texture
    {
      ezGALSystemMemoryDescription sourceData;
      sourceData.m_pData = ezMakeArrayPtr(reinterpret_cast<ezUInt8*>(rasterizerBuffer.GetData()),
                                           rasterizerBuffer.GetCount() * sizeof(ezColorLinearUB));
      sourceData.m_uiRowPitch = RasterizerSize * sizeof(ezColorLinearUB);
      m_pDevice->UpdateTextureForNextFrame(m_hRasterizerTexture, sourceData);
    }

    // Clear and blit
    ezGALCommandEncoder* pCommandEncoder = m_pDevice->BeginCommands("RasterizerBlit");

    const ezGALSwapChain* pSwapChain = m_pDevice->GetSwapChain(m_hSwapChain);
    ezGALTextureHandle hBackBuffer = pSwapChain->GetRenderTargets().m_hRTs[0];
    ezGALRenderTargetViewHandle hBBRTV = m_pDevice->GetDefaultRenderTargetView(hBackBuffer);
    ezGALRenderTargetViewHandle hBBDSV = m_pDevice->GetDefaultRenderTargetView(m_hDepthStencilTexture);

    // Clear to dark gray
    ezGALRenderingSetup renderingSetup;
    renderingSetup.SetColorTarget(0, hBBRTV).SetDepthStencilTarget(hBBDSV);
    renderingSetup.SetClearColor(0, ezColor(0.2f, 0.2f, 0.2f)).SetClearDepth().SetClearStencil();

    pCommandEncoder->BeginRendering(renderingSetup);

    // Copy texture to backbuffer
    pCommandEncoder->EndRendering();

    // Use CopyTextureRegion to blit the rasterizer texture to the center of the backbuffer
    {
      ezBoundingBoxu32 srcBox;
      srcBox.m_vMin = ezVec3U32(0, 0, 0);
      srcBox.m_vMax = ezVec3U32(RasterizerSize, RasterizerSize, 1);

      const ezUInt32 uiWinW = m_pWindow->GetClientAreaSize().width;
      const ezUInt32 uiWinH = m_pWindow->GetClientAreaSize().height;
      const ezUInt32 uiOffX = (uiWinW > RasterizerSize) ? (uiWinW - RasterizerSize) / 2 : 0;
      const ezUInt32 uiOffY = (uiWinH > RasterizerSize) ? (uiWinH - RasterizerSize) / 2 : 0;

      pCommandEncoder->CopyTextureRegion(hBackBuffer, ezGALTextureSubresource(),
        ezVec3U32(uiOffX, uiOffY, 0), m_hRasterizerTexture, ezGALTextureSubresource(), srcBox);
    }

    m_pDevice->EndCommands(pCommandEncoder);
    m_pDevice->EndFrame();
  }

  ezTaskSystem::FinishFrameTasks();
}

void ezRasterizerExplorerApp::AfterCoreSystemsStartup()
{
  m_pCamera = EZ_DEFAULT_NEW(ezCamera);
  m_pCamera->LookAt(ezVec3(0, 0, -8), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
  m_pCamera->SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);

  m_pRasterizerView = EZ_DEFAULT_NEW(ezRasterizerView);

  ezFileSystem::AddDataDirectory(">sdk/Data/Base", "Base", "base").AssertSuccess();

  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

#ifdef BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
  constexpr const char* szDefaultRenderer = "Vulkan";
#else
  constexpr const char* szDefaultRenderer = "DX11";
#endif

  ezStringView sRendererName = ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, szDefaultRenderer);
  const char* szShaderModel = "";
  const char* szShaderCompiler = "";
  ezGALDeviceFactory::GetShaderModelAndCompiler(sRendererName, szShaderModel, szShaderCompiler);

  // Input configuration
  {
    ezInputActionConfig cfg;

    cfg = ezInputManager::GetInputActionConfig("Main", "CloseApp");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyEscape;
    ezInputManager::SetInputActionConfig("Main", "CloseApp", cfg, true);

    cfg = ezInputManager::GetInputActionConfig("Main", "PrintTest");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyP;
    ezInputManager::SetInputActionConfig("Main", "PrintTest", cfg, true);

    cfg = ezInputManager::GetInputActionConfig("Main", "Sprint");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyLeftShift;
    cfg.m_sInputSlotTrigger[1] = ezInputSlot_KeyRightShift;
    cfg.m_bApplyTimeScaling = false;
    ezInputManager::SetInputActionConfig("Main", "Sprint", cfg, true);

    cfg = ezInputManager::GetInputActionConfig("Main", "Look");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseButton0;
    cfg.m_bApplyTimeScaling = false;
    ezInputManager::SetInputActionConfig("Main", "Look", cfg, true);

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

  // Window
  {
    ezWindowCreationDesc desc;
    desc.m_Resolution.width = 512;
    desc.m_Resolution.height = 512;
    desc.m_Title = "Rasterizer Explorer (P = print test code, LMB+mouse = look, WASD = move, ESC = quit)";
    desc.m_bShowMouseCursor = true;
    desc.m_bClipMouseCursor = false;
    desc.m_WindowMode = ezWindowMode::WindowResizable;
    m_pWindow = EZ_DEFAULT_NEW(ezWindow);
    m_pWindow->Initialize(desc).IgnoreResult();

    m_pWindow->WindowEvents().AddEventHandler([this](const ezWindowEvent& e)
      {
        if (e.m_Type == ezWindowEvent::Type::CloseButtonClicked)
          this->QuitApplication();
        if (e.m_Type == ezWindowEvent::Type::SizeChanged)
          g_bWindowResized = true;
      });
  }

  // GPU device
  {
    ezGALDeviceCreationDescription DeviceInit;
    DeviceInit.m_bDebugDevice = false;

    m_pDevice = ezGALDeviceFactory::CreateDevice(sRendererName, ezFoundation::GetDefaultAllocator(), DeviceInit);
    EZ_ASSERT_DEV(m_pDevice != nullptr, "Device implementation for '{}' not found", sRendererName);
    EZ_VERIFY(m_pDevice->Init() == EZ_SUCCESS, "Device init failed!");
    ezGALDevice::SetDefaultDevice(m_pDevice);
  }

  ezStartup::StartupHighLevelSystems();

  UpdateSwapChain();

  // Create the rasterizer output texture
  {
    ezGALTextureCreationDescription texDesc;
    texDesc.m_uiWidth = RasterizerSize;
    texDesc.m_uiHeight = RasterizerSize;
    texDesc.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
    texDesc.m_ResourceAccess.m_bImmutable = false;
    m_hRasterizerTexture = m_pDevice->CreateTexture(texDesc);
  }

  ezLog::Info("Rasterizer Explorer started.");
  ezLog::Info("  LMB + mouse: look around");
  ezLog::Info("  WASD: move camera");
  ezLog::Info("  P: print unit test code for current camera");
  ezLog::Info("  ESC: quit");
}

void ezRasterizerExplorerApp::BeforeHighLevelSystemsShutdown()
{
  m_pRasterizerView.Clear();

  m_pDevice->DestroyTexture(m_hRasterizerTexture);
  m_hRasterizerTexture.Invalidate();

  m_pDevice->DestroyTexture(m_hDepthStencilTexture);
  m_hDepthStencilTexture.Invalidate();

  m_pDevice->DestroySwapChain(m_hSwapChain);

  ezStartup::ShutdownHighLevelSystems();
  m_pDevice->Shutdown().IgnoreResult();
  EZ_DEFAULT_DELETE(m_pDevice);

  m_pWindow->DestroyWindow();
  EZ_DEFAULT_DELETE(m_pWindow);

  m_pCamera.Clear();
}

void ezRasterizerExplorerApp::UpdateSwapChain()
{
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

  ezGALTextureCreationDescription texDesc;
  texDesc.m_uiWidth = m_pWindow->GetClientAreaSize().width;
  texDesc.m_uiHeight = m_pWindow->GetClientAreaSize().height;
  texDesc.m_Format = ezGALResourceFormat::D24S8;
  texDesc.m_TextureFlags.Add(ezGALTextureUsageFlags::RenderTarget);
  m_hDepthStencilTexture = m_pDevice->CreateTexture(texDesc);
}

EZ_APPLICATION_ENTRY_POINT(ezRasterizerExplorerApp);
