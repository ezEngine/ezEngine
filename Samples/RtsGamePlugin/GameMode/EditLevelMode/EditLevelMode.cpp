#include <PCH.h>
#include <RtsGamePlugin/GameMode/EditLevelMode/EditLevelMode.h>
#include <GameEngine/DearImgui/DearImgui.h>
#include <Core/Input/InputManager.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
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
  }
}

void RtsEditLevelMode::OnProcessInput()
{
  DoDefaultCameraInput();

  ezVec3 vPickedGroundPlanePos;
  if (m_pGameState->PickGroundPlanePosition(vPickedGroundPlanePos).Failed())
    return;

  if (ezInputManager::GetInputActionState("EditLevelMode", "PlaceObject") == ezKeyState::Pressed)
  {
    m_pGameState->SpawnNamedObjectAt(ezTransform(vPickedGroundPlanePos, ezQuat::IdentityQuaternion()), "FederationShip1", 0);
    return;
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

  if (m_LeftClickState == ezKeyState::Released)
  {
    m_pGameState->SelectUnits();
  }
}

