#include <RTSPlugin/RTSPluginPCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/Utils/Blackboard.h>
#include <RTSPlugin/GameMode/EditLevelMode/EditLevelMode.h>
#include <RTSPlugin/GameState/RTSGameState.h>
#include <RmlUiPlugin/Components/RmlUiCanvas2DComponent.h>
#include <RmlUiPlugin/RmlUiContext.h>

const char* g_BuildItemTypes[] = {
  "FederationShip1",
  "FederationShip2",
  "FederationShip3",
  "KlingonShip1",
  "KlingonShip2",
  "KlingonShip3",
};

static ezHashedString s_sTeam = ezMakeHashedString("Team");
static ezHashedString s_sShipType = ezMakeHashedString("ShipType");
static ezHashedString s_sSelectKey = ezMakeHashedString("SelectKey");
static ezHashedString s_sCreateKey = ezMakeHashedString("CreateKey");
static ezHashedString s_sRemoveKey = ezMakeHashedString("RemoveKey");
static ezHashedString s_sShowEditWidget = ezMakeHashedString("ShowEditWidget");

RtsEditLevelMode::RtsEditLevelMode()
{
  // create a blackboard to easily share state with the RML UI
  m_pBlackboard = ezBlackboard::Create();
  m_pBlackboard->SetName("game-ui");

  m_pBlackboard->SetEntryValue(s_sTeam, 0);
  m_pBlackboard->SetEntryValue(s_sShipType, 0);
  m_pBlackboard->SetEntryValue(s_sSelectKey, ezVariant());
  m_pBlackboard->SetEntryValue(s_sCreateKey, ezVariant());
  m_pBlackboard->SetEntryValue(s_sRemoveKey, ezVariant());
}

RtsEditLevelMode::~RtsEditLevelMode() = default;

void RtsEditLevelMode::OnActivateMode()
{
  m_pBlackboard->SetEntryValue(s_sShowEditWidget, true);
  SetUiActive(m_pMainWorld, ezTempHashedString("game-ui"), true);

  SetupEditUI();
}

void RtsEditLevelMode::OnDeactivateMode()
{
  EZ_LOCK(m_pMainWorld->GetWriteMarker());

  m_pBlackboard->SetEntryValue(s_sShowEditWidget, false);
  SetUiActive(m_pMainWorld, ezTempHashedString("game-ui"), false);
}

void RtsEditLevelMode::OnBeforeWorldUpdate()
{
  SetupSelectModeUI();

  m_pGameState->RenderUnitSelection();
}

void RtsEditLevelMode::SetupEditUI()
{
  // Set blackboard values
  {
    m_pBlackboard->SetEntryValue(s_sSelectKey, ezInputManager::GetInputSlotDisplayName(ezInputSlot_MouseButton0));
    m_pBlackboard->SetEntryValue(s_sCreateKey, ezInputManager::GetInputSlotDisplayName("game-ui", "PlaceObject"));
    m_pBlackboard->SetEntryValue(s_sRemoveKey, ezInputManager::GetInputSlotDisplayName("game-ui", "RemoveObject"));
  }

  if (m_hEditUIComponent.IsInvalidated())
  {
    ezGameObject* pEditUIObject = nullptr;
    if (!m_pMainWorld->TryGetObjectWithGlobalKey(ezTempHashedString("game-ui"), pEditUIObject))
      return;

    ezRmlUiCanvas2DComponent* pUiComponent = nullptr;
    if (!pEditUIObject->TryGetComponentOfBaseType(pUiComponent))
      return;

    pUiComponent->AddBlackboardBinding(m_pBlackboard);

    m_hEditUIComponent = pUiComponent->GetHandle();
  }
}

void RtsEditLevelMode::OnFirstActivation()
{
  ezInputActionConfig cfg;

  // Level Editing
  {
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeySpace;
    ezInputManager::SetInputActionConfig("game-ui", "PlaceObject", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyDelete;
    ezInputManager::SetInputActionConfig("game-ui", "RemoveObject", cfg, true);
  }
}

void RtsEditLevelMode::OnProcessInput(const RtsMouseInputState& MouseInput, bool bUiWantsInput)
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

  if (ezInputManager::GetInputActionState("game-ui", "PlaceObject") == ezKeyState::Pressed)
  {
    ezGameObject* pSpawned = nullptr;

    ezUInt16 uiTeam = static_cast<ezUInt16>(m_pBlackboard->GetEntryValue(s_sTeam).Get<int>());
    int iShipType = m_pBlackboard->GetEntryValue(s_sShipType).Get<int>();
    pSpawned = m_pGameState->SpawnNamedObjectAt(ezTransform(vPickedGroundPlanePos, ezQuat::MakeIdentity()), g_BuildItemTypes[iShipType], uiTeam);

    ezMsgSetColor msg;
    msg.m_Color = RtsGameMode::GetTeamColor(uiTeam);

    pSpawned->PostMessageRecursive(msg, ezTime::MakeZero(), ezObjectMsgQueueType::AfterInitialized);

    return;
  }

  auto& unitSelection = m_pGameState->m_SelectedUnits;

  if (ezInputManager::GetInputActionState("game-ui", "RemoveObject") == ezKeyState::Pressed)
  {
    for (ezUInt32 i = 0; i < unitSelection.GetCount(); ++i)
    {
      ezGameObjectHandle hObject = unitSelection.GetObject(i);
      m_pMainWorld->DeleteObjectDelayed(hObject);
    }

    return;
  }

  m_pGameState->DetectHoveredSelectable();

  if (MouseInput.m_LeftClickState == ezKeyState::Released)
  {
    m_pGameState->SelectUnits();
  }
}
