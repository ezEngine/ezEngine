
#include <GameEngine/GameEnginePCH.h>

#include <Core/ActorSystem/Actor.h>
#include <Core/ActorSystem/ActorManager.h>
#include <Core/ActorSystem/ActorPluginWindow.h>
#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/GameState/GameStateWindow.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/System/Screen.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GameEngine/Configuration/RendererProfileConfigs.h>
#include <GameEngine/Configuration/XRConfig.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GameEngine/Gameplay/PlayerStartPointComponent.h>
#include <GameEngine/XR/DummyXR.h>
#include <GameEngine/XR/XRInterface.h>
#include <GameEngine/XR/XRRemotingInterface.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Utils/CoreRenderProfile.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>

ezCommandLineOptionPath opt_Window("GameState", "-wnd", "Path to the window configuration file to use.", "");

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGameState, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_STATICLINK_FILE(GameEngine, GameEngine_GameState_Implementation_GameState);
// clang-format on

ezGameState* ezGameState::s_pActiveGameState = nullptr;

ezGameState::ezGameState()
{
  // initialize camera to default values
  m_MainCamera.SetCameraMode(ezCameraMode::PerspectiveFixedFovY, 60.0f, 0.1f, 1000.0f);
  m_MainCamera.LookAt(ezVec3::MakeZero(), ezVec3(1, 0, 0), ezVec3(0, 0, 1));
}

ezGameState::~ezGameState() = default;

ezGameState* ezGameState::GetActiveGameState()
{
  return s_pActiveGameState;
}

void ezGameState::OnActivation(ezWorld* pWorld, ezStringView sStartPosition, const ezTransform& startPositionOffset)
{
  s_pActiveGameState = this;

  CreateActors();
  ConfigureInputActions();

  if (pWorld)
  {
    ChangeMainWorld(pWorld, sStartPosition, startPositionOffset);
  }
  else
  {
    ezStringBuilder sSceneFile = GetStartupSceneFile();

    if (!sSceneFile.IsEmpty())
    {
      // TODO: also pass along a preload collection
      LoadScene(sSceneFile, {}, sStartPosition, startPositionOffset);
    }
  }
}

void ezGameState::OnDeactivation()
{
  CancelBackgroundSceneLoading();

  if (m_bXREnabled)
  {
    m_bXREnabled = false;
    ezXRInterface* pXRInterface = ezSingletonRegistry::GetSingletonInstance<ezXRInterface>();
    ezActorManager::GetSingleton()->DestroyAllActors(pXRInterface);
    pXRInterface->Deinitialize();

    if (ezXRRemotingInterface* pXRRemotingInterface = ezSingletonRegistry::GetSingletonInstance<ezXRRemotingInterface>())
    {
      if (pXRRemotingInterface->Deinitialize().Failed())
      {
        ezLog::Error("Failed to deinitialize ezXRRemotingInterface, make sure all actors are destroyed and ezXRInterface deinitialized.");
      }
    }

    m_pDummyXR = nullptr;
  }

  ezRenderWorld::DeleteView(m_hMainView);

  s_pActiveGameState = nullptr;
}

void ezGameState::AddMainViewsToRender()
{
  if (!m_hMainView.IsInvalidated())
  {
    ezRenderWorld::AddMainView(m_hMainView);
  }
}

void ezGameState::RequestQuit()
{
  m_bStateWantsToQuit = true;
}

bool ezGameState::WasQuitRequested() const
{
  return m_bStateWantsToQuit;
}


void ezGameState::ProcessInput()
{
  UpdateBackgroundSceneLoading();
}

ezView* ezGameState::GetMainView()
{
  ezView* pView = nullptr;
  if (ezRenderWorld::TryGetView(m_hMainView, pView))
  {
    return pView;
  }

  return nullptr;
}

bool ezGameState::IsLoadingSceneInBackground(float* out_pProgress) const
{
  if (out_pProgress)
  {
    *out_pProgress = 0.0f;

    if (m_pBackgroundSceneLoad != nullptr)
    {
      *out_pProgress = m_pBackgroundSceneLoad->GetLoadingProgress();

      auto state = m_pBackgroundSceneLoad->GetLoadingState();
      if (state != ezSceneLoadUtility::LoadingState::FinishedSuccessfully)
      {
        *out_pProgress = ezMath::Min(*out_pProgress, 0.99f);
      }
    }
  }

  return m_pBackgroundSceneLoad != nullptr;
}

bool ezGameState::IsInLoadingScreen() const
{
  return m_pMainWorld == m_pLoadingScreenWorld;
}

ezUniquePtr<ezActor> ezGameState::CreateXRActor()
{
  EZ_LOG_BLOCK("CreateXRActor");
  // Init XR
  const ezXRConfig* pConfig = ezGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetTypeConfig<ezXRConfig>();
  if (!pConfig)
    return nullptr;

  if (!pConfig->m_bEnableXR)
    return nullptr;

  ezXRInterface* pXRInterface = ezSingletonRegistry::GetSingletonInstance<ezXRInterface>();
  if (!pXRInterface)
  {
    ezLog::Warning("No ezXRInterface interface found. Please load a XR plugin to enable XR. Loading dummyXR interface.");
    m_pDummyXR = EZ_DEFAULT_NEW(ezDummyXR);
    pXRInterface = ezSingletonRegistry::GetSingletonInstance<ezXRInterface>();
    EZ_ASSERT_DEV(pXRInterface, "Creating dummyXR did not register the ezXRInterface.");
  }

  ezXRRemotingInterface* pXRRemotingInterface = ezSingletonRegistry::GetSingletonInstance<ezXRRemotingInterface>();
  if (ezXRRemotingInterface::cvar_XrRemoting)
  {
    if (pXRRemotingInterface)
    {
      if (pXRRemotingInterface->Initialize().Failed())
      {
        ezLog::Error("ezXRRemotingInterface could not be initialized. See log for details.");
      }
      else
      {
        m_bXRRemotingEnabled = true;
      }
    }
    else
    {
      ezLog::Error("No ezXRRemotingInterface interface found. Please load a XR remoting plugin to enable XR Remoting.");
    }
  }

  if (pXRInterface->Initialize().Failed())
  {
    ezLog::Error("ezXRInterface could not be initialized. See log for details.");
    return nullptr;
  }
  m_bXREnabled = true;

  ezUniquePtr<ezWindow> pMainWindow;
  ezUniquePtr<ezWindowOutputTargetGAL> pOutput;

  if (pXRInterface->SupportsCompanionView())
  {
    // XR Window with added companion window (allows keyboard / mouse input).
    pMainWindow = CreateMainWindow();
    EZ_ASSERT_DEV(pMainWindow != nullptr, "To change the main window creation behavior, override ezGameState::CreateActors().");
    pOutput = CreateMainOutputTarget(pMainWindow.Borrow());
    ConfigureMainWindowInputDevices(pMainWindow.Borrow());
    CreateMainView();
    SetupMainView(pOutput->m_hSwapChain, pMainWindow->GetClientAreaSize());
  }
  else
  {
    // XR Window (no companion window)
    CreateMainView();
    SetupMainView({}, {});
  }

  if (m_bXRRemotingEnabled)
  {
    if (pXRRemotingInterface->Connect(ezXRRemotingInterface::cvar_XrRemotingHostName.GetValue().GetData()).Failed())
    {
      ezLog::Error("Failed to connect XR Remoting.");
    }
  }

  ezView* pView = nullptr;
  EZ_VERIFY(ezRenderWorld::TryGetView(m_hMainView, pView), "");
  ezUniquePtr<ezActor> pXRActor = pXRInterface->CreateActor(pView, ezGALMSAASampleCount::Default, std::move(pMainWindow), std::move(pOutput));
  return std::move(pXRActor);
}

void ezGameState::CreateActors()
{
  EZ_LOG_BLOCK("CreateActors");
  ezUniquePtr<ezActor> pXRActor = CreateXRActor();
  if (pXRActor != nullptr)
  {
    ezActorManager::GetSingleton()->AddActor(std::move(pXRActor));
    return;
  }

  ezUniquePtr<ezWindow> pMainWindow = CreateMainWindow();
  EZ_ASSERT_DEV(pMainWindow != nullptr, "To change the main window creation behavior, override ezGameState::CreateActors().");
  ezUniquePtr<ezWindowOutputTargetGAL> pOutput = CreateMainOutputTarget(pMainWindow.Borrow());
  ConfigureMainWindowInputDevices(pMainWindow.Borrow());
  CreateMainView();
  SetupMainView(pOutput->m_hSwapChain, pMainWindow->GetClientAreaSize());

  {
    // Default flat window
    ezUniquePtr<ezActorPluginWindowOwner> pWindowPlugin = EZ_DEFAULT_NEW(ezActorPluginWindowOwner);
    pWindowPlugin->m_pWindow = std::move(pMainWindow);
    pWindowPlugin->m_pWindowOutputTarget = std::move(pOutput);
    ezUniquePtr<ezActor> pActor = EZ_DEFAULT_NEW(ezActor, "Main Window", this);
    pActor->AddPlugin(std::move(pWindowPlugin));
    ezActorManager::GetSingleton()->AddActor(std::move(pActor));
  }
}

void ezGameState::ConfigureMainWindowInputDevices(ezWindow* pWindow) {}

void ezGameState::ConfigureInputActions() {}

void ezGameState::SetupMainView(ezGALSwapChainHandle hSwapChain, ezSizeU32 viewportSize)
{
  ezView* pView = nullptr;
  if (!ezRenderWorld::TryGetView(m_hMainView, pView))
  {
    ezLog::Error("Main view is invalid, SetupMainView canceled.");
    return;
  }

  if (m_bXREnabled)
  {
    const ezXRConfig* pConfig = ezGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetTypeConfig<ezXRConfig>();

    auto renderPipeline = ezResourceManager::LoadResource<ezRenderPipelineResource>(pConfig->m_sXRRenderPipeline);
    pView->SetRenderPipelineResource(renderPipeline);
    // Render target setup is done by ezXRInterface::CreateActor
  }
  else
  {
    // Render target setup
    {
      const auto* pConfig = ezGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetTypeConfig<ezRenderPipelineProfileConfig>();
      auto renderPipeline = ezResourceManager::LoadResource<ezRenderPipelineResource>(pConfig->m_sMainRenderPipeline);
      pView->SetRenderPipelineResource(renderPipeline);
      pView->SetSwapChain(hSwapChain);
      pView->SetViewport(ezRectFloat(0.0f, 0.0f, (float)viewportSize.width, (float)viewportSize.height));
      pView->ForceUpdate();
    }
  }
}

ezView* ezGameState::CreateMainView()
{
  EZ_ASSERT_DEV(m_hMainView.IsInvalidated(), "CreateMainView was already called.");

  EZ_LOG_BLOCK("CreateMainView");
  ezView* pView = nullptr;
  m_hMainView = ezRenderWorld::CreateView("MainView", pView);
  pView->SetCameraUsageHint(ezCameraUsageHint::MainView);
  pView->SetWorld(m_pMainWorld);
  pView->SetCamera(&m_MainCamera);
  ezRenderWorld::AddMainView(m_hMainView);

  const ezTag& tagEditor = ezTagRegistry::GetGlobalRegistry().RegisterTag("Editor");
  // exclude all editor objects from rendering in proper game views
  pView->m_ExcludeTags.Set(tagEditor);
  return pView;
}

ezResult ezGameState::SpawnPlayer(ezStringView sStartPosition, const ezTransform& startPositionOffset)
{
  if (m_pMainWorld == nullptr)
    return EZ_FAILURE;

  EZ_LOCK(m_pMainWorld->GetWriteMarker());

  ezPlayerStartPointComponentManager* pMan = m_pMainWorld->GetComponentManager<ezPlayerStartPointComponentManager>();
  if (pMan == nullptr)
    return EZ_FAILURE;

  ezPlayerStartPointComponent* pBestComp = nullptr;

  for (auto it = pMan->GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActive() && it->GetPlayerPrefab().IsValid())
    {
      if (pBestComp == nullptr)
      {
        // take the first one, no matter what
        pBestComp = it;
      }
      else if (it->GetOwner()->GetName().IsEqual_NoCase(sStartPosition))
      {
        // if we find one by exact name match, take that one
        pBestComp = it;
      }
      else if (!pBestComp->GetOwner()->GetName().IsEqual_NoCase(sStartPosition) && it->GetOwner()->GetName().IsEmpty())
      {
        // if the name of the best one isn't identical to the searched name, yet
        // and this one is nameless, prefer the nameless one
        pBestComp = it;
      }
    }
  }

  if (pBestComp)
  {
    ezResourceLock<ezPrefabResource> pPrefab(pBestComp->GetPlayerPrefab(), ezResourceAcquireMode::BlockTillLoaded);

    if (pPrefab.GetAcquireResult() == ezResourceAcquireResult::Final)
    {
      const ezUInt16 uiTeamID = pBestComp->GetOwner()->GetTeamID();
      ezTransform startPos = ezTransform::MakeGlobalTransform(pBestComp->GetOwner()->GetGlobalTransform(), startPositionOffset);

      if (sStartPosition.IsEqual_NoCase("GlobalOverride"))
      {
        startPos = startPositionOffset;
      }

      startPos.m_vScale.Set(1.0f);

      ezPrefabInstantiationOptions options;
      options.m_pOverrideTeamID = &uiTeamID;

      pPrefab->InstantiatePrefab(*m_pMainWorld, startPos, options, &(pBestComp->m_Parameters));

      return EZ_SUCCESS;
    }
  }

  return EZ_FAILURE;
}

void ezGameState::ChangeMainWorld(ezWorld* pNewMainWorld, ezStringView sStartPosition, const ezTransform& startPositionOffset)
{
  if (m_pMainWorld == pNewMainWorld)
    return;

  ezWorld* pPrevWorld = m_pMainWorld;

  m_pMainWorld = pNewMainWorld;

  ezView* pView = nullptr;
  if (ezRenderWorld::TryGetView(m_hMainView, pView))
  {
    pView->SetWorld(m_pMainWorld);
  }

  OnChangedMainWorld(pPrevWorld, pNewMainWorld, sStartPosition, startPositionOffset);

  // make sure the camera gets re-initialized for the new world
  ConfigureMainCamera();
}

void ezGameState::OnChangedMainWorld(ezWorld* pPrevWorld, ezWorld* pNewWorld, ezStringView sStartPosition, const ezTransform& startPositionOffset)
{
  if (pNewWorld != m_pLoadingScreenWorld)
  {
    // can get rid of the loading screen world, or we could also keep it around for later, if that has any use
    m_pLoadingScreenWorld.Clear();

    SpawnPlayer(sStartPosition, startPositionOffset).IgnoreResult();
  }
}

void ezGameState::ConfigureMainCamera()
{
  if (m_MainCamera.GetCameraMode() == ezCameraMode::Stereo)
  {
    // if the camera is already set to be in 'Stereo' mode, its parameters are set from the outside
    return;
  }


  if (const ezWorld* pConstWorld = m_pMainWorld)
  {
    EZ_LOCK(pConstWorld->GetReadMarker());

    const ezCameraComponentManager* pManager = pConstWorld->GetComponentManager<ezCameraComponentManager>();
    if (pManager != nullptr)
    {
      for (auto itComp = pManager->GetComponents(); itComp.IsValid(); itComp.Next())
      {
        const ezCameraComponent* pComp = itComp;

        if (pComp->IsActive() && pComp->GetUsageHint() == ezCameraUsageHint::MainView)
        {
          ezVec3 vCameraPos = pComp->GetOwner()->GetGlobalPosition();

          ezCoordinateSystem coordSys;
          coordSys.m_vForwardDir = pComp->GetOwner()->GetGlobalDirForwards();
          coordSys.m_vRightDir = pComp->GetOwner()->GetGlobalDirRight();
          coordSys.m_vUpDir = pComp->GetOwner()->GetGlobalDirUp();

          // update the camera position
          // camera options (FOV etc) are already set by ezCameraComponentManager on demand
          m_MainCamera.LookAt(vCameraPos, vCameraPos + coordSys.m_vForwardDir, coordSys.m_vUpDir);
          return;
        }
      }
    }
  }
}

ezUniquePtr<ezWindow> ezGameState::CreateMainWindow()
{
  if (false)
  {
    ezHybridArray<ezScreenInfo, 2> screens;
    ezScreen::EnumerateScreens(screens).IgnoreResult();
    ezScreen::PrintScreenInfo(screens);
  }

  ezStringBuilder sWndCfg = opt_Window.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified);

  if (!sWndCfg.IsEmpty() && !ezFileSystem::ExistsFile(sWndCfg))
  {
    ezLog::Dev("Window Config file does not exist: '{0}'", sWndCfg);
    sWndCfg.Clear();
  }

  if (sWndCfg.IsEmpty())
  {
    const ezStringView sCfgAppData = ":appdata/RuntimeConfigs/Window.ddl";
    const ezStringView sCfgProject = ":project/RuntimeConfigs/Window.ddl";

    if (ezFileSystem::ExistsFile(sCfgAppData))
      sWndCfg = sCfgAppData;
    else
      sWndCfg = sCfgProject;
  }

  ezWindowCreationDesc wndDesc;
  wndDesc.LoadFromDDL(sWndCfg).IgnoreResult();

  ezUniquePtr<ezGameStateWindow> pWindow = EZ_DEFAULT_NEW(ezGameStateWindow, wndDesc, [] {});
  pWindow->ResetOnClickClose([this]()
    { this->RequestQuit(); });

  return pWindow;
}

ezUniquePtr<ezWindowOutputTargetGAL> ezGameState::CreateMainOutputTarget(ezWindow* pMainWindow)
{
  ezUniquePtr<ezWindowOutputTargetGAL> pOutput = EZ_DEFAULT_NEW(ezWindowOutputTargetGAL, [this](ezGALSwapChainHandle hSwapChain, ezSizeU32 size)
    { SetupMainView(hSwapChain, size); });

  ezGALWindowSwapChainCreationDescription desc;
  desc.m_pWindow = pMainWindow;
  desc.m_BackBufferFormat = ezGALResourceFormat::RGBAUByteNormalizedsRGB;

  pOutput->CreateSwapchain(desc);

  return pOutput;
}

ezString ezGameState::GetStartupSceneFile()
{
  return ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-scene");
}

void ezGameState::LoadScene(ezStringView sSceneFile, ezStringView sPreloadCollection, ezStringView sStartPosition, const ezTransform& startPositionOffset)
{
  m_sTargetSceneSpawnPoint = sStartPosition;
  m_TargetSceneSpawnOffset = startPositionOffset;

  StartBackgroundSceneLoading(sSceneFile, sPreloadCollection);
  m_bTransitionWhenReady = true;

  auto state = m_pBackgroundSceneLoad->GetLoadingState();
  EZ_ASSERT_DEBUG(state != ezSceneLoadUtility::LoadingState::FinishedAndRetrieved, "Scene already loaded and retrieved.");

  if (state != ezSceneLoadUtility::LoadingState::FinishedSuccessfully)
  {
    // switch to loading screen only if we can't immediately switch to the target scene
    SwitchToLoadingScreen(sSceneFile);
  }
}

void ezGameState::SwitchToLoadingScreen(ezStringView sTargetSceneFile)
{
  m_pLoadingScreenWorld = CreateLoadingScreenWorld(sTargetSceneFile);

  ChangeMainWorld(m_pLoadingScreenWorld.Borrow(), {}, ezTransform::MakeIdentity());
}

ezUniquePtr<ezWorld> ezGameState::CreateLoadingScreenWorld(ezStringView sTargetSceneFile)
{
  ezWorldDesc desc("LoadingScreen");
  return EZ_DEFAULT_NEW(ezWorld, desc);
}

void ezGameState::StartBackgroundSceneLoading(ezStringView sSceneFile, ezStringView sPreloadCollection)
{
  m_bTransitionWhenReady = false;

  if ((m_pBackgroundSceneLoad != nullptr) && (m_pBackgroundSceneLoad->GetRequestedScene() == sSceneFile))
  {
    // already being loaded
    return;
  }

  CancelBackgroundSceneLoading();

  m_pBackgroundSceneLoad = EZ_DEFAULT_NEW(ezSceneLoadUtility);
  m_pBackgroundSceneLoad->StartSceneLoading(sSceneFile, sPreloadCollection);
}

void ezGameState::CancelBackgroundSceneLoading()
{
  if (m_pBackgroundSceneLoad)
  {
    OnBackgroundSceneLoadingCanceled();
    m_pBackgroundSceneLoad.Clear();
  }
}

void ezGameState::UpdateBackgroundSceneLoading()
{
  if (m_pBackgroundSceneLoad)
  {
    ezSceneLoadUtility::LoadingState state = m_pBackgroundSceneLoad->GetLoadingState();

    switch (state)
    {
      case ezSceneLoadUtility::LoadingState::FinishedAndRetrieved:
        return;

      case ezSceneLoadUtility::LoadingState::NotStarted:
      case ezSceneLoadUtility::LoadingState::Ongoing:
        m_pBackgroundSceneLoad->TickSceneLoading();
        break;

      case ezSceneLoadUtility::LoadingState::FinishedSuccessfully:
        if (m_bTransitionWhenReady)
        {
          OnBackgroundSceneLoadingFinished(m_pBackgroundSceneLoad->RetrieveLoadedScene());
          m_pBackgroundSceneLoad.Clear();
        }
        break;

      case ezSceneLoadUtility::LoadingState::Failed:
        OnBackgroundSceneLoadingFailed(m_pBackgroundSceneLoad->GetLoadingFailureReason());
        m_pBackgroundSceneLoad.Clear();
        break;
    }
  }
}

void ezGameState::OnBackgroundSceneLoadingFinished(ezUniquePtr<ezWorld>&& pWorld)
{
  ezLog::Success("Finished loading scene '{}'.", m_pBackgroundSceneLoad->GetRequestedScene());

  m_pLoadedWorld = std::move(pWorld);
  ChangeMainWorld(m_pLoadedWorld.Borrow(), m_sTargetSceneSpawnPoint, m_TargetSceneSpawnOffset);
  m_sTargetSceneSpawnPoint.Clear();
  m_TargetSceneSpawnOffset = ezTransform::MakeIdentity();
}

void ezGameState::OnBackgroundSceneLoadingFailed(ezStringView sReason)
{
  ezLog::Error("Scene loading failed: {}", sReason);
}

void ezGameState::OnBackgroundSceneLoadingCanceled()
{
  ezLog::Dev("Cancelled background loading of scene '{}'.", m_pBackgroundSceneLoad->GetRequestedScene());
}
