#include <RTSPlugin/RTSPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <Core/System/Window.h>
#include <Core/World/World.h>
#include <GameEngine/DearImgui/DearImgui.h>
#include <RTSPlugin/Components/ComponentMessages.h>
#include <RTSPlugin/Components/SelectableComponent.h>
#include <RTSPlugin/GameMode/GameMode.h>
#include <RTSPlugin/GameState/RTSGameState.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(RTSGameState, 1, ezRTTIDefaultAllocator<RTSGameState>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

RTSGameState* RTSGameState::s_pSingleton = nullptr;

RTSGameState::RTSGameState()
{
  s_pSingleton = this;
}

RTSGameState::~RTSGameState() = default;

void RTSGameState::RequestQuit(ezStringView sRequestedBy)
{
  if (sRequestedBy == "window")
  {
    // we could do something when the window close button gets pressed
    // for example show the main menu with the option to quit the game
  }

  if (sRequestedBy == "editor-esc")
  {
    // sent when the ESC key was pressed while playing in the editor
    // but only when ezGameApplicationInputFlags::Dev_EscapeToClose is used
    // by default this is on, but deactivated in this sample in RTSGameState::ConfigureInputActions()
  }

  // by default, just quit, but we could filter this out in certain cases
  SUPER::RequestQuit(sRequestedBy);
}

ezString RTSGameState::GetStartupSceneFile()
{
  // replace this to load a certain scene at startup
  // the default implementation looks at the command line "-scene" argument

  // if we have a "-scene" command line argument, it was launched from the editor and we should load that
  if (ezCommandLineUtils::GetGlobalInstance()->HasOption("-scene"))
  {
    return ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-scene");
  }

  // otherwise, we use the hardcoded 'Main.ezScene'
  // if that doesn't exist, this function has to be adjusted
  // note that you can return an asset GUID here, instead of a path
  return "AssetCache/Common/Scenes/Main.ezBinScene";
}

float RTSGameState::GetCameraZoom() const
{
  return m_fCameraZoom;
}

float RTSGameState::SetCameraZoom(float fZoom)
{
  m_fCameraZoom = ezMath::Clamp(fZoom, 1.0f, 50.0f);

  return m_fCameraZoom;
}

void RTSGameState::OnActivation(ezWorld* pWorld, ezStringView sStartPosition, const ezTransform& startPositionOffset)
{
  EZ_LOG_BLOCK("GameState::Activate");

  SUPER::OnActivation(pWorld, sStartPosition, startPositionOffset);

  if (ezImgui::GetSingleton() == nullptr)
  {
    EZ_DEFAULT_NEW(ezImgui);
  }

  PreloadAssets();

  SwitchToGameMode(RtsActiveGameMode::EditLevelMode);
}

void RTSGameState::OnDeactivation()
{
  EZ_LOG_BLOCK("GameState::Deactivate");

  SetActiveGameMode(nullptr);

  if (ezImgui::GetSingleton() != nullptr)
  {
    ezImgui* pImgui = ezImgui::GetSingleton();
    EZ_DEFAULT_DELETE(pImgui);
  }

  SUPER::OnDeactivation();
}

void RTSGameState::PreloadAssets()
{
  // Load all assets that are referenced in some Collections

  m_hCollectionSpace = ezResourceManager::LoadResource<ezCollectionResource>("{ 7cd0dfa6-d2bb-433e-9fa2-b17bfae42b6b }");
  m_hCollectionFederation = ezResourceManager::LoadResource<ezCollectionResource>("{ 1edd3af8-6d59-4825-b853-ee8d7a60cb03 }");
  m_hCollectionKlingons = ezResourceManager::LoadResource<ezCollectionResource>("{ c683d049-0e54-4c42-9764-a122f9dbc69d }");

  // Register the loaded assets with the names defined in the collections
  // This allows to easily spawn those objects with human readable names instead of GUIDs
  {
    ezResourceLock<ezCollectionResource> pCollection(m_hCollectionSpace, ezResourceAcquireMode::BlockTillLoaded);
    pCollection->RegisterNames();
  }
  {
    ezResourceLock<ezCollectionResource> pCollection(m_hCollectionFederation, ezResourceAcquireMode::BlockTillLoaded);
    pCollection->RegisterNames();
  }
  {
    ezResourceLock<ezCollectionResource> pCollection(m_hCollectionKlingons, ezResourceAcquireMode::BlockTillLoaded);
    pCollection->RegisterNames();
  }
}

void RTSGameState::BeforeWorldUpdate()
{
  if (IsLoadingSceneInBackground())
    return;

  EZ_LOCK(m_pMainWorld->GetWriteMarker());

  ActivateQueuedGameMode();

  m_SelectedUnits.RemoveDeadObjects();

  if (m_pActiveGameMode)
  {
    m_pActiveGameMode->BeforeWorldUpdate();
  }

  // update the sound listener position to be the same as the camera position
  if (ezSoundInterface* pSoundInterface = ezSingletonRegistry::GetSingletonInstance<ezSoundInterface>())
  {
    const ezVec3 pos = m_MainCamera.GetCenterPosition();
    const ezVec3 dir = m_MainCamera.GetCenterDirForwards();
    const ezVec3 up = m_MainCamera.GetCenterDirUp();

    pSoundInterface->SetListener(0, pos, dir, up, ezVec3::MakeZero());
  }
}

void RTSGameState::ActivateQueuedGameMode()
{
  switch (m_GameModeToSwitchTo)
  {
    case RtsActiveGameMode::None:
      SetActiveGameMode(nullptr);
      break;
    case RtsActiveGameMode::MainMenuMode:
      SetActiveGameMode(&m_MainMenuMode);
      break;
    case RtsActiveGameMode::BattleMode:
      SetActiveGameMode(&m_BattleMode);
      break;
    case RtsActiveGameMode::EditLevelMode:
      SetActiveGameMode(&m_EditLevelMode);
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }
}

void RTSGameState::ConfigureMainCamera()
{
  SUPER::ConfigureMainCamera();

  m_fCameraZoom = 20.0f;
  ezVec3 vCameraPos = ezVec3(0.0f, 0.0f, m_fCameraZoom);

  ezCoordinateSystem coordSys;

  if (m_pMainWorld)
  {
    m_pMainWorld->GetCoordinateSystem(vCameraPos, coordSys);
  }
  else
  {
    coordSys.m_vForwardDir.Set(1, 0, 0);
    coordSys.m_vRightDir.Set(0, 1, 0);
    coordSys.m_vUpDir.Set(0, 0, 1);
  }

  m_MainCamera.SetCameraMode(ezCameraMode::PerspectiveFixedFovY, 45.0f, 0.1f, 100);
  m_MainCamera.LookAt(vCameraPos, vCameraPos - coordSys.m_vUpDir, coordSys.m_vForwardDir);
}


void RTSGameState::OnChangedMainWorld(ezWorld* pPrevWorld, ezWorld* pNewWorld, ezStringView sStartPosition, const ezTransform& startPositionOffset)
{
  SUPER::OnChangedMainWorld(pPrevWorld, pNewWorld, sStartPosition, startPositionOffset);

  m_SelectedUnits.Clear();
  m_SelectedUnits.SetWorld(pNewWorld);
}

void RTSGameState::SwitchToGameMode(RtsActiveGameMode mode)
{
  // can't just switch game modes in the middle of a frame, so delay this to the next frame
  m_GameModeToSwitchTo = mode;
}

void RTSGameState::SetActiveGameMode(RtsGameMode* pMode)
{
  if (m_pActiveGameMode == pMode)
    return;

  if (m_pActiveGameMode)
    m_pActiveGameMode->DeactivateMode();

  m_pActiveGameMode = pMode;

  if (m_pActiveGameMode)
    m_pActiveGameMode->ActivateMode(m_pMainWorld, m_hMainView, &m_MainCamera);
}

ezGameObject* RTSGameState::DetectHoveredSelectable()
{
  m_hHoveredSelectable.Invalidate();
  ezGameObject* pSelected = PickSelectableObject();

  if (pSelected != nullptr)
  {
    m_hHoveredSelectable = pSelected->GetHandle();
    return pSelected;
  }

  return nullptr;
}

void RTSGameState::SelectUnits()
{
  ezGameObject* pSelected = PickSelectableObject();

  if (pSelected != nullptr)
  {
    if (ezInputManager::GetInputSlotState(ezInputSlot_KeyLeftCtrl) == ezKeyState::Down || ezInputManager::GetInputSlotState(ezInputSlot_KeyRightCtrl) == ezKeyState::Down)
    {
      m_SelectedUnits.ToggleSelection(pSelected->GetHandle());
    }
    else
    {
      m_SelectedUnits.Clear();
      m_SelectedUnits.AddObject(pSelected->GetHandle());
    }
  }
  else
  {
    m_SelectedUnits.Clear();
  }
}

void RTSGameState::RenderUnitSelection() const
{
  ezBoundingBox bbox;

  for (ezUInt32 i = 0; i < m_SelectedUnits.GetCount(); ++i)
  {
    ezGameObjectHandle hObject = m_SelectedUnits.GetObject(i);

    ezGameObject* pObject;
    if (!m_pMainWorld->TryGetObject(hObject, pObject))
      continue;

    RtsSelectableComponent* pSelectable;
    if (!pObject->TryGetComponentOfBaseType(pSelectable))
      continue;

    const float fRadius = pSelectable->m_fSelectionRadius * 1.1f;

    ezTransform t = pObject->GetGlobalTransform();
    t.m_vScale.Set(1.0f);
    t.m_qRotation.SetIdentity();

    bbox = ezBoundingBox::MakeFromCenterAndHalfExtents(ezVec3::MakeZero(), ezVec3(fRadius, fRadius, 0));
    ezDebugRenderer::DrawLineBoxCorners(m_pMainWorld, bbox, 0.1f, ezColor::White, t);

    RenderUnitHealthbar(pObject, fRadius);
  }

  // hovered unit
  {
    ezGameObject* pObject;
    if (m_pMainWorld->TryGetObject(m_hHoveredSelectable, pObject))
    {
      RtsSelectableComponent* pSelectable;
      if (pObject->TryGetComponentOfBaseType(pSelectable))
      {
        const float fRadius = pSelectable->m_fSelectionRadius * 1.1f;

        ezTransform t = pObject->GetGlobalTransform();
        t.m_vScale.Set(1.0f);
        t.m_qRotation.SetIdentity();

        bbox = ezBoundingBox::MakeFromCenterAndHalfExtents(ezVec3::MakeZero(), ezVec3(fRadius, fRadius, 0));
        ezDebugRenderer::DrawLineBoxCorners(m_pMainWorld, bbox, 0.1f, ezColor::DodgerBlue, t);

        RenderUnitHealthbar(pObject, fRadius);
      }
    }
  }
}

void RTSGameState::RenderUnitHealthbar(ezGameObject* pObject, float fSelectableRadius) const
{
  RtsMsgGatherUnitStats msgStats;
  pObject->SendMessageRecursive(msgStats);

  if (msgStats.m_uiMaxHealth > 0)
  {
    const float percentage = msgStats.m_uiCurHealth / (float)msgStats.m_uiMaxHealth;
    const float fOffset = 0.01f;

    ezVec3 pos = pObject->GetGlobalPosition();
    pos.x += fSelectableRadius - 0.04f - fOffset;

    ezColor c = ezColor::Lime;

    if (percentage < 0.3f)
      c = ezColor::Red;
    else if (percentage < 0.6f)
      c = ezColor::Orange;
    else if (percentage < 0.8f)
      c = ezColor::Yellow;

    ezBoundingBox bbox = ezBoundingBox::MakeFromCenterAndHalfExtents(ezVec3::MakeZero(), ezVec3(0.04f, fSelectableRadius * percentage - fOffset, 0));
    ezDebugRenderer::DrawSolidBox(m_pMainWorld, bbox, c, ezTransform(pos));
  }
}

ezResult RTSGameState::ComputePickingRay()
{
  ezView* pView = nullptr;
  if (!ezRenderWorld::TryGetView(m_hMainView, pView))
    return EZ_FAILURE;

  ezVec3 vMousePos((float)m_MouseInputState.m_MousePos.x, (float)m_MouseInputState.m_MousePos.y, 0);

  pView->ConvertScreenPixelPosToNormalizedPos(vMousePos);

  return pView->ComputePickingRay(vMousePos.x, vMousePos.y, m_vCurrentPickingRayStart, m_vCurrentPickingRayDir);
}

ezResult RTSGameState::PickGroundPlanePosition(ezVec3& out_vPositon) const
{
  ezPlane p;
  p = ezPlane::MakeFromNormalAndPoint(ezVec3(0, 0, 1), ezVec3(0));

  return p.GetRayIntersection(m_vCurrentPickingRayStart, m_vCurrentPickingRayDir, nullptr, &out_vPositon) ? EZ_SUCCESS : EZ_FAILURE;
}

ezGameObject* RTSGameState::PickSelectableObject() const
{
  struct Payload
  {
    ezGameObject* pBestObject = nullptr;
    float fBestDistSQR = ezMath::Square(1000.0f);
    ezVec3 vGroundPos;
  };

  Payload pl;

  if (PickGroundPlanePosition(pl.vGroundPos).Failed())
    return nullptr;

  ezSpatialSystem::QueryCallback cb = [&pl](ezGameObject* pObject)
  {
    RtsSelectableComponent* pSelectable = nullptr;
    if (pObject->TryGetComponentOfBaseType(pSelectable))
    {
      const float dist = (pObject->GetGlobalTransform().m_vPosition - pl.vGroundPos).GetLengthSquared();

      if (dist < pl.fBestDistSQR && dist <= ezMath::Square(pSelectable->m_fSelectionRadius))
      {
        pl.fBestDistSQR = dist;
        pl.pBestObject = pObject;
      }
    }

    return ezVisitorExecution::Continue;
  };

  InspectObjectsInArea(pl.vGroundPos.GetAsVec2(), 1.0f, cb);

  return pl.pBestObject;
}

// BEGIN-DOCS-CODE-SNIPPET: spatial-query
void RTSGameState::InspectObjectsInArea(const ezVec2& vPosition, float fRadius, ezSpatialSystem::QueryCallback callback) const
{
  ezBoundingSphere sphere = ezBoundingSphere::MakeFromCenterAndRadius(vPosition.GetAsVec3(0), fRadius);
  ezSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = RtsSelectableComponent::s_SelectableCategory.GetBitmask();
  m_pMainWorld->GetSpatialSystem()->FindObjectsInSphere(sphere, queryParams, callback);
}
// END-DOCS-CODE-SNIPPET

ezGameObject* RTSGameState::SpawnNamedObjectAt(const ezTransform& transform, const char* szObjectName, ezUInt16 uiTeamID)
{
  ezPrefabResourceHandle hPrefab = ezResourceManager::LoadResource<ezPrefabResource>(szObjectName);

  ezResourceLock<ezPrefabResource> pPrefab(hPrefab, ezResourceAcquireMode::BlockTillLoaded);

  ezHybridArray<ezGameObject*, 8> CreatedRootObjects;

  ezPrefabInstantiationOptions options;
  options.m_pCreatedRootObjectsOut = &CreatedRootObjects;
  options.m_pOverrideTeamID = &uiTeamID;

  pPrefab->InstantiatePrefab(*m_pMainWorld, transform, options);

  return CreatedRootObjects[0];
}
