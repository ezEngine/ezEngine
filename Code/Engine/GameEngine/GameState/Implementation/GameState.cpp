
#include <GameEngine/PCH.h>
#include <GameEngine/GameState/GameStateWindow.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>
#include <RendererCore/Pipeline/Passes/SimpleRenderPass.h>
#include <RendererCore/Pipeline/Passes/TargetPass.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <RendererCore/Meshes/MeshRenderer.h>
#include <System/Screen/Screen.h>

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
  SetupMainView(ezGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(pSwapChain->GetBackBufferTexture()));

  ConfigureMainCamera();

  ConfigureInputDevices();

  ConfigureInputActions();
}


void ezGameState::OnDeactivation()
{
  ezRenderLoop::DeleteView(m_pMainView);
  m_pMainView = nullptr;

  DestroyMainWindow();
}


void ezGameState::AddAllMainViews()
{
  ezRenderLoop::AddMainView(m_pMainView);
}

void ezGameState::CreateMainWindow()
{
  EZ_LOG_BLOCK("ezGameState::CreateMainWindow");

  ezHybridArray<ezScreenInfo, 2> screens;
  ezScreen::EnumerateScreens(screens);
  ezScreen::PrintScreenInfo(screens);

  ezStringBuilder sWndCfg = ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-wnd", 0, "");

  if (!sWndCfg.IsEmpty() && !ezFileSystem::ExistsFile(sWndCfg))
  {
    ezLog::Dev("Window Config file does not exist: '{0}'", sWndCfg);
    sWndCfg.Clear();
  }

  if (sWndCfg.IsEmpty())
  {
    if (ezFileSystem::ExistsFile(":appdata/Window.ddl"))
      sWndCfg = ":appdata/Window.ddl";
    else
      sWndCfg = ":project/Window.ddl";
  }

  if (GetApplication()->GetAppType() == ezGameApplicationType::EmbeddedInTool)
  {
  }

  ezWindowCreationDesc wndDesc;
  wndDesc.LoadFromDDL(sWndCfg);
  //wndDesc.SaveToDDL(":project/Window.ddl");

  m_pMainWindow = EZ_DEFAULT_NEW(ezGameStateWindow, wndDesc);
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
  m_pMainWindow->GetInputDevice()->SetMouseSpeed(ezVec2(0.002f));
}

void ezGameState::ConfigureInputActions()
{

}

void ezGameState::SetupMainView(ezGALRenderTargetViewHandle hBackBuffer)
{
  EZ_LOG_BLOCK("SetupMainView");

  m_pMainView = ezRenderLoop::CreateView("MainView");
  m_pMainView->SetCameraUsageHint(ezCameraUsageHint::MainView);

  ezGALRenderTagetSetup renderTargetSetup;
  renderTargetSetup.SetRenderTarget(0, hBackBuffer);
  m_pMainView->SetRenderTargetSetup(renderTargetSetup);

  m_pMainView->SetRenderPipelineResource(ezResourceManager::LoadResource<ezRenderPipelineResource>("{ c533e113-2a4c-4f42-a546-653c78f5e8a7 }"));

  ezSizeU32 size = m_pMainWindow->GetClientAreaSize();
  m_pMainView->SetViewport(ezRectFloat(0.0f, 0.0f, (float)size.width, (float)size.height));

  m_pMainView->SetWorld(m_pMainWorld);
  m_pMainView->SetLogicCamera(&m_MainCamera);

  const ezTag* tagEditor = ezTagRegistry::GetGlobalRegistry().RegisterTag("Editor");

  // exclude all editor objects from rendering in proper game views
  m_pMainView->m_ExcludeTags.Set(*tagEditor);

  ezRenderLoop::AddMainView(m_pMainView);
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
  m_MainCamera.SetCameraMode(ezCameraMode::PerspectiveFixedFovY, 60.0f, 0.1f, 1000.0f);
}
