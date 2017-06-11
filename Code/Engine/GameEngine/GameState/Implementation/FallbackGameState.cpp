#include <PCH.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <GameEngine/GameApplication/InputConfig.h>
#include <RendererCore/Components/CameraComponent.h>
#include <Core/Input/InputManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFallbackGameState, 1, ezRTTIDefaultAllocator<ezFallbackGameState>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezFallbackGameState::ezFallbackGameState()
{
  m_iActiveCameraComponentIndex = -3;
}

float ezFallbackGameState::CanHandleThis(ezGameApplicationType AppType, ezWorld* pWorld) const
{
  if (pWorld == nullptr)
    return -1.0f;

  return 0.0f;
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

  if (szKey1 != nullptr)     cfg.m_sInputSlotTrigger[0] = szKey1;
  if (szKey2 != nullptr)     cfg.m_sInputSlotTrigger[1] = szKey2;
  if (szKey3 != nullptr)     cfg.m_sInputSlotTrigger[2] = szKey3;

  ezInputManager::SetInputActionConfig(szInputSet, szInputAction, cfg, true);
}

void ezFallbackGameState::ConfigureInputActions()
{
  g_AllInput.Clear();

  //if ( !ezFileSystem::ExistsFile( ":project/InputConfig.ddl" ) )
  {
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

    RegisterInputAction("Game", "TurnLeft", ezInputSlot_MouseMoveNegX);
    RegisterInputAction("Game", "TurnRight", ezInputSlot_MouseMovePosX);
    RegisterInputAction("Game", "TurnUp", ezInputSlot_MouseMoveNegY);
    RegisterInputAction("Game", "TurnDown", ezInputSlot_MouseMovePosY);


    RegisterInputAction("Game", "NextCamera", ezInputSlot_KeyPageDown);
    RegisterInputAction("Game", "PrevCamera", ezInputSlot_KeyPageUp);

    //if ( !g_AllInput.IsEmpty() )
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

  // take first camera that is not of usage type 'None'
  if (m_iActiveCameraComponentIndex == -3)
  {
    // skip cameras of usage 'None' for now
    m_iActiveCameraComponentIndex = Cameras[0].GetCount(); // this skips all cameras of type 'None'
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
    m_MainCamera.MoveGlobally(ezVec3(0, 0, fInput * fMoveSpeed));
  if (ezInputManager::GetInputActionState("Game", "MoveDown", &fInput) != ezKeyState::Up)
    m_MainCamera.MoveGlobally(ezVec3(0, 0, -fInput * fMoveSpeed));

  if (ezInputManager::GetInputActionState("Game", "TurnLeft", &fInput) != ezKeyState::Up)
    m_MainCamera.RotateGlobally(ezAngle(), ezAngle(), ezAngle::Degree(-fRotateSpeed * fInput));
  if (ezInputManager::GetInputActionState("Game", "TurnRight", &fInput) != ezKeyState::Up)
    m_MainCamera.RotateGlobally(ezAngle(), ezAngle(), ezAngle::Degree(fRotateSpeed * fInput));
  if (ezInputManager::GetInputActionState("Game", "TurnUp", &fInput) != ezKeyState::Up)
    m_MainCamera.RotateLocally(ezAngle(), ezAngle::Degree(-fRotateSpeed * fInput), ezAngle());
  if (ezInputManager::GetInputActionState("Game", "TurnDown", &fInput) != ezKeyState::Up)
    m_MainCamera.RotateLocally(ezAngle(), ezAngle::Degree(fRotateSpeed * fInput), ezAngle());

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
    ezVec3 vForward = pOwner->GetDirForwards();
    ezVec3 vUp = pOwner->GetDirUp();

    m_MainCamera.LookAt(vPosition, vPosition + vForward, vUp);
  }
}

