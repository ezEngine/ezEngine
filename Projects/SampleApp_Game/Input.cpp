#include "Main.h"
#include "Application.h"

#include <Core/Input/InputManager.h>
#include <InputXBox360/InputDeviceXBox.h>
#include <InputWindows/InputDeviceWindows.h>

#include "ShipComponent.h"

const ezInt32 MaxPlayerActions = 7;

const char* szPlayerActions[MaxPlayerActions] = { "Forwards",        "Backwards",        "Left",             "Right",         "RotLeft", "RotRight",    "Shoot"         };
const char* szControlerKeys[MaxPlayerActions] = { "leftstick_posy",  "leftstick_negy",   "leftstick_negx",  "leftstick_posx", "rightstick_negx",  "rightstick_posx",  "right_trigger" };

void SampleGameApp::UpdateInput()
{
  ezInputManager::Update();

  if (ezInputManager::GetInputActionState("Main", "CloseApp") == ezKeyState::Pressed)
    m_bActiveRenderLoop = false;

  if (ezInputManager::GetInputActionState("Main", "ResetLevel") == ezKeyState::Pressed)
  {
    DestroyGameLevel();
    CreateGameLevel();
  }

  if (ezInputManager::GetInputActionState("Main", "ToggleThumbstick") == ezKeyState::Pressed)
  {
    m_pThumbstick->SetEnabled(!m_pThumbstick->IsEnabled());
    m_pThumbstick2->SetEnabled(!m_pThumbstick2->IsEnabled());
  }
}

static void RegisterInputAction(const char* szInputSet, const char* szInputAction, const char* szKey1, const char* szKey2 = NULL, const char* szKey3 = NULL)
{
  ezInputManager::ezInputActionConfig cfg;

  cfg = ezInputManager::GetInputActionConfig(szInputSet, szInputAction);

  if (szKey1 != NULL)     cfg.m_sInputSlotTrigger[0] = szKey1;
  if (szKey2 != NULL)     cfg.m_sInputSlotTrigger[1] = szKey2;
  if (szKey3 != NULL)     cfg.m_sInputSlotTrigger[2] = szKey3;

  ezInputManager::SetInputActionConfig(szInputSet, szInputAction, cfg, true);
}

void SampleGameApp::SetupInput()
{
  ezInputDeviceXBox360::GetDevice();
  ezInputDeviceWindows::GetDevice()->SetClipMouseCursor(true);
  ezInputDeviceWindows::GetDevice()->SetShowMouseCursor(false);

  RegisterInputAction("Main", "CloseApp", ezInputSlot_KeyEscape);
  RegisterInputAction("Main", "ResetLevel", ezInputSlot_KeyReturn);
  RegisterInputAction("Main", "ToggleThumbstick", ezInputSlot_KeyT);

  // setup all controllers
  for (ezInt32 iPlayer = 0; iPlayer < MaxPlayers; ++iPlayer)
  {
    for (ezInt32 iAction = 0; iAction < MaxPlayerActions; ++iAction)
    {
      ezStringBuilder sAction;
      sAction.Format("Player%i_%s", iPlayer, szPlayerActions[iAction]);

      ezStringBuilder sKey;
      sKey.Format("controller%i_%s", iPlayer, szControlerKeys[iAction]);

      RegisterInputAction("Game", sAction.GetData(), sKey.GetData());

      
    }
  }

  // some more keyboard key bindings

  RegisterInputAction("Game", "Player1_Forwards",   NULL, ezInputSlot_KeyW);
  RegisterInputAction("Game", "Player1_Backwards",  NULL, ezInputSlot_KeyS);
  RegisterInputAction("Game", "Player1_Left",       NULL, ezInputSlot_KeyA);
  RegisterInputAction("Game", "Player1_Right",      NULL, ezInputSlot_KeyD);
  RegisterInputAction("Game", "Player1_Shoot",      NULL, ezInputSlot_KeySpace);
  RegisterInputAction("Game", "Player1_RotLeft",    NULL, ezInputSlot_KeyLeft);
  RegisterInputAction("Game", "Player1_RotRight",   NULL, ezInputSlot_KeyRight);

  RegisterInputAction("Game", "Player3_Forwards",   NULL, ezInputSlot_MouseMoveNegY);
  RegisterInputAction("Game", "Player3_Backwards",  NULL, ezInputSlot_MouseMovePosY);
  RegisterInputAction("Game", "Player3_Left",       NULL, ezInputSlot_MouseMoveNegX);
  RegisterInputAction("Game", "Player3_Right",      NULL, ezInputSlot_MouseMovePosX);
  RegisterInputAction("Game", "Player3_Shoot",      NULL, ezInputSlot_MouseButton2);
  RegisterInputAction("Game", "Player3_RotLeft",    NULL, ezInputSlot_MouseButton0);
  RegisterInputAction("Game", "Player3_RotRight",   NULL, ezInputSlot_MouseButton1);

  m_pThumbstick = EZ_DEFAULT_NEW(ezVirtualThumbStick);
  m_pThumbstick->SetInputArea(ezVec2(0.1f, 0.1f), ezVec2(0.3f, 0.3f), 0.1f, 0.0f);
  m_pThumbstick->SetTriggerInputSlot(ezVirtualThumbStick::Input::MousePosition);
  m_pThumbstick->SetThumbstickOutput(ezVirtualThumbStick::Output::Controller0_LeftStick);
  m_pThumbstick->SetEnabled(false);

  m_pThumbstick2 = EZ_DEFAULT_NEW(ezVirtualThumbStick);
  m_pThumbstick2->SetInputArea(ezVec2(0.2f, 0.1f), ezVec2(0.4f, 0.4f), 0.1f, 0.0f);
  m_pThumbstick2->SetTriggerInputSlot(ezVirtualThumbStick::Input::MousePosition);
  m_pThumbstick2->SetThumbstickOutput(ezVirtualThumbStick::Output::Controller0_RightStick);
  m_pThumbstick2->SetEnabled(false);
}


void Level::UpdatePlayerInput(ezInt32 iPlayer)
{
  float fVal = 0.0f;

  ezGameObject* pShip = m_pWorld->GetObject(m_hPlayerShips[iPlayer]);
  ShipComponent* pShipComponent = pShip->GetComponentOfType<ShipComponent>();

  ezVec3 vVelocity(0.0f);


  const ezQuat qRot = pShip->GetLocalRotation();
  const ezVec3 vShipDir = qRot * ezVec3(0, 1, 0);

  ezStringBuilder sControls[MaxPlayerActions];

  for (ezInt32 iAction = 0; iAction < MaxPlayerActions; ++iAction)
    sControls[iAction].Format("Player%i_%s", iPlayer, szPlayerActions[iAction]);


  if (ezInputManager::GetInputActionState("Game", sControls[0].GetData(), &fVal) != ezKeyState::Up)
  {
    ezVec3 vPos = pShip->GetLocalPosition();
    //vVelocity += 0.1f * vShipDir * fVal;
    vVelocity += 0.1f * ezVec3(0, 1, 0) * fVal;
  }

  if (ezInputManager::GetInputActionState("Game", sControls[1].GetData(), &fVal) != ezKeyState::Up)
  {
    ezVec3 vPos = pShip->GetLocalPosition();
    //vVelocity -= 0.1f * vShipDir * fVal;
    vVelocity += 0.1f * ezVec3(0, -1, 0) * fVal;
  }

  if (ezInputManager::GetInputActionState("Game", sControls[2].GetData(), &fVal) != ezKeyState::Up)
  {
    ezVec3 vPos = pShip->GetLocalPosition();
    //vVelocity += 0.1f * vShipDir * fVal;
    vVelocity += 0.1f * ezVec3(-1, 0, 0) * fVal;
  }

  if (ezInputManager::GetInputActionState("Game", sControls[3].GetData(), &fVal) != ezKeyState::Up)
  {
    ezVec3 vPos = pShip->GetLocalPosition();
    //vVelocity -= 0.1f * vShipDir * fVal;
    vVelocity += 0.1f * ezVec3(1, 0, 0) * fVal;
  }

  if (ezInputManager::GetInputActionState("Game", sControls[4].GetData(), &fVal) != ezKeyState::Up)
  {
    ezQuat qRotation;
    qRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), 3.0f * fVal);

    ezQuat qNewRot = qRotation * pShip->GetLocalRotation();
    pShip->SetLocalRotation(qNewRot);
  }

  if (ezInputManager::GetInputActionState("Game", sControls[5].GetData(), &fVal) != ezKeyState::Up)
  {
    ezQuat qRotation;
    qRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), -3.0f * fVal);

    ezQuat qNewRot = qRotation * pShip->GetLocalRotation();
    pShip->SetLocalRotation(qNewRot);
  }

  if (!vVelocity.IsZero())
    pShipComponent->SetVelocity(vVelocity);


  if (ezInputManager::GetInputActionState("Game", sControls[6].GetData(), &fVal) != ezKeyState::Up)
    pShipComponent->SetIsShooting(true);
  else
    pShipComponent->SetIsShooting(false);
}


