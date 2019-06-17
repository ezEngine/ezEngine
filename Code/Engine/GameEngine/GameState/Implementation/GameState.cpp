
#include <GameEnginePCH.h>

#include <Core/ActorSystem/Actor.h>
#include <Core/ActorSystem/ActorManager.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <GameApplication/WindowOutputTarget.h>
#include <GameEngine/Configuration/RendererProfileConfigs.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/GameStateWindow.h>
#include <GameEngine/Gameplay/PlayerStartPointComponent.h>
#include <GameEngine/Interfaces/VRInterface.h>
#include <GameEngine/Prefabs/PrefabResource.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <System/Screen/Screen.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGameState, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_STATICLINK_FILE(GameEngine, GameEngine_GameState_Implementation_GameState);
// clang-format on

ezGameState::ezGameState() {}

ezGameState::~ezGameState() {}

void ezGameState::OnActivation(ezWorld* pWorld, const ezTransform* pStartPosition)
{
  m_pMainWorld = pWorld;
  // bool bCreateNewWindow = true;
  // m_bVirtualRealityMode = ezCommandLineUtils::GetGlobalInstance()->GetBoolOption("-vr", false);
  // if (m_bVirtualRealityMode && !ezSingletonRegistry::GetSingletonInstance<ezVRInterface>())
  //{
  //  m_bVirtualRealityMode = false;
  //  ezLog::Error("-vr argument ignored, no ezVRInterface present.");
  //}
  //#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  //  if ((GetApplication()->GetAppType() == ezGameApplicationType::StandAloneMixedReality ||
  //        GetApplication()->GetAppType() == ezGameApplicationType::EmbeddedInToolMixedReality) &&
  //      ezWindowsHolographicSpace::GetSingleton()->IsAvailable())
  //  {
  //    m_bMixedRealityMode = true;
  //  }
  //
  //  if (GetApplication()->GetAppType() == ezGameApplicationType::EmbeddedInToolMixedReality)
  //    bCreateNewWindow = false;
  //
  //#endif

  // a bit hacky to get this to work with Mixed Reality
  // if (bCreateNewWindow)
  {
    CreateActors();

    //#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
    //    if (m_bMixedRealityMode)
    //    {
    //      // HololensRenderPipeline.ezRendePipelineAsset
    //      auto hRenderPipeline = ezResourceManager::LoadResource<ezRenderPipelineResource>("{ 2fe25ded-776c-7f9e-354f-e4c52a33d125 }");
    //
    //      auto pHoloFramework = ezMixedRealityFramework::GetSingleton();
    //      m_hMainView = pHoloFramework->CreateHolographicView(m_pMainWindow, hRenderPipeline, &m_MainCamera, m_pMainWorld);
    //      m_hMainSwapChain = ezGALDevice::GetDefaultDevice()->GetPrimarySwapChain();
    //    }
    //    else
    //#endif
    //      if (m_bVirtualRealityMode)
    //    {
    //      // TODO: Don't hardcode the HololensRenderPipeline.ezRendePipelineAsset
    //      auto hRenderPipeline = ezResourceManager::LoadResource<ezRenderPipelineResource>("{ 2fe25ded-776c-7f9e-354f-e4c52a33d125 }");
    //      ezVRInterface* pVRInterface = ezSingletonRegistry::GetRequiredSingletonInstance<ezVRInterface>();
    //      pVRInterface->Initialize();
    //      m_hMainView = pVRInterface->CreateVRView(hRenderPipeline, &m_MainCamera);
    //      ChangeMainWorld(pWorld);
    //
    //      if (pVRInterface->SupportsCompanionView())
    //      {
    //        ezWindowOutputTargetGAL* pOutputGAL = static_cast<ezWindowOutputTargetGAL*>(m_pMainOutputTarget);
    //
    //        const ezGALSwapChain* pSwapChain = ezGALDevice::GetDefaultDevice()->GetSwapChain(pOutputGAL->m_hSwapChain);
    //        pVRInterface->SetCompanionViewRenderTarget(pSwapChain->GetBackBufferTexture());
    //      }
    //    }


    ConfigureMainCamera();
  }

  ConfigureInputActions();

  SpawnPlayer(pStartPosition);
}

void ezGameState::OnDeactivation()
{
  // if (m_bVirtualRealityMode)
  //{
  //  ezVRInterface* pVRInterface = ezSingletonRegistry::GetRequiredSingletonInstance<ezVRInterface>();
  //  pVRInterface->DestroyVRView();
  //  pVRInterface->Deinitialize();
  //}
  // else
  {
    ezRenderWorld::DeleteView(m_hMainView);
  }
}

void ezGameState::ScheduleRendering()
{
  ezRenderWorld::AddMainView(m_hMainView);
}

void ezGameState::CreateActors()
{
  EZ_LOG_BLOCK("ezGameState::CreateMainWindow");

  // TODO: MR support
  //#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  //  if (m_bMixedRealityMode)
  //  {
  //    m_pMainWindow = EZ_DEFAULT_NEW(ezGameStateWindow, ezWindowCreationDesc(), [this]() { RequestQuit(); });
  //    GetApplication()->AddWindow(m_pMainWindow, ezGALSwapChainHandle());
  //    return;
  //  }
  //#endif

  // TODO: VR support
  // if (m_bVirtualRealityMode)
  //{
  //  wndDesc.m_bClipMouseCursor = false;
  //  wndDesc.m_bShowMouseCursor = true;
  //  wndDesc.m_WindowMode = ezWindowMode::WindowResizable;
  //}

  ezUniquePtr<ezActor> pActor = EZ_DEFAULT_NEW(ezActor, "Main Window", this);

  ezUniquePtr<ezWindow> pMainWindow = CreateMainWindow();
  EZ_ASSERT_DEV(pMainWindow != nullptr, "To change the main window creation behavior, override ezGameState::CreateActors().");

  ezUniquePtr<ezWindowOutputTargetBase> pOutput = CreateMainOutputTarget(pMainWindow.Borrow());

  ConfigureMainWindowInputDevices(pMainWindow.Borrow());

  pActor->m_pWindow = std::move(pMainWindow);
  pActor->m_pWindowOutputTarget = std::move(pOutput);

  SetupMainView(pActor->m_pWindowOutputTarget.Borrow(), pActor->m_pWindow->GetClientAreaSize());


  ezActorManager::GetSingleton()->AddActor(std::move(pActor));
}

void ezGameState::ConfigureMainWindowInputDevices(ezWindow* pWindow) {}

void ezGameState::ConfigureInputActions() {}

void ezGameState::SetupMainView(ezWindowOutputTargetBase* pOutputTarget, ezSizeU32 viewportSize)
{
  const auto* pConfig =
    ezGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetTypeConfig<ezRenderPipelineProfileConfig>();

  SetupMainView(pOutputTarget, viewportSize, ezResourceManager::LoadResource<ezRenderPipelineResource>(pConfig->m_sMainRenderPipeline));
}

void ezGameState::SetupMainView(
  ezWindowOutputTargetBase* pOutputTarget, ezSizeU32 viewportSize, ezTypedResourceHandle<ezRenderPipelineResource> hRenderPipeline)
{
  EZ_LOG_BLOCK("SetupMainView");

  ezWindowOutputTargetGAL* pOutputGAL = static_cast<ezWindowOutputTargetGAL*>(pOutputTarget);

  const ezGALSwapChain* pSwapChain = ezGALDevice::GetDefaultDevice()->GetSwapChain(pOutputGAL->m_hSwapChain);
  auto hBackBuffer = ezGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(pSwapChain->GetBackBufferTexture());

  ezView* pView = nullptr;
  m_hMainView = ezRenderWorld::CreateView("MainView", pView);
  pView->SetCameraUsageHint(ezCameraUsageHint::MainView);

  ezGALRenderTargetSetup renderTargetSetup;
  renderTargetSetup.SetRenderTarget(0, hBackBuffer);
  pView->SetRenderTargetSetup(renderTargetSetup);
  pView->SetRenderPipelineResource(hRenderPipeline);

  pView->SetViewport(ezRectFloat(0.0f, 0.0f, (float)viewportSize.width, (float)viewportSize.height));

  pView->SetWorld(m_pMainWorld);
  pView->SetCamera(&m_MainCamera);

  const ezTag& tagEditor = ezTagRegistry::GetGlobalRegistry().RegisterTag("Editor");

  // exclude all editor objects from rendering in proper game views
  pView->m_ExcludeTags.Set(tagEditor);

  ezRenderWorld::AddMainView(m_hMainView);
}

ezResult ezGameState::SpawnPlayer(const ezTransform* pStartPosition)
{
  if (m_pMainWorld == nullptr)
    return EZ_FAILURE;

  EZ_LOCK(m_pMainWorld->GetWriteMarker());

  ezPlayerStartPointComponentManager* pMan = m_pMainWorld->GetComponentManager<ezPlayerStartPointComponentManager>();
  if (pMan == nullptr)
    return EZ_FAILURE;

  for (auto it = pMan->GetComponents(); it.IsValid(); ++it)
  {
    if (it->GetPlayerPrefab().IsValid())
    {
      ezResourceLock<ezPrefabResource> pPrefab(it->GetPlayerPrefab(), ezResourceAcquireMode::NoFallback);

      if (pPrefab.GetAcquireResult() == ezResourceAcquireResult::Final)
      {
        const ezUInt16 uiTeamID = it->GetOwner()->GetTeamID();
        ezTransform startPos = it->GetOwner()->GetGlobalTransform();

        if (pStartPosition)
        {
          startPos = *pStartPosition;
          startPos.m_vScale.Set(1.0f);
          startPos.m_vPosition.z += 1.0f; // do not spawn player prefabs on the ground, they may not have their origin there
        }

        pPrefab->InstantiatePrefab(*m_pMainWorld, startPos, ezGameObjectHandle(), nullptr, &uiTeamID, nullptr);

        return EZ_SUCCESS;
      }
    }
  }

  return EZ_FAILURE;
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

ezUniquePtr<ezWindow> ezGameState::CreateMainWindow()
{
  if (false)
  {
    ezHybridArray<ezScreenInfo, 2> screens;
    ezScreen::EnumerateScreens(screens);
    ezScreen::PrintScreenInfo(screens);
  }

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

  ezUniquePtr<ezGameStateWindow> pWindow = EZ_DEFAULT_NEW(ezGameStateWindow, wndDesc, [] {});
  pWindow->ResetOnClickClose([]() { ezGameApplicationBase::GetGameApplicationBaseInstance()->RequestQuit(); });
  pWindow->GetInputDevice()->SetMouseSpeed(ezVec2(0.002f));

  return pWindow;
}

ezUniquePtr<ezWindowOutputTargetBase> ezGameState::CreateMainOutputTarget(ezWindow* pMainWindow)
{
  ezUniquePtr<ezWindowOutputTargetGAL> pOutput = EZ_DEFAULT_NEW(ezWindowOutputTargetGAL);

  ezGALSwapChainCreationDescription desc;
  desc.m_pWindow = pMainWindow;
  desc.m_BackBufferFormat = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
  desc.m_bAllowScreenshots = true;

  pOutput->CreateSwapchain(desc);

  return pOutput;
}
