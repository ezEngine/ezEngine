#include <GameEnginePCH.h>

#include <Core/Input/InputManager.h>
#include <GameEngine/Configuration/InputConfig.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <GameEngine/Gameplay/PlayerStartPointComponent.h>
#include <RendererCore/Components/CameraComponent.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFallbackGameState, 1, ezRTTIDefaultAllocator<ezFallbackGameState>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezFallbackGameState::ezFallbackGameState()
{
  m_iActiveCameraComponentIndex = -3;
}



ezGameStatePriority ezFallbackGameState::DeterminePriority(ezWorld* pWorld) const
{
  if (pWorld == nullptr)
    return 

ezGameStatePriority::None;

  return 

ezGameStatePriority::Fallback;
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

static void RegisterInputAction(const char* szInputSet, const char* szInputAction, const char* szKey1, const char* szKey2 = nullptr,
                                const char* szKey3 = nullptr)
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

  // if ( !ezFileSystem::ExistsFile( ":project/InputConfig.ddl" ) )
  {
    RegisterInputAction("Game", "MoveForwards", ezInputSlot_KeyW);
    RegisterInputAction("Game", "MoveBackwards", ezInputSlot_KeyS);
    RegisterInputAction("Game", "MoveLeft", ezInputSlot_KeyA);
    RegisterInputAction("Game", "MoveRight", ezInputSlot_KeyD);
    RegisterInputAction("Game", "MoveUp", ezInputSlot_KeyQ);
    RegisterInputAction("Game", "MoveDown", ezInputSlot_KeyE);
    RegisterInputAction("Game", "Run", ezInputSlot_KeyLeftShift);

    RegisterInputAction("Game", "TurnLeft", ezInputSlot_MouseMoveNegX, ezInputSlot_KeyLeft);
    RegisterInputAction("Game", "TurnRight", ezInputSlot_MouseMovePosX, ezInputSlot_KeyRight);
    RegisterInputAction("Game", "TurnUp", ezInputSlot_MouseMoveNegY, ezInputSlot_KeyUp);
    RegisterInputAction("Game", "TurnDown", ezInputSlot_MouseMovePosY, ezInputSlot_KeyDown);

    RegisterInputAction("Game", "NextCamera", ezInputSlot_KeyPageDown);
    RegisterInputAction("Game", "PrevCamera", ezInputSlot_KeyPageUp);

    // if ( !g_AllInput.IsEmpty() )
    //{
    //	ezFileWriter file;
    //	if ( file.Open( ":project/InputConfig.ddl" ).Succeeded() )
    //	{
    //		ezGameAppInputConfig::WriteToDDL( file, g_AllInput );
    //	}
    //}
  }
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
    Cameras[pComp->GetUsageHint().GetValue()].PushBack(pComp);

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

void ezFallbackGameState::AfterWorldUpdate()
{
  EZ_LOCK(m_pMainWorld->GetReadMarker());

  // Update the camera transform after world update so the owner node has its final position for this frame.
  // Setting the camera transform in ProcessInput introduces one frame delay.
  if (const ezCameraComponent* pCamComp = FindActiveCameraComponent())
  {
    const ezGameObject* pOwner = pCamComp->GetOwner();
    ezVec3 vPosition = pOwner->GetGlobalPosition();
    ezVec3 vForward = pOwner->GetGlobalDirForwards();
    ezVec3 vUp = pOwner->GetGlobalDirUp();

    m_MainCamera.LookAt(vPosition, vPosition + vForward, vUp);
  }
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_GameState_Implementation_FallbackGameState);

