#include <PCH.h>
#include <RtsGamePlugin/Components/SelectableComponent.h>
#include <RtsGamePlugin/GameMode/GameMode.h>
#include <RtsGamePlugin/GameState/RtsGameState.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(RtsGameState, 1, ezRTTIDefaultAllocator<RtsGameState>)
EZ_END_DYNAMIC_REFLECTED_TYPE

RtsGameState* RtsGameState::s_pSingleton = nullptr;

RtsGameState::RtsGameState()
{
  s_pSingleton = this;
}

float RtsGameState::GetCameraZoom() const
{
  return m_fCameraZoom;
}

float RtsGameState::SetCameraZoom(float zoom)
{
  m_fCameraZoom = ezMath::Clamp(zoom, 1.0f, 50.0f);

  return m_fCameraZoom;
}

ezGameState::Priority RtsGameState::DeterminePriority(ezGameApplicationType AppType, ezWorld* pWorld) const
{
  return RtsGameState::Priority::Default;
}

void RtsGameState::OnActivation(ezWorld* pWorld)
{
  EZ_LOG_BLOCK("GameState::Activate");

  SUPER::OnActivation(pWorld);

  m_SelectedUnits.SetWorld(m_pMainWorld);

  if (ezImgui::GetSingleton() == nullptr)
  {
    EZ_DEFAULT_NEW(ezImgui);
  }

  PreloadAssets();

  SwitchToGameMode(RtsActiveGameMode::EditLevelMode);
}

void RtsGameState::OnDeactivation()
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

void RtsGameState::PreloadAssets()
{
  // Load all assets that are referenced in some Collections

  m_CollectionSpace = ezResourceManager::LoadResource<ezCollectionResource>("{ 7cd0dfa6-d2bb-433e-9fa2-b17bfae42b6b }");
  m_CollectionFederation = ezResourceManager::LoadResource<ezCollectionResource>("{ 1edd3af8-6d59-4825-b853-ee8d7a60cb03 }");
  m_CollectionKlingons = ezResourceManager::LoadResource<ezCollectionResource>("{ c683d049-0e54-4c42-9764-a122f9dbc69d }");

  // Register the loaded assets with the names defined in the collections
  // This allows to easily spawn those objects with human readable names instead of GUIDs
  {
    ezResourceLock<ezCollectionResource> pCollection(m_CollectionSpace, ezResourceAcquireMode::NoFallback);
    pCollection->RegisterNames();
  }
  {
    ezResourceLock<ezCollectionResource> pCollection(m_CollectionFederation, ezResourceAcquireMode::NoFallback);
    pCollection->RegisterNames();
  }
  {
    ezResourceLock<ezCollectionResource> pCollection(m_CollectionKlingons, ezResourceAcquireMode::NoFallback);
    pCollection->RegisterNames();
  }
}

void RtsGameState::BeforeWorldUpdate()
{
  EZ_LOCK(m_pMainWorld->GetWriteMarker());

  ActivateQueuedGameMode();

  m_SelectedUnits.RemoveDeadObjects();

  // prepare the UI for the next frame
  if (ezImgui::GetSingleton() != nullptr)
  {
    ezView* pView = nullptr;
    if (ezRenderWorld::TryGetView(m_hMainView, pView))
    {
      const ezRectFloat viewport = pView->GetViewport();
      ezImgui::GetSingleton()->BeginNewFrame(ezSizeU32((ezUInt32)viewport.width, (ezUInt32)viewport.height));
    }
  }

  if (m_pActiveGameMode)
  {
    m_pActiveGameMode->BeforeWorldUpdate();
  }

  // update the sound listener position to be the same as the camera position
  if (ezSoundInterface* pSoundInterface = ezSingletonRegistry::GetSingletonInstance<ezSoundInterface>("ezSoundInterface"))
  {
    const ezVec3 pos = m_MainCamera.GetCenterPosition();
    const ezVec3 dir = m_MainCamera.GetCenterDirForwards();
    const ezVec3 up = m_MainCamera.GetCenterDirUp();

    pSoundInterface->SetListener(0, pos, dir, up, ezVec3::ZeroVector());
  }
}

void RtsGameState::ActivateQueuedGameMode()
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

void RtsGameState::ConfigureMainCamera()
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

void RtsGameState::SwitchToGameMode(RtsActiveGameMode mode)
{
  // can't just switch game modes in the middle of a frame, so delay this to the next frame
  m_GameModeToSwitchTo = mode;
}

void RtsGameState::SetActiveGameMode(RtsGameMode* pMode)
{
  if (m_pActiveGameMode == pMode)
    return;

  if (m_pActiveGameMode)
    m_pActiveGameMode->DeactivateMode();

  m_pActiveGameMode = pMode;

  if (m_pActiveGameMode)
    m_pActiveGameMode->ActivateMode(m_pMainWorld, m_hMainView, &m_MainCamera);
}

ezGameObject* RtsGameState::DetectHoveredSelectable()
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

void RtsGameState::SelectUnits()
{
  ezGameObject* pSelected = PickSelectableObject();

  if (pSelected != nullptr)
  {
    if (ezInputManager::GetInputSlotState(ezInputSlot_KeyLeftCtrl) == ezKeyState::Down ||
        ezInputManager::GetInputSlotState(ezInputSlot_KeyRightCtrl) == ezKeyState::Down)
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

void RtsGameState::RenderUnitSelection() const
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

    ezTransform t = pObject->GetGlobalTransform();
    t.m_vScale.Set(1.0f);
    t.m_qRotation.SetIdentity();

    bbox.SetCenterAndHalfExtents(ezVec3::ZeroVector(), ezVec3(pSelectable->m_fSelectionRadius, pSelectable->m_fSelectionRadius, 0));
    ezDebugRenderer::DrawLineBoxCorners(m_pMainWorld, bbox, 0.1f, ezColor::White, t);
  }

  // hovered unit
  {
    ezGameObject* pObject;
    if (m_pMainWorld->TryGetObject(m_hHoveredSelectable, pObject))
    {
      RtsSelectableComponent* pSelectable;
      if (pObject->TryGetComponentOfBaseType(pSelectable))
      {

        ezTransform t = pObject->GetGlobalTransform();
        t.m_vScale.Set(1.0f);
        t.m_qRotation.SetIdentity();

        bbox.SetCenterAndHalfExtents(ezVec3::ZeroVector(), ezVec3(pSelectable->m_fSelectionRadius, pSelectable->m_fSelectionRadius, 0));
        ezDebugRenderer::DrawLineBoxCorners(m_pMainWorld, bbox, 0.1f, ezColor::DodgerBlue, t);
      }
    }
  }
}

ezResult RtsGameState::ComputePickingRay()
{
  ezView* pView = nullptr;
  if (!ezRenderWorld::TryGetView(m_hMainView, pView))
    return EZ_FAILURE;

  const auto& vp = pView->GetViewport();

  if (ezGraphicsUtils::ConvertScreenPosToWorldPos(pView->GetInverseViewProjectionMatrix(ezCameraEye::Left), (ezUInt32)vp.x, (ezUInt32)vp.y, (ezUInt32)vp.width, (ezUInt32)vp.height, ezVec3((float)m_MouseInputState.m_MousePos.x, (float)m_MouseInputState.m_MousePos.y, 0), m_vCurrentPickingRayStart, &m_vCurrentPickingRayDir).Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezResult RtsGameState::PickGroundPlanePosition(ezVec3& out_vPositon) const
{
  ezPlane p;
  p.SetFromNormalAndPoint(ezVec3(0, 0, 1), ezVec3(0));

  return p.GetRayIntersection(m_vCurrentPickingRayStart, m_vCurrentPickingRayDir, nullptr, &out_vPositon) ? EZ_SUCCESS : EZ_FAILURE;
}

ezGameObject* RtsGameState::PickSelectableObject() const
{
  ezVec3 vGroundPos;
  if (PickGroundPlanePosition(vGroundPos).Failed())
    return nullptr;

  ezBoundingSphere sphere(vGroundPos, 100.0f);

  ezDynamicArray<ezGameObject*> objects;
  m_pMainWorld->GetSpatialSystem().FindObjectsInSphere(sphere, objects, nullptr);

  ezGameObject* pBestObject = nullptr;
  float fBestDistSQR = ezMath::Square(1000.0f);

  for (ezUInt32 i = 0; i < objects.GetCount(); ++i)
  {
    RtsSelectableComponent* pSelectable = nullptr;
    if (objects[i]->TryGetComponentOfBaseType(pSelectable))
    {
      const float dist = (objects[i]->GetGlobalTransform().m_vPosition - vGroundPos).GetLengthSquared();

      if (dist < fBestDistSQR && dist <= ezMath::Square(pSelectable->m_fSelectionRadius))
      {
        fBestDistSQR = dist;
        pBestObject = objects[i];
      }
    }
  }

  return pBestObject;
}

ezGameObject* RtsGameState::SpawnNamedObjectAt(const ezTransform& transform, const char* szObjectName, ezUInt16 uiTeamID)
{
  ezPrefabResourceHandle hPrefab = ezResourceManager::LoadResource<ezPrefabResource>(szObjectName);

  ezResourceLock<ezPrefabResource> pPrefab(hPrefab, ezResourceAcquireMode::NoFallback);

  ezHybridArray<ezGameObject*, 8> CreatedRootObjects;
  pPrefab->InstantiatePrefab(*m_pMainWorld, transform, ezGameObjectHandle(), &CreatedRootObjects, &uiTeamID, nullptr);

  return CreatedRootObjects[0];
}
