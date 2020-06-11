#include <RtsGamePluginPCH.h>

#include <RendererCore/Messages/SetColorMessage.h>
#include <RmlUiPlugin/Components/RmlUiCanvas2DComponent.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RtsGamePlugin/GameMode/EditLevelMode/EditLevelMode.h>
#include <RtsGamePlugin/GameState/RtsGameState.h>

const char* g_BuildItemTypes[] = {
  "FederationShip1",
  "FederationShip2",
  "FederationShip3",
  "KlingonShip1",
  "KlingonShip2",
  "KlingonShip3",
};

RtsEditLevelMode::RtsEditLevelMode() = default;
RtsEditLevelMode::~RtsEditLevelMode() = default;

void RtsEditLevelMode::OnActivateMode()
{
  CreateEditUI();
}

void RtsEditLevelMode::OnDeactivateMode()
{
  EZ_LOCK(m_pMainWorld->GetWriteMarker());

  ezRmlUiCanvas2DComponent* pUiComponent = nullptr;
  if (m_pMainWorld->TryGetComponent(m_hEditUIComponent, pUiComponent))
  {
    pUiComponent->SetActiveFlag(false);
  }
}

void RtsEditLevelMode::OnBeforeWorldUpdate()
{
  DisplaySelectModeUI();
  DisplayEditUI();

  m_pGameState->RenderUnitSelection();
}

void RtsEditLevelMode::CreateEditUI()
{
  constexpr const char* szKey = "EditUI";
  ezGameObject* pEditUIObject = nullptr;
  if (!m_pMainWorld->TryGetObjectWithGlobalKey(ezTempHashedString(szKey), pEditUIObject))
  {
    ezGameObjectDesc desc;
    desc.m_bDynamic = true;
    desc.m_sName.Assign(szKey);

    m_pMainWorld->CreateObject(desc, pEditUIObject);
    pEditUIObject->SetGlobalKey(szKey);

    ezRmlUiCanvas2DComponent* pUiComponent = nullptr;
    m_hEditUIComponent = ezRmlUiCanvas2DComponent::CreateComponent(pEditUIObject, pUiComponent);

    pUiComponent->SetRmlFile("UI/Edit.rml");
    pUiComponent->SetAnchorPoint(ezVec2(1, 0));
    pUiComponent->SetOffset(ezVec2I32(10, 10));
    pUiComponent->SetSize(ezVec2U32(200, 150));

    pUiComponent->EnsureInitialized();

    /*pUiComponent->GetRmlContext()->RegisterEventHandler("switchToImGui", [](Rml::Core::Event& e) {
      s_bUseRmlUi = false;
    });
    pUiComponent->GetRmlContext()->RegisterEventHandler("switchMode", [](Rml::Core::Event& e) {
      auto& sValue = e.GetTargetElement()->GetId();
      if (sValue == "battle")
      {
        RtsGameState::GetSingleton()->SwitchToGameMode(RtsActiveGameMode::BattleMode);
      }
      else if (sValue == "edit")
      {
        RtsGameState::GetSingleton()->SwitchToGameMode(RtsActiveGameMode::EditLevelMode);
      }
      else if (sValue == "mainmenu")
      {
        RtsGameState::GetSingleton()->SwitchToGameMode(RtsActiveGameMode::MainMenuMode);
      }
    });*/
  }
}

void RtsEditLevelMode::DisplayEditUI()
{
  ezRmlUiCanvas2DComponent* pUiComponent = nullptr;
  if (m_pMainWorld->TryGetComponent(m_hEditUIComponent, pUiComponent))
  {
    pUiComponent->SetActiveFlag(s_bUseRmlUi);
  }

  if (s_bUseRmlUi)
  {
  }
  else
  {
    ezImgui::GetSingleton()->SetCurrentContextForView(m_hMainView);

    const ezSizeU32 resolution = ezImgui::GetSingleton()->GetCurrentWindowResolution();

    const float ww = 200;

    ImGui::SetNextWindowPos(ImVec2((float)resolution.width - ww - 10, 10));
    ImGui::SetNextWindowSize(ImVec2(ww, 150));
    ImGui::Begin("Edit Level", nullptr,
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

    int iTeam = m_uiTeam;
    if (ImGui::Combo("Team", &iTeam, "Red\0Green\0Blue\0Yellow\0\0", 4))
    {
      m_uiTeam = iTeam;
    }

    if (ImGui::Combo("Build", &m_iShipType, g_BuildItemTypes, EZ_ARRAY_SIZE(g_BuildItemTypes)))
    {
    }

    ImGui::Text("Select: %s", ezInputManager::GetInputSlotDisplayName(ezInputSlot_MouseButton0));
    ImGui::Text("Create: %s", ezInputManager::GetInputSlotDisplayName("EditLevelMode", "PlaceObject"));
    ImGui::Text("Remove: %s", ezInputManager::GetInputSlotDisplayName("EditLevelMode", "RemoveObject"));

    ImGui::End();
  }
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

void RtsEditLevelMode::OnProcessInput(const RtsMouseInputState& MouseInput)
{
  DoDefaultCameraInput(MouseInput);

  ezVec3 vPickedGroundPlanePos;
  if (m_pGameState->PickGroundPlanePosition(vPickedGroundPlanePos).Failed())
    return;

  if (ezInputManager::GetInputActionState("EditLevelMode", "PlaceObject") == ezKeyState::Pressed)
  {
    ezGameObject* pSpawned = nullptr;

    pSpawned = m_pGameState->SpawnNamedObjectAt(ezTransform(vPickedGroundPlanePos, ezQuat::IdentityQuaternion()),
      g_BuildItemTypes[m_iShipType], m_uiTeam);

    ezMsgSetColor msg;
    msg.m_Color = RtsGameMode::GetTeamColor(m_uiTeam);

    pSpawned->PostMessageRecursive(msg, ezTime::Zero(), ezObjectMsgQueueType::AfterInitialized);

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

  m_pGameState->DetectHoveredSelectable();

  if (MouseInput.m_LeftClickState == ezKeyState::Released)
  {
    m_pGameState->SelectUnits();
  }
}
