#include <PCH.h>
#include <RtsGamePlugin/GameMode/EditLevelMode/EditLevelMode.h>
#include <RtsGamePlugin/GameState/RtsGameState.h>

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
    switch (m_iShipType)
    {
    case 0:
      m_pGameState->SpawnNamedObjectAt(ezTransform(vPickedGroundPlanePos, ezQuat::IdentityQuaternion()), "FederationShip1", 0);
      break;
    case 1:
      m_pGameState->SpawnNamedObjectAt(ezTransform(vPickedGroundPlanePos, ezQuat::IdentityQuaternion()), "KlingonShip1", 0);
      break;
    case 2:
      m_pGameState->SpawnNamedObjectAt(ezTransform(vPickedGroundPlanePos, ezQuat::IdentityQuaternion()), "KlingonShip2", 0);
      break;
    case 3:
      m_pGameState->SpawnNamedObjectAt(ezTransform(vPickedGroundPlanePos, ezQuat::IdentityQuaternion()), "KlingonShip3", 0);
      break;

    }

    return;
  }

  if (ezInputManager::GetInputActionState("EditLevelMode", "NextShipType") == ezKeyState::Pressed)
  {
    m_iShipType = ezMath::Clamp(m_iShipType + 1, 0, 3);
  }

  if (ezInputManager::GetInputActionState("EditLevelMode", "PrevShipType") == ezKeyState::Pressed)
  {
    m_iShipType = ezMath::Clamp(m_iShipType - 1, 0, 3);
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

