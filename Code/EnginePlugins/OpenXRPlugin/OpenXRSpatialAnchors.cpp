#include <OpenXRPlugin/OpenXRPluginPCH.h>

#include <Core/World/World.h>
#include <GameEngine/XR/StageSpaceComponent.h>
#include <OpenXRPlugin/OpenXRDeclarations.h>
#include <OpenXRPlugin/OpenXRSingleton.h>
#include <OpenXRPlugin/OpenXRSpatialAnchors.h>

EZ_IMPLEMENT_SINGLETON(ezOpenXRSpatialAnchors);

ezOpenXRSpatialAnchors::ezOpenXRSpatialAnchors(ezOpenXR* pOpenXR)
  : m_SingletonRegistrar(this)
  , m_pOpenXR(pOpenXR)
{
  EZ_ASSERT_DEV(m_pOpenXR->m_extensions.m_bSpatialAnchor, "Spatial anchors not supported");
}

ezOpenXRSpatialAnchors::~ezOpenXRSpatialAnchors()
{
  for (auto it = m_Anchors.GetIterator(); it.IsValid(); ++it)
  {
    AnchorData anchorData;
    m_Anchors.TryGetValue(it.Id(), anchorData);
    XR_LOG_ERROR(m_pOpenXR->m_extensions.pfn_xrDestroySpatialAnchorMSFT(anchorData.m_Anchor));
    XR_LOG_ERROR(xrDestroySpace(anchorData.m_Space));
  }
  m_Anchors.Clear();
}

ezXRSpatialAnchorID ezOpenXRSpatialAnchors::CreateAnchor(const ezTransform& globalTransform)
{
  ezWorld* pWorld = m_pOpenXR->GetWorld();
  if (pWorld == nullptr)
    return ezXRSpatialAnchorID();

  ezTransform globalStageTransform;
  globalStageTransform.SetIdentity();
  if (const ezStageSpaceComponentManager* pStageMan = pWorld->GetComponentManager<ezStageSpaceComponentManager>())
  {
    if (const ezStageSpaceComponent* pStage = pStageMan->GetSingletonComponent())
    {
      globalStageTransform = pStage->GetOwner()->GetGlobalTransform();
    }
  }
  ezTransform local;
  local.SetLocalTransform(globalStageTransform, globalTransform);

  XrSpatialAnchorCreateInfoMSFT createInfo{XR_TYPE_SPATIAL_ANCHOR_CREATE_INFO_MSFT};
  createInfo.space = m_pOpenXR->GetBaseSpace();
  createInfo.pose.position = ezOpenXR::ConvertPosition(local.m_vPosition);
  createInfo.pose.orientation = ezOpenXR::ConvertOrientation(local.m_qRotation);
  createInfo.time = m_pOpenXR->m_frameState.predictedDisplayTime;

  XrSpatialAnchorMSFT anchor;
  XrResult res = m_pOpenXR->m_extensions.pfn_xrCreateSpatialAnchorMSFT(m_pOpenXR->m_session, &createInfo, &anchor);
  if (res != XrResult::XR_SUCCESS)
    return ezXRSpatialAnchorID();

  XrSpatialAnchorSpaceCreateInfoMSFT createSpaceInfo{XR_TYPE_SPATIAL_ANCHOR_SPACE_CREATE_INFO_MSFT};
  createSpaceInfo.anchor = anchor;
  createSpaceInfo.poseInAnchorSpace = ezOpenXR::ConvertTransform(ezTransform::IdentityTransform());

  XrSpace space;
  res = m_pOpenXR->m_extensions.pfn_xrCreateSpatialAnchorSpaceMSFT(m_pOpenXR->m_session, &createSpaceInfo, &space);

  return m_Anchors.Insert({anchor, space});
}

ezResult ezOpenXRSpatialAnchors::DestroyAnchor(ezXRSpatialAnchorID id)
{
  AnchorData anchorData;
  if (!m_Anchors.TryGetValue(id, anchorData))
    return EZ_FAILURE;

  XR_LOG_ERROR(m_pOpenXR->m_extensions.pfn_xrDestroySpatialAnchorMSFT(anchorData.m_Anchor));
  XR_LOG_ERROR(xrDestroySpace(anchorData.m_Space));
  m_Anchors.Remove(id);

  return EZ_SUCCESS;
}

ezResult ezOpenXRSpatialAnchors::TryGetAnchorTransform(ezXRSpatialAnchorID id, ezTransform& out_globalTransform)
{
  ezWorld* pWorld = m_pOpenXR->GetWorld();
  if (!pWorld)
    return EZ_FAILURE;

  AnchorData anchorData;
  if (!m_Anchors.TryGetValue(id, anchorData))
    return EZ_FAILURE;

  const XrTime time = m_pOpenXR->m_frameState.predictedDisplayTime;
  XrSpaceLocation viewInScene = {XR_TYPE_SPACE_LOCATION};
  XrResult res = xrLocateSpace(anchorData.m_Space, m_pOpenXR->m_sceneSpace, time, &viewInScene);
  if (res != XrResult::XR_SUCCESS)
    return EZ_FAILURE;

  if ((viewInScene.locationFlags & (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)) ==
      (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT))
  {
    ezTransform globalStageTransform;
    globalStageTransform.SetIdentity();
    if (const ezStageSpaceComponentManager* pStageMan = pWorld->GetComponentManager<ezStageSpaceComponentManager>())
    {
      if (const ezStageSpaceComponent* pStage = pStageMan->GetSingletonComponent())
      {
        globalStageTransform = pStage->GetOwner()->GetGlobalTransform();
      }
    }
    ezTransform local(ezOpenXR::ConvertPosition(viewInScene.pose.position), ezOpenXR::ConvertOrientation(viewInScene.pose.orientation));
    out_globalTransform.SetGlobalTransform(globalStageTransform, local);

    return EZ_SUCCESS;
  }
  return EZ_FAILURE;
}

EZ_STATICLINK_FILE(OpenXRPlugin, OpenXRPlugin_OpenXRSpatialAnchors);
