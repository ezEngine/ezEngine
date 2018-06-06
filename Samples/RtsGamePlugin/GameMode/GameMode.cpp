#include <PCH.h>
#include <RtsGamePlugin/GameMode/GameMode.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Pipeline/View.h>
#include <GameEngine/Prefabs/PrefabResource.h>

RtsGameMode::RtsGameMode() = default;
RtsGameMode::~RtsGameMode() = default;

void RtsGameMode::ActivateMode(ezWorld* pMainWorld, ezViewHandle hView, ezCamera* pMainCamera)
{
  if (!m_bInitialized)
  {
    m_pMainWorld = pMainWorld;
    m_hMainView = hView;
    m_pMainCamera = pMainCamera;

    m_bInitialized = true;
    RegisterInputActions();
  }

  OnActivateMode();
}

void RtsGameMode::DeactivateMode()
{
  OnDeactivateMode();
}

void RtsGameMode::ProcessInput(ezUInt32 uiMousePosX, ezUInt32 uiMousePosY, ezKeyState::Enum LeftClickState, ezKeyState::Enum RightClickState)
{
  m_uiMousePosX = uiMousePosX;
  m_uiMousePosY = uiMousePosY;
  m_LeftClickState = LeftClickState;
  m_RightClickState = RightClickState;

  OnProcessInput();
}

void RtsGameMode::BeforeWorldUpdate()
{
  OnBeforeWorldUpdate();
}

ezResult RtsGameMode::ComputePickingRay(ezVec3& out_vRayStart, ezVec3& out_vRayDir) const
{
  ezView* pView = nullptr;
  if (!ezRenderWorld::TryGetView(m_hMainView, pView))
    return EZ_FAILURE;

  const auto& vp = pView->GetViewport();

  if (ezGraphicsUtils::ConvertScreenPosToWorldPos(pView->GetInverseViewProjectionMatrix(ezCameraEye::Left), (ezUInt32)vp.x, (ezUInt32)vp.y, (ezUInt32)vp.width, (ezUInt32)vp.height, ezVec3((float)m_uiMousePosX, (float)m_uiMousePosY, 0), out_vRayStart, &out_vRayDir).Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezResult RtsGameMode::PickGroundPlanePosition(const ezVec3& vRayStart, const ezVec3& vRayDir, ezVec3& out_vPositon) const
{
  ezPlane p;
  p.SetFromNormalAndPoint(ezVec3(0, 0, 1), ezVec3(0));

  return p.GetRayIntersection(vRayStart, vRayDir, nullptr, &out_vPositon) ? EZ_SUCCESS : EZ_FAILURE;
}

ezGameObject* RtsGameMode::SpawnNamedObjectAt(const ezTransform& transform, const char* szObjectName, ezUInt16 uiTeamID)
{
  ezPrefabResourceHandle hPrefab = ezResourceManager::LoadResource<ezPrefabResource>(szObjectName);

  ezResourceLock<ezPrefabResource> pPrefab(hPrefab, ezResourceAcquireMode::NoFallback);

  ezHybridArray<ezGameObject*, 8> CreatedRootObjects;
  pPrefab->InstantiatePrefab(*m_pMainWorld, transform, ezGameObjectHandle(), &CreatedRootObjects, &uiTeamID, nullptr);

  return CreatedRootObjects[0];
}
