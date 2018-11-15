
#include <PCH.h>

#include <Core/World/World.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/GameStateWindow.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <System/Screen/Screen.h>

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
#  include <MixedReality/MixedRealityFramework.h>
#  include <WindowsMixedReality/HolographicSpace.h>
#endif

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGameState, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_STATICLINK_FILE(GameEngine, GameEngine_GameState_Implementation_GameState);

ezGameState::ezGameState() {}

ezGameState::~ezGameState() {}

void ezGameState::OnActivation(ezWorld* pWorld, const ezTransform* pStartPosition)
{
  m_pMainWorld = pWorld;
  bool bCreateNewWindow = true;

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  if ((GetApplication()->GetAppType() == ezGameApplicationType::StandAloneMixedReality ||
       GetApplication()->GetAppType() == ezGameApplicationType::EmbeddedInToolMixedReality) &&
      ezWindowsHolographicSpace::GetSingleton()->IsAvailable())
  {
    m_bMixedRealityMode = true;
  }

  if (GetApplication()->GetAppType() == ezGameApplicationType::EmbeddedInToolMixedReality)
    bCreateNewWindow = false;

#endif

  // a bit hacky to get this to work with Mixed Reality
  if (bCreateNewWindow)
  {
    CreateMainWindow();

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
    if (m_bMixedRealityMode)
    {
      // HololensRenderPipeline.ezRendePipelineAsset
      auto hRenderPipeline = ezResourceManager::LoadResource<ezRenderPipelineResource>("{ 2fe25ded-776c-7f9e-354f-e4c52a33d125 }");

      auto pHoloFramework = ezMixedRealityFramework::GetSingleton();
      m_hMainView = pHoloFramework->CreateHolographicView(m_pMainWindow, hRenderPipeline, &m_MainCamera, m_pMainWorld);
      m_hMainSwapChain = ezGALDevice::GetDefaultDevice()->GetPrimarySwapChain();
    }
    else
#endif
    {
      const ezGALSwapChain* pSwapChain = ezGALDevice::GetDefaultDevice()->GetSwapChain(m_hMainSwapChain);
      SetupMainView(ezGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(pSwapChain->GetBackBufferTexture()));
    }

    ConfigureMainCamera();

    ConfigureInputDevices();
  }


  ConfigureInputActions();
}

void ezGameState::OnDeactivation()
{
  ezRenderWorld::DeleteView(m_hMainView);

  DestroyMainWindow();
}

void ezGameState::AddAllMainViews()
{
  ezRenderWorld::AddMainView(m_hMainView);
}

void ezGameState::CreateMainWindow()
{
  EZ_LOG_BLOCK("ezGameState::CreateMainWindow");

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  if (m_bMixedRealityMode)
  {
    m_pMainWindow = EZ_DEFAULT_NEW(ezGameStateWindow, ezWindowCreationDesc(), [this]() { RequestQuit(); });
    GetApplication()->AddWindow(m_pMainWindow, ezGALSwapChainHandle());
    return;
  }
#endif

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

  ezWindowCreationDesc wndDesc;
  wndDesc.LoadFromDDL(sWndCfg);
  // wndDesc.SaveToDDL(":project/Window.ddl");

  GetApplication()->AdjustWindowCreation(wndDesc);

  m_pMainWindow = EZ_DEFAULT_NEW(ezGameStateWindow, wndDesc, [this]() { RequestQuit(); });
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

void ezGameState::ConfigureInputActions() {}

void ezGameState::SetupMainView(ezGALRenderTargetViewHandle hBackBuffer)
{
  const auto* pConfig = GetApplication()->GetPlatformProfile().GetTypeConfig<ezRenderPipelineProfileConfig>();

  SetupMainView(hBackBuffer, ezResourceManager::LoadResource<ezRenderPipelineResource>(pConfig->m_sMainRenderPipeline));
}

void ezGameState::SetupMainView(ezGALRenderTargetViewHandle hBackBuffer, ezTypedResourceHandle<ezRenderPipelineResource> hRenderPipeline)
{
  EZ_LOG_BLOCK("SetupMainView");

  ezView* pView = nullptr;
  m_hMainView = ezRenderWorld::CreateView("MainView", pView);
  pView->SetCameraUsageHint(ezCameraUsageHint::MainView);

  ezGALRenderTagetSetup renderTargetSetup;
  renderTargetSetup.SetRenderTarget(0, hBackBuffer);
  pView->SetRenderTargetSetup(renderTargetSetup);
  pView->SetRenderPipelineResource(hRenderPipeline);

  ezSizeU32 size = m_pMainWindow->GetClientAreaSize();
  pView->SetViewport(ezRectFloat(0.0f, 0.0f, (float)size.width, (float)size.height));

  pView->SetWorld(m_pMainWorld);
  pView->SetCamera(&m_MainCamera);

  const ezTag& tagEditor = ezTagRegistry::GetGlobalRegistry().RegisterTag("Editor");

  // exclude all editor objects from rendering in proper game views
  pView->m_ExcludeTags.Set(tagEditor);

  ezRenderWorld::AddMainView(m_hMainView);
}

void ezGameState::ChangeMainWorld(ezWorld* pNewMainWorld)
{
  m_pMainWorld = pNewMainWorld;

  ezView* pView = nullptr;
  if (ezRenderWorld::TryGetView(m_hMainView, pView))
  {
    pView->SetWorld(m_pMainWorld);
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

  // if the camera is already set to be in 'Stereo' mode, its parameters are set from the outside
  if (m_MainCamera.GetCameraMode() != ezCameraMode::Stereo)
  {
    m_MainCamera.LookAt(vCameraPos, vCameraPos + coordSys.m_vForwardDir, coordSys.m_vUpDir);
    m_MainCamera.SetCameraMode(ezCameraMode::PerspectiveFixedFovY, 60.0f, 0.1f, 1000.0f);
  }
}
