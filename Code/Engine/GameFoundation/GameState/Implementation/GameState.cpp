
#include <GameFoundation/PCH.h>
#include <GameFoundation/GameState/GameStateWindow.h>
#include <GameFoundation/GameApplication/GameApplication.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>
#include <RendererCore/Pipeline/Passes/SimpleRenderPass.h>
#include <RendererCore/Pipeline/Passes/TargetPass.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <RendererCore/Meshes/MeshRenderer.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGameState, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE



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


void ezGameState::OnActivation(ezWorld* pWorld)
{
  m_bStateWantsToQuit = false;
  m_pMainWorld = pWorld;

  CreateMainWindow();

  const ezGALSwapChain* pSwapChain = ezGALDevice::GetDefaultDevice()->GetSwapChain(m_hMainSwapChain);
  SetupMainView(ezGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(pSwapChain->GetBackBufferTexture()), 
    ezGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(pSwapChain->GetDepthStencilBufferTexture()));

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

void ezGameState::SetupMainView(ezGALRenderTargetViewHandle hBackBuffer, ezGALRenderTargetViewHandle hDSV)
{
  EZ_LOG_BLOCK("SetupMainView");

  m_pMainView = ezRenderLoop::CreateView("MainView");
  ezRenderLoop::AddMainView(m_pMainView);

  ezGALRenderTagetSetup RTS;
  RTS.SetRenderTarget(0, hBackBuffer)
    .SetDepthStencilTarget(hDSV);
  m_pMainView->SetRenderTargetSetup(RTS);

  m_pMainView->SetRenderPipelineResource(CreateMainRenderPipeline());

  ezSizeU32 size = m_pMainWindow->GetClientAreaSize();
  m_pMainView->SetViewport(ezRectFloat(0.0f, 0.0f, (float)size.width, (float)size.height));

  m_pMainView->SetWorld(m_pMainWorld);
  m_pMainView->SetLogicCamera(&m_MainCamera);

  ezTag tagEditor;
  ezTagRegistry::GetGlobalRegistry().RegisterTag("Editor", &tagEditor);

  // exclude all editor objects from rendering in proper game views
  m_pMainView->m_ExcludeTags.Set(tagEditor);
}


ezRenderPipelineResourceHandle ezGameState::CreateMainRenderPipeline()
{
  auto hRP = ezResourceManager::GetExistingResource<ezRenderPipelineResource>("MainRenderPipeline");
  if (hRP.IsValid())
    return hRP;

  ezUniquePtr<ezRenderPipeline> pRenderPipeline = EZ_DEFAULT_NEW(ezRenderPipeline);

  ezForwardRenderPass* pForwardPass = nullptr;
  {
    ezUniquePtr<ezForwardRenderPass> pPass = EZ_DEFAULT_NEW(ezForwardRenderPass);
    pForwardPass = pPass.Borrow();

    pForwardPass->AddRenderer(EZ_DEFAULT_NEW(ezMeshRenderer));

    pRenderPipeline->AddPass(std::move(pPass));
  }

  ezSimpleRenderPass* pSimplePass = nullptr;
  {
    ezUniquePtr<ezSimpleRenderPass> pPass = EZ_DEFAULT_NEW(ezSimpleRenderPass);
    pSimplePass = pPass.Borrow();
    pRenderPipeline->AddPass(std::move(pPass));
  }

  ezTargetPass* pTargetPass = nullptr;
  {
    ezUniquePtr<ezTargetPass> pPass = EZ_DEFAULT_NEW(ezTargetPass);
    pTargetPass = pPass.Borrow();
    pRenderPipeline->AddPass(std::move(pPass));
  }

  EZ_VERIFY(pRenderPipeline->Connect(pForwardPass, "Color", pSimplePass, "Color"), "Connect failed!");
  EZ_VERIFY(pRenderPipeline->Connect(pForwardPass, "DepthStencil", pSimplePass, "DepthStencil"), "Connect failed!");

  EZ_VERIFY(pRenderPipeline->Connect(pSimplePass, "Color", pTargetPass, "Color0"), "Connect failed!");
  EZ_VERIFY(pRenderPipeline->Connect(pSimplePass, "DepthStencil", pTargetPass, "DepthStencil"), "Connect failed!");

  pRenderPipeline->AddExtractor(EZ_DEFAULT_NEW(ezVisibleObjectsExtractor));


  ezRenderPipelineResourceDescriptor desc;
  ezRenderPipelineResourceLoader::CreateRenderPipelineResourceDescriptor(pRenderPipeline.Borrow(), desc);

  return ezResourceManager::CreateResource<ezRenderPipelineResource>("MainRenderPipeline", desc);
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

  if (m_pMainWorld)
  {
    m_pMainWorld->GetCoordinateSystem(vCameraPos, coordSys);
  }
  else
  {
    coordSys.m_vForwardDir.Set(1, 0, 0);
    coordSys.m_vRightDir.Set(0, 1, 0);
    coordSys.m_vUpDir.Set(0, 0, 1);
  }

  m_MainCamera.LookAt(vCameraPos, vCameraPos + coordSys.m_vForwardDir, coordSys.m_vUpDir);
  m_MainCamera.SetCameraMode(ezCameraMode::PerspectiveFixedFovY, 60.0f, 1.0f, 5000.0f);
}