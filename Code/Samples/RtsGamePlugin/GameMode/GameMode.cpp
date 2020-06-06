#include <RtsGamePluginPCH.h>

#include <GameEngine/DearImgui/DearImgui.h>
#include <RmlUiPlugin/Components/RmlUiCanvas2DComponent.h>
#include <RmlUiPlugin/RmlUiContext.h>
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

    CreateSelectModeUI();

    m_bInitialized = true;
    RegisterInputActions();
  }

  OnActivateMode();
}

void RtsGameMode::DeactivateMode()
{
  OnDeactivateMode();
}

void RtsGameMode::ProcessInput(const RtsMouseInputState& MouseInput)
{
  OnProcessInput(MouseInput);
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

bool RtsMouseInputState::HasMouseMoved(ezVec2U32 start, ezVec2U32 now)
{
  const ezVec2 v1((float)now.x, (float)now.y);
  const ezVec2 v2((float)start.x, (float)start.y);

  return (v1 - v2).GetLength() > 3.0f;
}

ezColor RtsGameMode::GetTeamColor(ezInt32 iTeam)
{
  switch (iTeam)
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

void RtsGameMode::CreateSelectModeUI()
{
  constexpr const char* szKey = "SelectModeUI";
  ezGameObject* pSelectModeUIObject = nullptr;
  if (!m_pMainWorld->TryGetObjectWithGlobalKey(ezTempHashedString(szKey), pSelectModeUIObject))
  {
    ezGameObjectDesc desc;
    desc.m_bDynamic = true;
    desc.m_sName.Assign(szKey);

    m_pMainWorld->CreateObject(desc, pSelectModeUIObject);
    pSelectModeUIObject->SetGlobalKey(szKey);

    ezRmlUiCanvas2DComponent* pUiComponent = nullptr;
    m_hSelectModeUIComponent = ezRmlUiCanvas2DComponent::CreateComponent(pSelectModeUIObject, pUiComponent);

    pUiComponent->SetRmlFile("UI/SelectMode.rml");
    pUiComponent->SetOffset(ezVec2I32(10, 10));
    pUiComponent->SetSize(ezVec2U32(140, 130));

    pUiComponent->EnsureInitialized();

    pUiComponent->GetRmlContext()->RegisterEventHandler("switchToImGui", [&](Rml::Core::Event& e) {
      s_bUseRmlUi = false;
    });
  }
  else
  {
    ezRmlUiCanvas2DComponent* pUiComponent = nullptr;
    pSelectModeUIObject->TryGetComponentOfBaseType(pUiComponent);

    m_hSelectModeUIComponent = pUiComponent->GetHandle();
  }
}

void RtsGameMode::DisplaySelectModeUI()
{
  ezComponent* pUiComponent = nullptr;
  if (m_pMainWorld->TryGetComponent(m_hSelectModeUIComponent, pUiComponent))
  {
    pUiComponent->SetActiveFlag(s_bUseRmlUi);
  }

  if (!s_bUseRmlUi)
  {
    ezImgui::GetSingleton()->SetCurrentContextForView(m_hMainView);

    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowSize(ImVec2(140, 130));
    ImGui::Begin("Game Mode", nullptr,
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

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
