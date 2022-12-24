#include <GameEngine/GameEnginePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/Collection/CollectionResource.h>
#include <Core/Input/InputManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GameEngine/Configuration/InputConfig.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <GameEngine/Gameplay/PlayerStartPointComponent.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFallbackGameState, 1, ezRTTIDefaultAllocator<ezFallbackGameState>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezFallbackGameState::ezFallbackGameState()
{
  m_iActiveCameraComponentIndex = -3;
}

void ezFallbackGameState::EnableSceneSelectionMenu(bool bEnable)
{
  m_bEnableSceneSelectionMenu = bEnable;
}

void ezFallbackGameState::EnableFreeCameras(bool bEnable)
{
  m_bEnableFreeCameras = bEnable;
}

void ezFallbackGameState::EnableAutoSwitchToLoadedScene(bool bEnable)
{
  m_bAutoSwitchToLoadedScene = bEnable;
}

ezGameStatePriority ezFallbackGameState::DeterminePriority(ezWorld* pWorld) const
{
  return ezGameStatePriority::Fallback;
}

void ezFallbackGameState::OnActivation(ezWorld* pWorld, const ezTransform* pStartPosition)
{
  SUPER::OnActivation(pWorld, pStartPosition);

  // if we already have a scene (editor use case), just use that and don't create any other world
  if (pWorld != nullptr)
    return;

  // otherwise we need to load a scene

  SwitchToLoadingScreen();

  if (!ezFileSystem::ExistsFile(":project/ezProject"))
  {
    m_bShowMenu = true;

    if (ezCommandLineUtils::GetGlobalInstance()->HasOption("-project"))
      m_State = State::BadProject;
    else
      m_State = State::NoProject;
  }
  else
  {
    ezStringBuilder sScenePath = GetStartupSceneFile();
    sScenePath.MakeCleanPath();

    if (sScenePath.IsEmpty())
    {
      m_bShowMenu = true;
      m_State = State::NoScene;
    }
    else if (StartSceneLoading(sScenePath, {}).Failed())
    {
      m_bShowMenu = true;
      m_State = State::BadScene;
    }
  }
}

void ezFallbackGameState::OnDeactivation()
{
  CancelSceneLoading();

  SUPER::OnDeactivation();
}

ezString ezFallbackGameState::GetStartupSceneFile()
{
  return ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-scene");
}

void ezFallbackGameState::SwitchToLoadingScreen()
{
  m_sTitleOfActiveScene = "Loading Screen";
  m_bIsInLoadingScreen = true;

  m_pActiveWorld = std::move(CreateLoadingScreenWorld());
  ChangeMainWorld(m_pActiveWorld.Borrow());
}

ezUniquePtr<ezWorld> ezFallbackGameState::CreateLoadingScreenWorld()
{
  ezWorldDesc desc("LoadingScreen");

  return EZ_DEFAULT_NEW(ezWorld, desc);
}

ezResult ezFallbackGameState::StartSceneLoading(ezStringView sSceneFile, ezStringView sPreloadCollection)
{
  if (m_pSceneToLoad != nullptr && m_sTitleOfLoadingScene == sSceneFile)
  {
    // already being loaded
    return EZ_SUCCESS;
  }

  m_sTitleOfLoadingScene = sSceneFile;

  m_pSceneToLoad = EZ_DEFAULT_NEW(ezSceneLoadUtility);
  m_pSceneToLoad->StartSceneLoading(sSceneFile, sPreloadCollection);

  if (m_pSceneToLoad->GetLoadingState() == ezSceneLoadUtility::LoadingState::Failed)
  {
    ezLog::Error("Scene loading failed: {}", m_pSceneToLoad->GetLoadingFailureReason());
    CancelSceneLoading();
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

void ezFallbackGameState::CancelSceneLoading()
{
  m_sTitleOfLoadingScene.Clear();
  m_pSceneToLoad.Clear();
}

bool ezFallbackGameState::IsLoadingScene() const
{
  return m_pSceneToLoad != nullptr;
}

void ezFallbackGameState::SwitchToLoadedScene()
{
  EZ_ASSERT_DEV(IsLoadingScene(), "Can't switch to loaded scene, if no scene is currently being loaded.");
  EZ_ASSERT_DEV(m_pSceneToLoad->GetLoadingState() == ezSceneLoadUtility::LoadingState::FinishedSuccessfully, "Can't switch to loaded scene before it has finished loading.");

  m_State = State::Ok;
  m_sTitleOfActiveScene = m_sTitleOfLoadingScene;
  m_pActiveWorld = m_pSceneToLoad->RetrieveLoadedScene();
  ChangeMainWorld(m_pActiveWorld.Borrow());
  SpawnPlayer(nullptr).IgnoreResult();

  CancelSceneLoading();

  m_bIsInLoadingScreen = false;
}

ezResult ezFallbackGameState::SpawnPlayer(const ezTransform* pStartPosition)
{
  if (SUPER::SpawnPlayer(pStartPosition).Succeeded())
    return EZ_SUCCESS;

  if (m_pMainWorld && pStartPosition)
  {
    m_iActiveCameraComponentIndex = -1; // set free camera
    m_MainCamera.LookAt(pStartPosition->m_vPosition, pStartPosition->m_vPosition + pStartPosition->m_qRotation * ezVec3(1, 0, 0),
      pStartPosition->m_qRotation * ezVec3(0, 0, 1));
  }

  return EZ_FAILURE;
}

static ezHybridArray<ezGameAppInputConfig, 16> g_AllInput;

static void RegisterInputAction(const char* szInputSet, const char* szInputAction, const char* szKey1, const char* szKey2 = nullptr, const char* szKey3 = nullptr)
{
  ezGameAppInputConfig& gacfg = g_AllInput.ExpandAndGetRef();
  gacfg.m_sInputSet = szInputSet;
  gacfg.m_sInputAction = szInputAction;
  gacfg.m_sInputSlotTrigger[0] = szKey1;
  gacfg.m_sInputSlotTrigger[1] = szKey2;
  gacfg.m_sInputSlotTrigger[2] = szKey3;
  gacfg.m_bApplyTimeScaling = true;

  ezInputActionConfig cfg;

  cfg = ezInputManager::GetInputActionConfig(szInputSet, szInputAction);
  cfg.m_bApplyTimeScaling = true;

  if (szKey1 != nullptr)
    cfg.m_sInputSlotTrigger[0] = szKey1;
  if (szKey2 != nullptr)
    cfg.m_sInputSlotTrigger[1] = szKey2;
  if (szKey3 != nullptr)
    cfg.m_sInputSlotTrigger[2] = szKey3;

  ezInputManager::SetInputActionConfig(szInputSet, szInputAction, cfg, true);
}

void ezFallbackGameState::ConfigureInputActions()
{
  g_AllInput.Clear();

  RegisterInputAction("Game", "MoveForwards", ezInputSlot_KeyW);
  RegisterInputAction("Game", "MoveBackwards", ezInputSlot_KeyS);
  RegisterInputAction("Game", "MoveLeft", ezInputSlot_KeyA);
  RegisterInputAction("Game", "MoveRight", ezInputSlot_KeyD);
  RegisterInputAction("Game", "MoveUp", ezInputSlot_KeyE);
  RegisterInputAction("Game", "MoveDown", ezInputSlot_KeyQ);
  RegisterInputAction("Game", "Run", ezInputSlot_KeyLeftShift);

  RegisterInputAction("Game", "TurnLeft", ezInputSlot_MouseMoveNegX, ezInputSlot_KeyLeft);
  RegisterInputAction("Game", "TurnRight", ezInputSlot_MouseMovePosX, ezInputSlot_KeyRight);
  RegisterInputAction("Game", "TurnUp", ezInputSlot_MouseMoveNegY, ezInputSlot_KeyUp);
  RegisterInputAction("Game", "TurnDown", ezInputSlot_MouseMovePosY, ezInputSlot_KeyDown);

  RegisterInputAction("Game", "NextCamera", ezInputSlot_KeyPageDown);
  RegisterInputAction("Game", "PrevCamera", ezInputSlot_KeyPageUp);
}

const ezCameraComponent* ezFallbackGameState::FindActiveCameraComponent()
{
  if (m_iActiveCameraComponentIndex == -1)
    return nullptr;

  const ezWorld* pWorld = m_pMainWorld;
  const ezCameraComponentManager* pManager = pWorld->GetComponentManager<ezCameraComponentManager>();
  if (pManager == nullptr)
    return nullptr;

  auto itComp = pManager->GetComponents();

  ezHybridArray<const ezCameraComponent*, 32> Cameras[ezCameraUsageHint::ENUM_COUNT];

  // first find all cameras and sort them by usage type
  while (itComp.IsValid())
  {
    const ezCameraComponent* pComp = itComp;

    if (pComp->IsActive())
    {
      Cameras[pComp->GetUsageHint().GetValue()].PushBack(pComp);
    }

    itComp.Next();
  }

  Cameras[ezCameraUsageHint::None].Clear();
  Cameras[ezCameraUsageHint::RenderTarget].Clear();
  Cameras[ezCameraUsageHint::Culling].Clear();
  Cameras[ezCameraUsageHint::Shadow].Clear();
  Cameras[ezCameraUsageHint::Thumbnail].Clear();

  if (m_iActiveCameraComponentIndex == -3)
  {
    // take first camera of a good usage type
    m_iActiveCameraComponentIndex = 0;
  }

  // take last camera (wrap around)
  if (m_iActiveCameraComponentIndex == -2)
  {
    m_iActiveCameraComponentIndex = 0;
    for (ezUInt32 i = 0; i < ezCameraUsageHint::ENUM_COUNT; ++i)
    {
      m_iActiveCameraComponentIndex += Cameras[i].GetCount();
    }

    --m_iActiveCameraComponentIndex;
  }

  if (m_iActiveCameraComponentIndex >= 0)
  {
    ezInt32 offset = m_iActiveCameraComponentIndex;

    // now find the camera by that index
    for (ezUInt32 i = 0; i < ezCameraUsageHint::ENUM_COUNT; ++i)
    {
      if (offset < (ezInt32)Cameras[i].GetCount())
        return Cameras[i][offset];

      offset -= Cameras[i].GetCount();
    }
  }

  // on overflow, reset to free camera
  m_iActiveCameraComponentIndex = -1;
  return nullptr;
}

void ezFallbackGameState::ProcessInput()
{
  if (IsLoadingScene())
  {
    m_pSceneToLoad->TickSceneLoading();

    switch (m_pSceneToLoad->GetLoadingState())
    {
      case ezSceneLoadUtility::LoadingState::FinishedSuccessfully:
        if (m_bAutoSwitchToLoadedScene)
        {
          SwitchToLoadedScene();
        }
        break;

      case ezSceneLoadUtility::LoadingState::Failed:
        ezLog::Error("Scene loading failed: {}", m_pSceneToLoad->GetLoadingFailureReason());
        CancelSceneLoading();
        break;

      default:
        break;
    }
  }

  if (m_bEnableSceneSelectionMenu)
  {
    if (ezStringUtils::IsNullOrEmpty(ezInputManager::GetExclusiveInputSet()) ||
        ezStringUtils::IsEqual(ezInputManager::GetExclusiveInputSet(), "ezPlayer"))
    {
      if (DisplayMenu())
      {
        // prevents the currently active scene from getting any input
        ezInputManager::SetExclusiveInputSet("ezPlayer");
      }
      else
      {
        // allows the active scene to retrieve input again
        ezInputManager::SetExclusiveInputSet("");
      }
    }
  }

  if (m_bEnableFreeCameras)
  {
    EZ_LOCK(m_pMainWorld->GetReadMarker());

    if (ezInputManager::GetInputActionState("Game", "NextCamera") == ezKeyState::Pressed)
      ++m_iActiveCameraComponentIndex;
    if (ezInputManager::GetInputActionState("Game", "PrevCamera") == ezKeyState::Pressed)
      --m_iActiveCameraComponentIndex;

    const ezCameraComponent* pCamComp = FindActiveCameraComponent();
    if (pCamComp)
    {
      return;
    }

    float fRotateSpeed = 180.0f;
    float fMoveSpeed = 10.0f;
    float fInput = 0.0f;

    if (ezInputManager::GetInputActionState("Game", "Run", &fInput) != ezKeyState::Up)
      fMoveSpeed *= 10.0f;

    if (ezInputManager::GetInputActionState("Game", "MoveForwards", &fInput) != ezKeyState::Up)
      m_MainCamera.MoveLocally(fInput * fMoveSpeed, 0, 0);
    if (ezInputManager::GetInputActionState("Game", "MoveBackwards", &fInput) != ezKeyState::Up)
      m_MainCamera.MoveLocally(-fInput * fMoveSpeed, 0, 0);
    if (ezInputManager::GetInputActionState("Game", "MoveLeft", &fInput) != ezKeyState::Up)
      m_MainCamera.MoveLocally(0, -fInput * fMoveSpeed, 0);
    if (ezInputManager::GetInputActionState("Game", "MoveRight", &fInput) != ezKeyState::Up)
      m_MainCamera.MoveLocally(0, fInput * fMoveSpeed, 0);

    if (ezInputManager::GetInputActionState("Game", "MoveUp", &fInput) != ezKeyState::Up)
      m_MainCamera.MoveGlobally(0, 0, fInput * fMoveSpeed);
    if (ezInputManager::GetInputActionState("Game", "MoveDown", &fInput) != ezKeyState::Up)
      m_MainCamera.MoveGlobally(0, 0, -fInput * fMoveSpeed);

    if (ezInputManager::GetInputActionState("Game", "TurnLeft", &fInput) != ezKeyState::Up)
      m_MainCamera.RotateGlobally(ezAngle(), ezAngle(), ezAngle::Degree(-fRotateSpeed * fInput));
    if (ezInputManager::GetInputActionState("Game", "TurnRight", &fInput) != ezKeyState::Up)
      m_MainCamera.RotateGlobally(ezAngle(), ezAngle(), ezAngle::Degree(fRotateSpeed * fInput));
    if (ezInputManager::GetInputActionState("Game", "TurnUp", &fInput) != ezKeyState::Up)
      m_MainCamera.RotateLocally(ezAngle(), ezAngle::Degree(fRotateSpeed * fInput), ezAngle());
    if (ezInputManager::GetInputActionState("Game", "TurnDown", &fInput) != ezKeyState::Up)
      m_MainCamera.RotateLocally(ezAngle(), ezAngle::Degree(-fRotateSpeed * fInput), ezAngle());
  }
}

void ezFallbackGameState::AfterWorldUpdate()
{
  EZ_LOCK(m_pMainWorld->GetReadMarker());

  // Update the camera transform after world update so the owner node has its final position for this frame.
  // Setting the camera transform in ProcessInput introduces one frame delay.
  if (const ezCameraComponent* pCamComp = FindActiveCameraComponent())
  {
    if (pCamComp->GetCameraMode() != ezCameraMode::Stereo)
    {
      const ezGameObject* pOwner = pCamComp->GetOwner();
      ezVec3 vPosition = pOwner->GetGlobalPosition();
      ezVec3 vForward = pOwner->GetGlobalDirForwards();
      ezVec3 vUp = pOwner->GetGlobalDirUp();

      m_MainCamera.LookAt(vPosition, vPosition + vForward, vUp);
    }
  }
}

void ezFallbackGameState::FindAvailableScenes()
{
  if (m_bCheckedForScenes)
    return;

  m_bCheckedForScenes = true;

  if (!ezFileSystem::ExistsFile(":project/ezProject"))
    return;

#if EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS)
  ezFileSystemIterator fsit;
  ezStringBuilder sScenePath;

  for (ezFileSystem::StartSearch(fsit, "", ezFileSystemIteratorFlags::ReportFilesRecursive);
       fsit.IsValid(); fsit.Next())
  {
    fsit.GetStats().GetFullPath(sScenePath);

    if (!sScenePath.HasExtension(".ezScene"))
      continue;

    sScenePath.MakeRelativeTo(fsit.GetCurrentSearchTerm()).AssertSuccess();

    m_AvailableScenes.PushBack(sScenePath);
  }
#endif
}

bool ezFallbackGameState::DisplayMenu()
{
  if (IsLoadingScene() || m_pMainWorld == nullptr)
    return false;

  auto pWorld = m_pMainWorld;

  if (m_State == State::NoProject)
  {
    ezDebugRenderer::DrawInfoText(pWorld, ezDebugRenderer::ScreenPlacement::TopCenter, "_Player", "No project path provided.\n\nUse the command-line argument\n-project \"Path/To/ezProject\"\nto tell ezPlayer which project to load.\n\nWith the argument\n-scene \"Path/To/Scene.ezScene\"\nyou can also directly load a specific scene.\n\nPress ESC to quit.", ezColor::Red);

    return false;
  }

  if (m_State == State::BadProject)
  {
    ezDebugRenderer::DrawInfoText(pWorld, ezDebugRenderer::ScreenPlacement::TopCenter, "_Player", ezFmt("Invalid project path provided.\nThe given project directory does not exist:\n\n{}\n\nPress ESC to quit.", ezGameApplication::GetGameApplicationInstance()->GetAppProjectPath()), ezColor::Red);

    return false;
  }

  if (ezInputManager::GetInputSlotState(ezInputSlot_KeyLeftWin) == ezKeyState::Pressed || ezInputManager::GetInputSlotState(ezInputSlot_KeyRightWin) == ezKeyState::Pressed)
  {
    m_bShowMenu = !m_bShowMenu;
  }

  if (m_State == State::Ok && !m_bShowMenu)
    return false;

  ezDebugRenderer::DrawInfoText(pWorld, ezDebugRenderer::ScreenPlacement::TopCenter, "_Player", ezFmt("Project: '{}'", ezGameApplication::GetGameApplicationInstance()->GetAppProjectPath()), ezColor::White);

  if (m_State == State::NoScene)
  {
    ezDebugRenderer::DrawInfoText(pWorld, ezDebugRenderer::ScreenPlacement::TopCenter, "_Player", "No scene path provided.\n\nUse the command-line argument\n-scene \"Path/To/Scene.ezScene\"\nto directly load a specific scene.", ezColor::Orange);
  }
  else if (m_State == State::BadScene)
  {
    ezDebugRenderer::DrawInfoText(pWorld, ezDebugRenderer::ScreenPlacement::TopCenter, "_Player", ezFmt("Failed to load scene: '{}'", m_sTitleOfLoadingScene), ezColor::Red);
  }
  else
  {
    ezDebugRenderer::DrawInfoText(pWorld, ezDebugRenderer::ScreenPlacement::TopCenter, "_Player", ezFmt("Scene: '{}'", m_sTitleOfActiveScene), ezColor::White);
  }

  if (m_bShowMenu)
  {
    FindAvailableScenes();

    ezDebugRenderer::DrawInfoText(pWorld, ezDebugRenderer::ScreenPlacement::TopCenter, "_Player", "\nSelect scene:\n", ezColor::White);

    for (ezUInt32 i = 0; i < m_AvailableScenes.GetCount(); ++i)
    {
      const auto& file = m_AvailableScenes[i];

      if (i == m_uiSelectedScene)
      {
        ezDebugRenderer::DrawInfoText(pWorld, ezDebugRenderer::ScreenPlacement::TopCenter, "_Player", ezFmt("> {} <", file), ezColor::Gold);
      }
      else
      {
        ezDebugRenderer::DrawInfoText(pWorld, ezDebugRenderer::ScreenPlacement::TopCenter, "_Player", ezFmt("  {}  ", file), ezColor::GhostWhite);
      }
    }

    ezDebugRenderer::DrawInfoText(pWorld, ezDebugRenderer::ScreenPlacement::TopCenter, "_Player", "\nPress 'Return' to load scene.\nPress the 'Windows' key to toggle this menu.", ezColor::White);

    if (ezInputManager::GetInputSlotState(ezInputSlot_KeyEscape) == ezKeyState::Pressed)
    {
      m_bShowMenu = false;
    }
    else if (!m_AvailableScenes.IsEmpty())
    {
      if (ezInputManager::GetInputSlotState(ezInputSlot_KeyUp) == ezKeyState::Pressed)
      {
        if (m_uiSelectedScene == 0)
          m_uiSelectedScene = m_AvailableScenes.GetCount() - 1;
        else
          --m_uiSelectedScene;
      }

      if (ezInputManager::GetInputSlotState(ezInputSlot_KeyDown) == ezKeyState::Pressed)
      {
        if (m_uiSelectedScene == m_AvailableScenes.GetCount() - 1)
          m_uiSelectedScene = 0;
        else
          ++m_uiSelectedScene;
      }

      if (ezInputManager::GetInputSlotState(ezInputSlot_KeyReturn) == ezKeyState::Pressed || ezInputManager::GetInputSlotState(ezInputSlot_KeyNumpadEnter) == ezKeyState::Pressed)
      {
        if (StartSceneLoading(m_AvailableScenes[m_uiSelectedScene], {}).Succeeded())
        {
          m_bShowMenu = false;
        }
        else
        {
          m_bShowMenu = true;
          m_State = State::BadScene;
        }
      }

      return true;
    }
  }

  return false;
}

bool ezFallbackGameState::IsInLoadingScreen() const
{
  return m_bIsInLoadingScreen;
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_GameState_Implementation_FallbackGameState);
