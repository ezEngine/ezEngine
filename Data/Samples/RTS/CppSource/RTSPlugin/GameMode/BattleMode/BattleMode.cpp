#include <RTSPlugin/RTSPluginPCH.h>

#include <RTSPlugin/Components/ComponentMessages.h>
#include <RTSPlugin/GameMode/BattleMode/BattleMode.h>
#include <RTSPlugin/GameState/RTSGameState.h>
#include <RmlUiPlugin/Components/RmlUiCanvas2DComponent.h>
#include <RmlUiPlugin/RmlUiContext.h>

RtsBattleMode::RtsBattleMode() = default;
RtsBattleMode::~RtsBattleMode() = default;

void RtsBattleMode::OnActivateMode()
{
  SetUiActive(m_pMainWorld, ezTempHashedString("game-ui"), true);
}

void RtsBattleMode::OnDeactivateMode()
{
  SetUiActive(m_pMainWorld, ezTempHashedString("game-ui"), false);
}

void RtsBattleMode::OnBeforeWorldUpdate()
{
  SetupSelectModeUI();

  m_pGameState->RenderUnitSelection();
}

void RtsBattleMode::OnProcessInput(const RtsMouseInputState& MouseInput, bool bUiWantsInput)
{
  if (ezInputManager::GetInputSlotState(ezInputSlot_KeyEscape) == ezKeyState::Pressed)
  {
    m_pGameState->SwitchToGameMode(RtsActiveGameMode::MainMenuMode);
    return;
  }

  if (bUiWantsInput)
    return;

  DoDefaultCameraInput(MouseInput);

  ezVec3 vPickedGroundPlanePos;
  if (m_pGameState->PickGroundPlanePosition(vPickedGroundPlanePos).Failed())
    return;

  const auto& unitSelection = m_pGameState->m_SelectedUnits;

  ezGameObject* pHoveredSelectable = m_pGameState->DetectHoveredSelectable();

  if (MouseInput.m_LeftClickState == ezKeyState::Released)
  {
    m_pGameState->SelectUnits();
  }

  if (MouseInput.m_RightClickState == ezKeyState::Released && !MouseInput.m_bRightMouseMoved)
  {
    if (ezInputManager::GetInputSlotState(ezInputSlot_KeyLeftCtrl) == ezKeyState::Up)
    {
      RtsMsgAssignPosition msg;
      msg.m_vTargetPosition = vPickedGroundPlanePos.GetAsVec2();

      for (ezUInt32 i = 0; i < unitSelection.GetCount(); ++i)
      {
        m_pMainWorld->SendMessage(unitSelection.GetObject(i), msg);
      }
    }
    else
    {
      RtsMsgSetTarget msg;
      msg.m_hObject = pHoveredSelectable ? pHoveredSelectable->GetHandle() : ezGameObjectHandle();

      for (ezUInt32 i = 0; i < unitSelection.GetCount(); ++i)
      {
        m_pMainWorld->SendMessage(unitSelection.GetObject(i), msg);
      }
    }
  }
}
