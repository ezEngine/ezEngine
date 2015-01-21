#include "Main.h"
#include "Application.h"
#include "Window.h"

#include <Foundation/Logging/Log.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Core/Input/InputManager.h>
#include <InputXBox360/InputDeviceXBox.h>
#include <Foundation/Configuration/CVar.h>

#include "ShipComponent.h"

const ezInt32 MaxPlayerActions = 7;

const char* szPlayerActions[MaxPlayerActions] = { "Forwards",        "Backwards",        "Left",             "Right",         "RotLeft", "RotRight",    "Shoot"         };
const char* szControlerKeys[MaxPlayerActions] = { "leftstick_posy",  "leftstick_negy",   "leftstick_negx",  "leftstick_posx", "rightstick_negx",  "rightstick_posx",  "right_trigger" };

ezCVarInt CVarInput("CVar_Input", 0, ezCVarFlags::Default, "Bla bla");

void SampleGameApp::UpdateInput(ezTime UpdateDiff)
{
  ezInputManager::Update(UpdateDiff);

  if (ezInputManager::GetInputActionState("Main", "CloseApp") == ezKeyState::Pressed)
    m_bActiveRenderLoop = false;

  if (ezInputManager::GetInputActionState("Main", "ResetLevel") == ezKeyState::Pressed)
  {
    ezLog::Info("Resetting Level");

    DestroyGameLevel();
    CreateGameLevel();
  }

  if (ezInputManager::GetInputActionState("Main", "Assert") == ezKeyState::Pressed)
  {
    ezLog::Info("Asserting");

    EZ_ASSERT_DEV(false, "This is safe to ignore.");
  }

  if (ezInputManager::GetInputActionState("Main", "CVarUp") == ezKeyState::Pressed)
  {
    CVarInput = CVarInput + 1;
  }

  if (ezInputManager::GetInputActionState("Main", "CVarDown") == ezKeyState::Pressed)
  {
    CVarInput = CVarInput - 1;
  }

  if (ezInputManager::GetInputActionState("Main", "ToggleThumbstick") == ezKeyState::Pressed)
  {
    m_pThumbstick->SetEnabled(!m_pThumbstick->IsEnabled());
    m_pThumbstick2->SetEnabled(!m_pThumbstick2->IsEnabled());
  }

  if (ezInputManager::GetInputActionState("Main", "ToggleMouseShow") == ezKeyState::Pressed)
  {
    m_pWindow->GetInputDevice()->SetShowMouseCursor(!m_pWindow->GetInputDevice()->GetShowMouseCursor());
  }

  if (ezInputManager::GetInputActionState("Main", "ToggleMouseClip") == ezKeyState::Pressed)
  {
    m_pWindow->GetInputDevice()->SetClipMouseCursor(!m_pWindow->GetInputDevice()->GetClipMouseCursor());
  }

  if (ezInputManager::GetInputActionState("Main", "CaptureProfiling") == ezKeyState::Pressed)
  {
    ezFileWriter fileWriter;
    if (fileWriter.Open("profiling.json") == EZ_SUCCESS)
    {
      ezProfilingSystem::Capture(fileWriter);
    }
  }
}

static void RegisterInputAction(const char* szInputSet, const char* szInputAction, const char* szKey1, const char* szKey2 = nullptr, const char* szKey3 = nullptr)
{
  ezInputActionConfig cfg;

  cfg = ezInputManager::GetInputActionConfig(szInputSet, szInputAction);
  cfg.m_bApplyTimeScaling = true;

  if (szKey1 != nullptr)     cfg.m_sInputSlotTrigger[0] = szKey1;
  if (szKey2 != nullptr)     cfg.m_sInputSlotTrigger[1] = szKey2;
  if (szKey3 != nullptr)     cfg.m_sInputSlotTrigger[2] = szKey3;

  ezInputManager::SetInputActionConfig(szInputSet, szInputAction, cfg, true);
}

void SampleGameApp::SetupInput()
{
  ezInputDeviceXBox360::GetDevice()->EnableVibration(0, true);
  ezInputDeviceXBox360::GetDevice()->EnableVibration(1, true);
  ezInputDeviceXBox360::GetDevice()->EnableVibration(2, true);
  ezInputDeviceXBox360::GetDevice()->EnableVibration(3, true);

  m_pWindow->GetInputDevice()->SetClipMouseCursor(true);
  m_pWindow->GetInputDevice()->SetShowMouseCursor(false);
  m_pWindow->GetInputDevice()->SetMouseSpeed(ezVec2(0.002f));

  RegisterInputAction("Main", "CloseApp", ezInputSlot_KeyEscape);
  RegisterInputAction("Main", "ResetLevel", ezInputSlot_KeyReturn);
  RegisterInputAction("Main", "ToggleThumbstick", ezInputSlot_KeyT);
  RegisterInputAction("Main", "Assert", ezInputSlot_KeyNumpadEnter);
  RegisterInputAction("Main", "CVarDown", ezInputSlot_KeyO);
  RegisterInputAction("Main", "CVarUp", ezInputSlot_KeyP);
  RegisterInputAction("Main", "ToggleMouseShow", ezInputSlot_KeyM);
  RegisterInputAction("Main", "ToggleMouseClip", ezInputSlot_KeyN);
  RegisterInputAction("Main", "CaptureProfiling", ezInputSlot_KeyF12);

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

  RegisterInputAction("Game", "Player1_Forwards",   nullptr, ezInputSlot_KeyW);
  RegisterInputAction("Game", "Player1_Backwards",  nullptr, ezInputSlot_KeyS);
  RegisterInputAction("Game", "Player1_Left",       nullptr, ezInputSlot_KeyA);
  RegisterInputAction("Game", "Player1_Right",      nullptr, ezInputSlot_KeyD);
  RegisterInputAction("Game", "Player1_Shoot",      nullptr, ezInputSlot_KeySpace);
  RegisterInputAction("Game", "Player1_RotLeft",    nullptr, ezInputSlot_KeyLeft,  ezInputSlot_MouseMoveNegX);
  RegisterInputAction("Game", "Player1_RotRight",   nullptr, ezInputSlot_KeyRight, ezInputSlot_MouseMovePosX);

  //RegisterInputAction("Game", "Player3_Forwards",   nullptr, ezInputSlot_MouseMoveNegY);
  //RegisterInputAction("Game", "Player3_Backwards",  nullptr, ezInputSlot_MouseMovePosY);
  //RegisterInputAction("Game", "Player3_Left",       nullptr, ezInputSlot_MouseMoveNegX);
  //RegisterInputAction("Game", "Player3_Right",      nullptr, ezInputSlot_MouseMovePosX);
  //RegisterInputAction("Game", "Player3_Shoot",      nullptr, ezInputSlot_MouseButton2);
  //RegisterInputAction("Game", "Player3_RotLeft",    nullptr, ezInputSlot_MouseButton0);
  //RegisterInputAction("Game", "Player3_RotRight",   nullptr, ezInputSlot_MouseButton1);

  m_pThumbstick = EZ_DEFAULT_NEW(ezVirtualThumbStick);
  m_pThumbstick->SetInputArea(ezVec2(0.1f, 0.1f), ezVec2(0.3f, 0.3f), 0.1f, 0.0f);
  m_pThumbstick->SetTriggerInputSlot(ezVirtualThumbStick::Input::Touchpoint);
  m_pThumbstick->SetThumbstickOutput(ezVirtualThumbStick::Output::Controller0_LeftStick);
  m_pThumbstick->SetEnabled(false);

  m_pThumbstick2 = EZ_DEFAULT_NEW(ezVirtualThumbStick);
  m_pThumbstick2->SetInputArea(ezVec2(0.2f, 0.1f), ezVec2(0.4f, 0.4f), 0.1f, 0.0f);
  m_pThumbstick2->SetTriggerInputSlot(ezVirtualThumbStick::Input::Touchpoint);
  m_pThumbstick2->SetThumbstickOutput(ezVirtualThumbStick::Output::Controller0_RightStick);
  m_pThumbstick2->SetEnabled(false);
}


void Level::UpdatePlayerInput(ezInt32 iPlayer)
{
  float fVal = 0.0f;

  ezGameObject* pShip = nullptr;
  if (!m_pWorld->TryGetObject(m_hPlayerShips[iPlayer], pShip))
    return;

  ShipComponent* pShipComponent = nullptr;
  if (!pShip->TryGetComponentOfBaseType(pShipComponent))
    return;

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
    vVelocity += 0.1f * ezVec3(0, 1, 0) * fVal * 60.0f;
  }

  if (ezInputManager::GetInputActionState("Game", sControls[1].GetData(), &fVal) != ezKeyState::Up)
  {
    ezVec3 vPos = pShip->GetLocalPosition();
    //vVelocity -= 0.1f * vShipDir * fVal;
    vVelocity += 0.1f * ezVec3(0, -1, 0) * fVal * 60.0f;
  }

  if (ezInputManager::GetInputActionState("Game", sControls[2].GetData(), &fVal) != ezKeyState::Up)
  {
    ezVec3 vPos = pShip->GetLocalPosition();
    //vVelocity += 0.1f * vShipDir * fVal;
    vVelocity += 0.1f * ezVec3(-1, 0, 0) * fVal * 60.0f;
  }

  if (ezInputManager::GetInputActionState("Game", sControls[3].GetData(), &fVal) != ezKeyState::Up)
  {
    ezVec3 vPos = pShip->GetLocalPosition();
    //vVelocity -= 0.1f * vShipDir * fVal;
    vVelocity += 0.1f * ezVec3(1, 0, 0) * fVal * 60.0f;
  }

  if (ezInputManager::GetInputActionState("Game", sControls[4].GetData(), &fVal) != ezKeyState::Up)
  {
    ezQuat qRotation;
    qRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(3.0f * fVal * 60.0f));

    ezQuat qNewRot = qRotation * pShip->GetLocalRotation();
    pShip->SetLocalRotation(qNewRot);
  }

  if (ezInputManager::GetInputActionState("Game", sControls[5].GetData(), &fVal) != ezKeyState::Up)
  {
    ezQuat qRotation;
    qRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(-3.0f * fVal * 60.0f));

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


