
#include <GameEnginePCH.h>

#include <Core/ActorSystem/Actor.h>
#include <Core/ActorSystem/ActorManager.h>
#include <Core/ActorSystem/ActorPluginWindow.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/System/Screen.h>
#include <GameApplication/WindowOutputTarget.h>
#include <GameEngine/Configuration/RendererProfileConfigs.h>
#include <GameEngine/Configuration/XRConfig.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/GameStateWindow.h>
#include <GameEngine/Gameplay/PlayerStartPointComponent.h>
#include <GameEngine/XR/XRInterface.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGameState, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_STATICLINK_FILE(GameEngine, GameEngine_GameState_Implementation_GameState);
// clang-format on

ezGameState::ezGameState() {}

ezGameState::~ezGameState() {}

void ezGameState::OnActivation(ezWorld* pWorld, const ezTransform* pStartPosition)
{
  m_pMainWorld = pWorld;
  {
    ConfigureMainCamera();

    CreateActors();
  }

  ConfigureInputActions();

  SpawnPlayer(pStartPosition);
}

void ezGameState::OnDeactivation()
{
  if (m_bXREnabled)
  {
    m_bXREnabled = false;
    ezXRInterface* pXRInterface = ezSingletonRegistry::GetSingletonInstance<ezXRInterface>();
    ezActorManager::GetSingleton()->DestroyAllActors(pXRInterface);
    pXRInterface->Deinitialize();
  }

  ezRenderWorld::DeleteView(m_hMainView);
}

void ezGameState::ScheduleRendering()
{
  ezRenderWorld::AddMainView(m_hMainView);
}

void ezGameState::CreateActors()
{
  EZ_LOG_BLOCK("CreateActors");

  // Init XR
  const ezXRConfig* pConfig = ezGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetTypeConfig<ezXRConfig>();
  ezXRInterface* pXRInterface = nullptr;
  if (pConfig && pConfig->m_bEnableXR)
  {
    if (ezXRInterface* pXR = ezSingletonRegistry::GetSingletonInstance<ezXRInterface>())
    {
      if (pXR->Initialize().Succeeded())
      {
        pXRInterface = pXR;
        m_bXREnabled = true;
      }
      else
      {
        ezLog::Error("ezXRInterface could not be initialized. Make sure the XR plugin runtime is installed.");
      }
    }
    else
    {
      ezLog::Error("No ezXRInterface interface found. Please load a XR plugin to enable XR.");
    }
  }

  if (m_bXREnabled && !pXRInterface->SupportsCompanionView())
  {
    // XR Window (no companion window)
    SetupMainView(nullptr, {});
    ezView* pView = nullptr;
    EZ_VERIFY(ezRenderWorld::TryGetView(m_hMainView, pView), "");
    ezUniquePtr<ezActor> pXRActor = pXRInterface->CreateActor(pView, ezGALMSAASampleCount::Default);
    ezActorManager::GetSingleton()->AddActor(std::move(pXRActor));
  }
  else
  {
    ezUniquePtr<ezWindow> pMainWindow = CreateMainWindow();
    EZ_ASSERT_DEV(pMainWindow != nullptr, "To change the main window creation behavior, override ezGameState::CreateActors().");
    ezUniquePtr<ezWindowOutputTargetBase> pOutput = CreateMainOutputTarget(pMainWindow.Borrow());
    ConfigureMainWindowInputDevices(pMainWindow.Borrow());
    SetupMainView(pOutput.Borrow(), pMainWindow->GetClientAreaSize());

    if (m_bXREnabled)
    {
      // XR Window with added companion window (allows keyboard / mouse input).
      ezView* pView = nullptr;
      EZ_VERIFY(ezRenderWorld::TryGetView(m_hMainView, pView), "");
      ezUniquePtr<ezActor> pXRActor = pXRInterface->CreateActor(pView, ezGALMSAASampleCount::Default, std::move(pMainWindow), std::move(pOutput));
      ezActorManager::GetSingleton()->AddActor(std::move(pXRActor));
    }
    else
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
}

void ezGameState::ConfigureMainWindowInputDevices(ezWindow* pWindow) {}

void ezGameState::ConfigureInputActions() {}

void ezGameState::SetupMainView(ezWindowOutputTargetBase* pOutputTarget, ezSizeU32 viewportSize)
{
  if (m_bXREnabled)
  {
    const ezXRConfig* pConfig = ezGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetTypeConfig<ezXRConfig>();
    ezXRInterface* pXRInterface = ezSingletonRegistry::GetSingletonInstance<ezXRInterface>();

    auto renderPipeline = ezResourceManager::LoadResource<ezRenderPipelineResource>(pConfig->m_sXRRenderPipeline);
    CreateMainView(renderPipeline);
    // Render target setup is done by ezXRInterface::CreateActor
  }
  else
  {
    const auto* pConfig = ezGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetTypeConfig<ezRenderPipelineProfileConfig>();
    auto renderPipeline = ezResourceManager::LoadResource<ezRenderPipelineResource>(pConfig->m_sMainRenderPipeline);
    ezView* pView = CreateMainView(renderPipeline);

    // Render target setup
    {
      ezWindowOutputTargetGAL* pOutputGAL = static_cast<ezWindowOutputTargetGAL*>(pOutputTarget);
      const ezGALSwapChain* pSwapChain = ezGALDevice::GetDefaultDevice()->GetSwapChain(pOutputGAL->m_hSwapChain);
      ezGALRenderTargetViewHandle hBackBuffer = ezGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(pSwapChain->GetBackBufferTexture());
      ezGALRenderTargetSetup renderTargetSetup;
      renderTargetSetup.SetRenderTarget(0, hBackBuffer);
      pView->SetRenderTargetSetup(renderTargetSetup);
      pView->SetViewport(ezRectFloat(0.0f, 0.0f, (float)viewportSize.width, (float)viewportSize.height));
    }
  }
}

ezView* ezGameState::CreateMainView(ezTypedResourceHandle<ezRenderPipelineResource> hRenderPipeline)
{
  EZ_LOG_BLOCK("CreateMainView");
  ezView* pView = nullptr;
  m_hMainView = ezRenderWorld::CreateView("MainView", pView);
  pView->SetCameraUsageHint(ezCameraUsageHint::MainView);
  pView->SetRenderPipelineResource(hRenderPipeline);
  pView->SetWorld(m_pMainWorld);
  pView->SetCamera(&m_MainCamera);
  ezRenderWorld::AddMainView(m_hMainView);

  const ezTag& tagEditor = ezTagRegistry::GetGlobalRegistry().RegisterTag("Editor");
  // exclude all editor objects from rendering in proper game views
  pView->m_ExcludeTags.Set(tagEditor);
  return pView;
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
    if (it->IsActive() && it->GetPlayerPrefab().IsValid())
    {
      ezResourceLock<ezPrefabResource> pPrefab(it->GetPlayerPrefab(), ezResourceAcquireMode::BlockTillLoaded);

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

        pPrefab->InstantiatePrefab(*m_pMainWorld, startPos, ezGameObjectHandle(), nullptr, &uiTeamID, &(it->m_Parameters), false);

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

  if (m_bXREnabled)
  {
    wndDesc.m_bClipMouseCursor = false;
    wndDesc.m_bShowMouseCursor = true;
    wndDesc.m_WindowMode = ezWindowMode::WindowResizable;
  }

  ezUniquePtr<ezGameStateWindow> pWindow = EZ_DEFAULT_NEW(ezGameStateWindow, wndDesc, [] {});
  pWindow->ResetOnClickClose([]() { ezGameApplicationBase::GetGameApplicationBaseInstance()->RequestQuit(); });
  if (pWindow->GetInputDevice())
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
