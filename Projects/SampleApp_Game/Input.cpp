#include "Main.h"
#include "Application.h"

#include <Core/Input/InputManager.h>
#include <InputXBox360/InputDeviceXBox.h>
#include <InputWindows/InputDeviceWindows.h>

#include "ShipComponent.h"

const ezInt32 MaxPlayerActions = 5;

const char* szPlayerActions[MaxPlayerActions] = { "Forwards",        "Backwards",        "Left",             "Right",            "Shoot"         };
const char* szControlerKeys[MaxPlayerActions] = { "leftstick_posy",  "leftstick_negy",   "rightstick_negx",  "rightstick_posx",  "right_trigger" };

void SampleGameApp::UpdateInput()
{
  ezInputManager::Update();

  if (ezInputManager::GetInputActionState("Main", "CloseApp") == ezKeyState::Pressed)
    m_bActiveRenderLoop = false;
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
  g_InputDeviceXBox360.EnableVibration(0, true);
  g_InputDeviceWindows.SetClipMouseCursor(true);
  g_InputDeviceWindows.SetShowMouseCursor(false);

  RegisterInputAction("Main", "CloseApp", "keyboard_escape");

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

  RegisterInputAction("Game", "Player0_Forwards",   NULL, "keyboard_w");
  RegisterInputAction("Game", "Player0_Backwards",  NULL, "keyboard_s");
  RegisterInputAction("Game", "Player0_Left",       NULL, "keyboard_a");
  RegisterInputAction("Game", "Player0_Right",      NULL, "keyboard_d");
  RegisterInputAction("Game", "Player0_Shoot",      NULL, "keyboard_space");

  RegisterInputAction("Game", "Player1_Forwards",   NULL, "keyboard_up");
  RegisterInputAction("Game", "Player1_Backwards",  NULL, "keyboard_down");
  RegisterInputAction("Game", "Player1_Left",       NULL, "keyboard_left");
  RegisterInputAction("Game", "Player1_Right",      NULL, "keyboard_right");
  RegisterInputAction("Game", "Player1_Shoot",      NULL, "keyboard_right_ctrl");

  RegisterInputAction("Game", "Player2_Forwards",   NULL, "keyboard_numpad_8");
  RegisterInputAction("Game", "Player2_Backwards",  NULL, "keyboard_numpad_5");
  RegisterInputAction("Game", "Player2_Left",       NULL, "keyboard_numpad_4");
  RegisterInputAction("Game", "Player2_Right",      NULL, "keyboard_numpad_6");
  RegisterInputAction("Game", "Player2_Shoot",      NULL, "keyboard_numpad_0");

  RegisterInputAction("Game", "Player3_Forwards",   NULL, "mouse_button_1");
  RegisterInputAction("Game", "Player3_Backwards",  NULL, "mouse_button_2");
  RegisterInputAction("Game", "Player3_Left",       NULL, "mouse_move_negx");
  RegisterInputAction("Game", "Player3_Right",      NULL, "mouse_move_posx");
  RegisterInputAction("Game", "Player3_Shoot",      NULL, "mouse_button_0");
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
    vVelocity += 0.1f * vShipDir * fVal;

    pShip->SetLocalPosition(vPos);
  }

  if (ezInputManager::GetInputActionState("Game", sControls[1].GetData(), &fVal) != ezKeyState::Up)
  {
    ezVec3 vPos = pShip->GetLocalPosition();
    vVelocity -= 0.1f * vShipDir * fVal;

    pShip->SetLocalPosition(vPos);
  }

  if (!vVelocity.IsZero())
    pShipComponent->SetVelocity(vVelocity);

  if (ezInputManager::GetInputActionState("Game", sControls[2].GetData(), &fVal) != ezKeyState::Up)
  {
    ezQuat qRotation;
    qRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), 4.0f * fVal);

    ezQuat qNewRot = qRotation * pShip->GetLocalRotation();
    pShip->SetLocalRotation(qNewRot);
  }

  if (ezInputManager::GetInputActionState("Game", sControls[3].GetData(), &fVal) != ezKeyState::Up)
  {
    ezQuat qRotation;
    qRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), -4.0f * fVal);

    ezQuat qNewRot = qRotation * pShip->GetLocalRotation();
    pShip->SetLocalRotation(qNewRot);
  }

  if (ezInputManager::GetInputActionState("Game", sControls[4].GetData(), &fVal) != ezKeyState::Up)
    pShipComponent->SetIsShooting(true);
  else
    pShipComponent->SetIsShooting(false);
}


