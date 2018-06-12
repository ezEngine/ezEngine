#include <PCH.h>
#include <RtsGamePlugin/GameMode/EditLevelMode/EditLevelMode.h>
#include <RtsGamePlugin/GameState/RtsGameState.h>
#include <RendererCore/Messages/SetColorMessage.h>

RtsEditLevelMode::RtsEditLevelMode() = default;
RtsEditLevelMode::~RtsEditLevelMode() = default;

void RtsEditLevelMode::OnActivateMode()
{
}

void RtsEditLevelMode::OnDeactivateMode()
{
}

void RtsEditLevelMode::OnBeforeWorldUpdate()
{
  m_pGameState->RenderUnitSelection();
}

void RtsEditLevelMode::RegisterInputActions()
{
  ezInputActionConfig cfg;

  // Level Editing
  {
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeySpace;
    ezInputManager::SetInputActionConfig("EditLevelMode", "PlaceObject", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyDelete;
    ezInputManager::SetInputActionConfig("EditLevelMode", "RemoveObject", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyNumpadPlus;
    ezInputManager::SetInputActionConfig("EditLevelMode", "NextShipType", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyNumpadMinus;
    ezInputManager::SetInputActionConfig("EditLevelMode", "PrevShipType", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_Key1;
    ezInputManager::SetInputActionConfig("EditLevelMode", "Team0", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_Key2;
    ezInputManager::SetInputActionConfig("EditLevelMode", "Team1", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_Key3;
    ezInputManager::SetInputActionConfig("EditLevelMode", "Team2", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_Key4;
    ezInputManager::SetInputActionConfig("EditLevelMode", "Team3", cfg, true);
  }
}

void RtsEditLevelMode::OnProcessInput(const RtsMouseInputState& MouseInput)
{
  DoDefaultCameraInput(MouseInput);

  ezVec3 vPickedGroundPlanePos;
  if (m_pGameState->PickGroundPlanePosition(vPickedGroundPlanePos).Failed())
    return;

  if (ezInputManager::GetInputActionState("EditLevelMode", "PlaceObject") == ezKeyState::Pressed)
  {
    ezGameObject* pSpawned = nullptr;

    switch (m_iShipType)
    {
    case 0:
      pSpawned = m_pGameState->SpawnNamedObjectAt(ezTransform(vPickedGroundPlanePos, ezQuat::IdentityQuaternion()), "FederationShip1", m_uiTeam);
      break;
    case 1:
      pSpawned = m_pGameState->SpawnNamedObjectAt(ezTransform(vPickedGroundPlanePos, ezQuat::IdentityQuaternion()), "FederationShip2", m_uiTeam);
      break;
    case 2:
      pSpawned = m_pGameState->SpawnNamedObjectAt(ezTransform(vPickedGroundPlanePos, ezQuat::IdentityQuaternion()), "KlingonShip1", m_uiTeam);
      break;
    case 3:
      pSpawned = m_pGameState->SpawnNamedObjectAt(ezTransform(vPickedGroundPlanePos, ezQuat::IdentityQuaternion()), "KlingonShip2", m_uiTeam);
      break;
    case 4:
      pSpawned = m_pGameState->SpawnNamedObjectAt(ezTransform(vPickedGroundPlanePos, ezQuat::IdentityQuaternion()), "KlingonShip3", m_uiTeam);
      break;
    }

    ezMsgSetColor msg;

    switch (m_uiTeam)
    {
    case 0:
      msg.m_Color = ezColorGammaUB(255, 0, 0);
      break;
    case 1:
      msg.m_Color = ezColorGammaUB(0, 255, 0);
      break;
    case 2:
      msg.m_Color = ezColorGammaUB(0, 0, 255);
      break;
    case 3:
      msg.m_Color = ezColorGammaUB(255, 255, 0);
      break;
    }

    pSpawned->PostMessageRecursive(msg, ezObjectMsgQueueType::AfterInitialized);

    return;
  }

  if (ezInputManager::GetInputActionState("EditLevelMode", "Team0") == ezKeyState::Pressed)
  {
    m_uiTeam = 0;
  }
  if (ezInputManager::GetInputActionState("EditLevelMode", "Team1") == ezKeyState::Pressed)
  {
    m_uiTeam = 1;
  }
  if (ezInputManager::GetInputActionState("EditLevelMode", "Team2") == ezKeyState::Pressed)
  {
    m_uiTeam = 2;
  }
  if (ezInputManager::GetInputActionState("EditLevelMode", "Team3") == ezKeyState::Pressed)
  {
    m_uiTeam = 3;
  }


  if (ezInputManager::GetInputActionState("EditLevelMode", "NextShipType") == ezKeyState::Pressed)
  {
    m_iShipType = ezMath::Clamp(m_iShipType + 1, 0, 4);
  }

  if (ezInputManager::GetInputActionState("EditLevelMode", "PrevShipType") == ezKeyState::Pressed)
  {
    m_iShipType = ezMath::Clamp(m_iShipType - 1, 0, 4);
  }

  auto& unitSelection = m_pGameState->m_SelectedUnits;

  if (ezInputManager::GetInputActionState("EditLevelMode", "RemoveObject") == ezKeyState::Pressed)
  {
    for (ezUInt32 i = 0; i < unitSelection.GetCount(); ++i)
    {
      ezGameObjectHandle hObject = unitSelection.GetObject(i);
      m_pMainWorld->DeleteObjectDelayed(hObject);
    }

    return;
  }

  if (MouseInput.m_LeftClickState == ezKeyState::Released)
  {
    m_pGameState->SelectUnits();
  }
}

