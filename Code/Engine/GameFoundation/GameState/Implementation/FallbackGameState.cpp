#include <GameFoundation/PCH.h>
#include <GameFoundation/GameState/FallbackGameState.h>
#include <GameFoundation/GameApplication/GameApplication.h>
#include <Core/Input/InputManager.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/SimpleRenderPass.h>
#include <RendererCore/Pipeline/TargetPass.h>
#include <RendererCore/Pipeline/Extractor.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFallbackGameState, 1, ezRTTIDefaultAllocator<ezFallbackGameState>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

class ezFallbackGameStateWindow : public ezWindow
{
public:
  ezFallbackGameStateWindow()
  {
    m_CreationDescription.m_ClientAreaSize.width = 720 * 16 / 9;
    m_CreationDescription.m_ClientAreaSize.height = 720;
    m_CreationDescription.m_Title = "SampleApp";
    m_CreationDescription.m_bFullscreenWindow = false;
    m_CreationDescription.m_bResizable = false;
    Initialize();
  }

  ~ezFallbackGameStateWindow()
  {
    Destroy();
  }

private:
  void OnResizeMessage(const ezSizeU32& newWindowSize) override
  {
    ezLog::Info("Resolution changed to %i * %i", newWindowSize.width, newWindowSize.height);

    m_CreationDescription.m_ClientAreaSize = newWindowSize;
  }
};



ezFallbackGameState::ezFallbackGameState() 
{
  m_pWindow = nullptr;
  m_pRenderPipeline = nullptr;
  m_pWorld = nullptr;
  m_pView = nullptr;
}

ezFallbackGameState::~ezFallbackGameState()
{

}

void ezFallbackGameState::Activate()
{
  EZ_LOG_BLOCK("ezFallbackGameState::Activate");

  m_pWindow = EZ_DEFAULT_NEW(ezFallbackGameStateWindow);

  ezGALSwapChainHandle hSwapChain = GetApplication()->AddWindow(m_pWindow);

  SetupInput();

  const ezGALSwapChain* pSwapChain = ezGALDevice::GetDefaultDevice()->GetSwapChain(hSwapChain);
  CreateRenderPipeline(pSwapChain->GetBackBufferRenderTargetView(), pSwapChain->GetDepthStencilTargetView());

}

void ezFallbackGameState::Deactivate()
{
  EZ_LOG_BLOCK("ezFallbackGameState::Deactivate");

  EZ_DEFAULT_DELETE(m_pWindow);
}


ezGameStateCanHandleThis ezFallbackGameState::CanHandleThis(ezGameApplicationType AppType, ezWorld* pWorld) const
{
  if (pWorld == nullptr)
    return ezGameStateCanHandleThis::No;

  /// \todo Pass this to Activate
  m_pWorld = pWorld;

  return ezGameStateCanHandleThis::AsFallback;
}

static void RegisterInputAction(const char* szInputSet, const char* szInputAction, const char* szKey1, const char* szKey2 = nullptr, const char* szKey3 = nullptr)
{
  ezInputActionConfig cfg;

  cfg = ezInputManager::GetInputActionConfig(szInputSet, szInputAction);
  cfg.m_bApplyTimeScaling = true;

  if (szKey1 != nullptr)     cfg.m_sInputSlotTrigger[0] = szKey1;
  if (szKey2 != nullptr)     cfg.m_sInputSlotTrigger[1] = szKey2;
  if (szKey3 != nullptr)     cfg.m_sInputSlotTrigger[2] = szKey3;

  ezInputManager::SetInputActionConfig(szInputSet, szInputAction, cfg, true);
}

void ezFallbackGameState::SetupInput()
{
  m_pWindow->GetInputDevice()->SetClipMouseCursor(true);
  m_pWindow->GetInputDevice()->SetShowMouseCursor(false);
  m_pWindow->GetInputDevice()->SetMouseSpeed(ezVec2(0.002f));

  RegisterInputAction("Main", "CloseApp", ezInputSlot_KeyEscape);
  RegisterInputAction("Main", "ReloadResources", ezInputSlot_KeyR);

  RegisterInputAction("Game", "MoveForwards", ezInputSlot_KeyW);
  RegisterInputAction("Game", "MoveBackwards", ezInputSlot_KeyS);
  RegisterInputAction("Game", "MoveLeft", ezInputSlot_KeyA);
  RegisterInputAction("Game", "MoveRight", ezInputSlot_KeyD);
  RegisterInputAction("Game", "MoveUp", ezInputSlot_KeyQ);
  RegisterInputAction("Game", "MoveDown", ezInputSlot_KeyE);
  RegisterInputAction("Game", "Run", ezInputSlot_KeyLeftShift);

  RegisterInputAction("Game", "TurnLeft", ezInputSlot_KeyLeft);
  RegisterInputAction("Game", "TurnRight", ezInputSlot_KeyRight);
  RegisterInputAction("Game", "TurnUp", ezInputSlot_KeyUp);
  RegisterInputAction("Game", "TurnDown", ezInputSlot_KeyDown);
}

void ezFallbackGameState::CreateRenderPipeline(ezGALRenderTargetViewHandle hBackBuffer, ezGALRenderTargetViewHandle hDSV)
{
  EZ_LOG_BLOCK("CreateRenderPipeline");

  ezVec3 vCameraPos = ezVec3(0.0f, 0.0f, 10.0f);

  ezCoordinateSystem coordSys;
  m_pWorld->GetCoordinateSystem(vCameraPos, coordSys);

  m_Camera.LookAt(vCameraPos, vCameraPos + coordSys.m_vForwardDir, coordSys.m_vUpDir);
  m_Camera.SetCameraMode(ezCamera::PerspectiveFixedFovY, 60.0f, 1.0f, 5000.0f);

  m_pView = ezRenderLoop::CreateView("View");
  ezRenderLoop::AddMainView(m_pView);

  ezGALRenderTagetSetup RTS;
  RTS.SetRenderTarget(0, hBackBuffer)
    .SetDepthStencilTarget(hDSV);
  m_pView->SetRenderTargetSetup(RTS);

  ezUniquePtr<ezRenderPipeline> pRenderPipeline = EZ_DEFAULT_NEW(ezRenderPipeline);

  ezSimpleRenderPass* pSimplePass = nullptr;
  {
    ezUniquePtr<ezRenderPipelinePass> pPass = EZ_DEFAULT_NEW(ezSimpleRenderPass);
    pSimplePass = static_cast<ezSimpleRenderPass*>(pPass.Borrow());
    pRenderPipeline->AddPass(std::move(pPass));
  }

  ezTargetPass* pTargetPass = nullptr;
  {
    ezUniquePtr<ezRenderPipelinePass> pPass = EZ_DEFAULT_NEW(ezTargetPass);
    pTargetPass = static_cast<ezTargetPass*>(pPass.Borrow());
    pRenderPipeline->AddPass(std::move(pPass));
  }

  EZ_VERIFY(pRenderPipeline->Connect(pSimplePass, "Color", pTargetPass, "Color0"), "Connect failed!");
  EZ_VERIFY(pRenderPipeline->Connect(pSimplePass, "DepthStencil", pTargetPass, "DepthStencil"), "Connect failed!");

  pRenderPipeline->AddExtractor(EZ_DEFAULT_NEW(ezVisibleObjectsExtractor));
  m_pView->SetRenderPipeline(std::move(pRenderPipeline));

  ezSizeU32 size = m_pWindow->GetClientAreaSize();
  m_pView->SetViewport(ezRectFloat(0.0f, 0.0f, (float)size.width, (float)size.height));

  m_pView->SetWorld(m_pWorld);
  m_pView->SetLogicCamera(&m_Camera);

}

void ezFallbackGameState::ProcessInput()
{
  float fRotateSpeed = 180.0f;
  float fMoveSpeed = 10.0f;
  float fInput = 0.0f;

  if (ezInputManager::GetInputActionState("Game", "Run", &fInput) != ezKeyState::Up)
    fMoveSpeed *= 10.0f;

  if (ezInputManager::GetInputActionState("Game", "MoveForwards", &fInput) != ezKeyState::Up)
    m_Camera.MoveLocally(fInput * fMoveSpeed, 0, 0);
  if (ezInputManager::GetInputActionState("Game", "MoveBackwards", &fInput) != ezKeyState::Up)
    m_Camera.MoveLocally(-fInput * fMoveSpeed, 0, 0);
  if (ezInputManager::GetInputActionState("Game", "MoveLeft", &fInput) != ezKeyState::Up)
    m_Camera.MoveLocally(0, -fInput * fMoveSpeed, 0);
  if (ezInputManager::GetInputActionState("Game", "MoveRight", &fInput) != ezKeyState::Up)
    m_Camera.MoveLocally(0, fInput * fMoveSpeed, 0);

  if (ezInputManager::GetInputActionState("Game", "MoveUp", &fInput) != ezKeyState::Up)
    m_Camera.MoveGlobally(ezVec3(0, 0, fInput * fMoveSpeed));
  if (ezInputManager::GetInputActionState("Game", "MoveDown", &fInput) != ezKeyState::Up)
    m_Camera.MoveGlobally(ezVec3(0, 0, -fInput * fMoveSpeed));

  if (ezInputManager::GetInputActionState("Game", "TurnLeft", &fInput) != ezKeyState::Up)
    m_Camera.RotateGlobally(ezAngle(), ezAngle(), ezAngle::Degree(-fRotateSpeed * fInput));
  if (ezInputManager::GetInputActionState("Game", "TurnRight", &fInput) != ezKeyState::Up)
    m_Camera.RotateGlobally(ezAngle(), ezAngle(), ezAngle::Degree(fRotateSpeed * fInput));
  if (ezInputManager::GetInputActionState("Game", "TurnUp", &fInput) != ezKeyState::Up)
    m_Camera.RotateLocally(ezAngle(), ezAngle::Degree(-fRotateSpeed * fInput), ezAngle());
  if (ezInputManager::GetInputActionState("Game", "TurnDown", &fInput) != ezKeyState::Up)
    m_Camera.RotateLocally(ezAngle(), ezAngle::Degree(fRotateSpeed * fInput), ezAngle());
}

