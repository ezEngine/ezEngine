
#include <GameFoundation/PCH.h>
#include <GameFoundation/GameState/GameState.h>
#include <GameFoundation/GameState/GameStateWindow.h>
#include <GameFoundation/GameApplication/GameApplication.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/SimpleRenderPass.h>
#include <RendererCore/Pipeline/TargetPass.h>
#include <RendererCore/Pipeline/Extractor.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGameState, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();



EZ_STATICLINK_FILE(GameFoundation, GameFoundation_GameState);


ezGameState::ezGameState() 
  : m_pApplication(nullptr)
  , m_pMainWindow(nullptr)
  , m_pMainWorld(nullptr)
  , m_pMainView(nullptr)
  , m_bStateWantsToQuit(false)
{

}

ezGameState::~ezGameState()
{

}


void ezGameState::OnActivation(ezGameApplicationType AppType, ezWorld* pWorld)
{
  m_bStateWantsToQuit = false;
  m_pMainWorld = pWorld;

  CreateMainWindow();

  const ezGALSwapChain* pSwapChain = ezGALDevice::GetDefaultDevice()->GetSwapChain(m_hMainSwapChain);
  CreateMainRenderPipeline(pSwapChain->GetBackBufferRenderTargetView(), pSwapChain->GetDepthStencilTargetView());

  ConfigureMainCamera();

  ConfigureInputDevices();

  ConfigureInputActions();
}


void ezGameState::OnDeactivation()
{
  DestroyMainWindow();
}


void ezGameState::AddAllMainViews()
{
  ezRenderLoop::AddMainView(m_pMainView);
}

void ezGameState::CreateMainWindow()
{
  EZ_LOG_BLOCK("ezGameState::CreateMainWindow");

  m_pMainWindow = EZ_DEFAULT_NEW(ezGameStateWindow);
  m_hMainSwapChain = GetApplication()->AddWindow(m_pMainWindow);
}

void ezGameState::DestroyMainWindow()
{
  if (m_pMainWindow)
  {
    GetApplication()->RemoveWindow(m_pMainWindow);
    EZ_DEFAULT_DELETE(m_pMainWindow);

    m_hMainSwapChain.Invalidate();
  }
}

void ezGameState::ConfigureInputDevices()
{
  m_pMainWindow->GetInputDevice()->SetClipMouseCursor(true);
  m_pMainWindow->GetInputDevice()->SetShowMouseCursor(false);
  m_pMainWindow->GetInputDevice()->SetMouseSpeed(ezVec2(0.002f));
}

void ezGameState::ConfigureInputActions()
{

}

void ezGameState::CreateMainRenderPipeline(ezGALRenderTargetViewHandle hBackBuffer, ezGALRenderTargetViewHandle hDSV)
{
  EZ_LOG_BLOCK("CreateMainRenderPipeline");

  m_pMainView = ezRenderLoop::CreateView("View");
  ezRenderLoop::AddMainView(m_pMainView);

  ezGALRenderTagetSetup RTS;
  RTS.SetRenderTarget(0, hBackBuffer)
    .SetDepthStencilTarget(hDSV);
  m_pMainView->SetRenderTargetSetup(RTS);

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
  m_pMainView->SetRenderPipeline(std::move(pRenderPipeline));

  ezSizeU32 size = m_pMainWindow->GetClientAreaSize();
  m_pMainView->SetViewport(ezRectFloat(0.0f, 0.0f, (float)size.width, (float)size.height));

  m_pMainView->SetWorld(m_pMainWorld);
  m_pMainView->SetLogicCamera(&m_MainCamera);
}

void ezGameState::ChangeMainWorld(ezWorld* pNewMainWorld)
{
  m_pMainWorld = pNewMainWorld;

  if (m_pMainView)
  {
    m_pMainView->SetWorld(m_pMainWorld);
  }
}

void ezGameState::ConfigureMainCamera()
{
  ezVec3 vCameraPos = ezVec3(0.0f, 0.0f, 0.0f);

  ezCoordinateSystem coordSys;
  m_pMainWorld->GetCoordinateSystem(vCameraPos, coordSys);

  m_MainCamera.LookAt(vCameraPos, vCameraPos + coordSys.m_vForwardDir, coordSys.m_vUpDir);
  m_MainCamera.SetCameraMode(ezCamera::PerspectiveFixedFovY, 60.0f, 1.0f, 5000.0f);
}