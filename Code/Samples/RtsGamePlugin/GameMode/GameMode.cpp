#include <RtsGamePlugin/RtsGamePluginPCH.h>

#include <GameEngine/DearImgui/DearImgui.h>
#include <RmlUiPlugin/Components/RmlUiCanvas2DComponent.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiSingleton.h>
#include <RtsGamePlugin/GameMode/GameMode.h>
#include <RtsGamePlugin/GameState/RtsGameState.h>

bool RtsGameMode::s_bUseRmlUi = true;

RtsGameMode::RtsGameMode() = default;
RtsGameMode::~RtsGameMode() = default;

void RtsGameMode::ActivateMode(ezWorld* pMainWorld, ezViewHandle hView, ezCamera* pMainCamera)
{
  if (!m_bInitialized)
  {
    m_pGameState = RtsGameState::GetSingleton();
    m_pMainWorld = pMainWorld;
    m_hMainView = hView;
    m_pMainCamera = pMainCamera;

    SetupSelectModeUI();

    m_bInitialized = true;
    RegisterInputActions();
  }

  OnActivateMode();
}

void RtsGameMode::DeactivateMode()
{
  OnDeactivateMode();
}

void RtsGameMode::ProcessInput(const RtsMouseInputState& mouseInput)
{
  if (s_bUseRmlUi)
  {
    if (ezRmlUi::GetSingleton() != nullptr)
    {
      // do not process input, when RmlUi already wants to work with it
      if (ezRmlUi::GetSingleton()->AnyContextWantsInput())
        return;
    }
  }
  else
  {
    if (ezImgui::GetSingleton() != nullptr)
    {
      // we got input (ie. this function was called), so let Imgui know that it can use the input
      ezImgui::GetSingleton()->SetPassInputToImgui(true);

      // do not process input, when Imgui already wants to work with it
      if (ezImgui::GetSingleton()->WantsInput())
        return;
    }
  }

  OnProcessInput(mouseInput);
}

void RtsGameMode::BeforeWorldUpdate()
{
  OnBeforeWorldUpdate();
}

void RtsGameMode::DoDefaultCameraInput(const RtsMouseInputState& MouseInput)
{
  ezView* pView = nullptr;
  if (!ezRenderWorld::TryGetView(m_hMainView, pView))
    return;

  const auto vp = pView->GetViewport();

  float movePosX, moveNegX, movePosY, moveNegY, zoomIn, zoomOut;
  ezInputManager::GetInputActionState("Game", "CamMovePosX", &movePosX);
  ezInputManager::GetInputActionState("Game", "CamMoveNegX", &moveNegX);
  ezInputManager::GetInputActionState("Game", "CamMovePosY", &movePosY);
  ezInputManager::GetInputActionState("Game", "CamMoveNegY", &moveNegY);
  ezInputManager::GetInputActionState("Game", "CamZoomIn", &zoomIn);
  ezInputManager::GetInputActionState("Game", "CamZoomOut", &zoomOut);

  const float moveX = movePosX - moveNegX;
  const float moveY = movePosY - moveNegY;
  const float zoom = -zoomIn + zoomOut;

  const bool bMoveCamera = MouseInput.m_RightClickState != ezKeyState::Up;

  const float fDimY = m_pMainCamera->GetFovOrDim();
  const float fDimX = (fDimY / vp.height) * vp.width;

  float fZoom = m_pGameState->GetCameraZoom();

  if (zoom != 0.0f)
  {
    if (zoom > 0)
      fZoom *= 1.1f;
    else
      fZoom *= 1.0f / 1.1f;

    fZoom = m_pGameState->SetCameraZoom(fZoom);

    ezVec3 pos = m_pMainCamera->GetCenterPosition();
    pos.z = fZoom;
    m_pMainCamera->LookAt(pos, pos + m_pMainCamera->GetCenterDirForwards(), m_pMainCamera->GetCenterDirUp());
  }

  if (bMoveCamera)
  {
    const float fMoveScale = 0.005f * fZoom;

    const float fMoveX = fDimX * moveX * fMoveScale;
    const float fMoveY = fDimY * moveY * fMoveScale;

    m_pMainCamera->MoveGlobally(-fMoveY, fMoveX, 0);
  }
}

bool RtsMouseInputState::HasMouseMoved(ezVec2U32 vStart, ezVec2U32 vNow)
{
  const ezVec2 v1((float)vNow.x, (float)vNow.y);
  const ezVec2 v2((float)vStart.x, (float)vStart.y);

  return (v1 - v2).GetLength() > 3.0f;
}

ezColor RtsGameMode::GetTeamColor(ezUInt16 uiTeam)
{
  switch (uiTeam)
  {
    case 0:
      return ezColorGammaUB(255, 0, 0);
    case 1:
      return ezColorGammaUB(0, 255, 0);
    case 2:
      return ezColorGammaUB(0, 0, 255);
    case 3:
      return ezColorGammaUB(255, 255, 0);
  }

  return ezColor::White;
}

void RtsGameMode::SetupSelectModeUI()
{
  ezGameObject* pSelectModeUIObject = nullptr;
  if (!m_pMainWorld->TryGetObjectWithGlobalKey(ezTempHashedString("SelectModeUI"), pSelectModeUIObject))
    return;

  ezRmlUiCanvas2DComponent* pUiComponent = nullptr;
  if (!pSelectModeUIObject->TryGetComponentOfBaseType(pUiComponent))
    return;

  ezRmlUiContext* pRmlContext = pUiComponent->GetOrCreateRmlContext();

  pRmlContext->RegisterEventHandler("switchToImGui", [](Rml::Event& e)
    { s_bUseRmlUi = false; });
  pRmlContext->RegisterEventHandler("switchMode", [](Rml::Event& e)
    {
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
    } });

  m_hSelectModeUIComponent = pUiComponent->GetHandle();
}

void RtsGameMode::DisplaySelectModeUI()
{
  ezRmlUiCanvas2DComponent* pUiComponent = nullptr;
  if (m_pMainWorld->TryGetComponent(m_hSelectModeUIComponent, pUiComponent))
  {
    pUiComponent->SetActiveFlag(s_bUseRmlUi);
  }

  if (s_bUseRmlUi && pUiComponent != nullptr)
  {
    const RtsActiveGameMode mode = RtsGameState::GetSingleton()->GetActiveGameMode();
    const char* szActiveModeId = "battle";
    if (mode == RtsActiveGameMode::EditLevelMode)
    {
      szActiveModeId = "edit";
    }
    else if (mode == RtsActiveGameMode::MainMenuMode)
    {
      szActiveModeId = "mainmenu";
    }

    if (auto pActiveModeElement = pUiComponent->GetRmlContext()->GetDocument(0)->GetElementById(szActiveModeId))
    {
      pActiveModeElement->SetAttribute("checked", "");
    }
  }
  else
  {
    ezImgui::GetSingleton()->SetCurrentContextForView(m_hMainView);

    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowSize(ImVec2(140, 130));
    ImGui::Begin(
      "Game Mode", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

    const RtsActiveGameMode mode = RtsGameState::GetSingleton()->GetActiveGameMode();

    if (ImGui::RadioButton("Battle", mode == RtsActiveGameMode::BattleMode))
      RtsGameState::GetSingleton()->SwitchToGameMode(RtsActiveGameMode::BattleMode);

    if (ImGui::RadioButton("Edit", mode == RtsActiveGameMode::EditLevelMode))
      RtsGameState::GetSingleton()->SwitchToGameMode(RtsActiveGameMode::EditLevelMode);

    if (ImGui::RadioButton("Main Menu", mode == RtsActiveGameMode::MainMenuMode))
      RtsGameState::GetSingleton()->SwitchToGameMode(RtsActiveGameMode::MainMenuMode);

    if (ImGui::Button("Switch to RmlUi"))
      s_bUseRmlUi = true;

    ImGui::End();
  }
}
