#include <PCH.h>
#include <ProceduralPlacementPlugin/Tasks/PlacementTask.h>
#include <Foundation/Math/Random.h>
#include <Foundation/SimdMath/SimdConversion.h>

using namespace ezPPInternal;

EZ_CHECK_AT_COMPILETIME(sizeof(PlacementPoint) == 32);
//EZ_CHECK_AT_COMPILETIME(sizeof(PlacementTransform) == 64); // TODO: Fails on Linux and Mac

PlacementTask::PlacementTask()
{
}

PlacementTask::~PlacementTask()
{
}

void PlacementTask::Execute()
{
  m_OutputTransforms.SetCountUninitialized(m_InputPoints.GetCount());

  //TODO: faster random, simd optimize
  ezRandom rng;
  ezVec3 vUp = ezVec3(0, 0, 1);

  ezUInt32 uiNumObjects = m_pLayer->m_ObjectsToPlace.GetCount();

  for (ezUInt32 i = 0; i < m_InputPoints.GetCount(); ++i)
  {
    auto& placementPoint = m_InputPoints[i];
    auto& placementTransform = m_OutputTransforms[i];

    rng.Initialize(placementPoint.m_uiPointIndex + 1);

    float offsetX = (float)rng.DoubleMinMax(m_pLayer->m_vMinOffset.x, m_pLayer->m_vMaxOffset.x);
    float offsetY = (float)rng.DoubleMinMax(m_pLayer->m_vMinOffset.y, m_pLayer->m_vMaxOffset.y);
    float offsetZ = (float)rng.DoubleMinMax(m_pLayer->m_vMinOffset.z, m_pLayer->m_vMaxOffset.z);
    ezSimdVec4f offset(offsetX, offsetY, offsetZ);

    ezAngle angle = ezAngle::Degree((float)rng.DoubleInRange(0.0f, 360.0f));
    ezQuat qYawRot; qYawRot.SetFromAxisAndAngle(vUp, angle);
    ezQuat qToNormalRot; qToNormalRot.SetShortestRotation(vUp, ezMath::Lerp(vUp, placementPoint.m_vNormal, m_pLayer->m_fAlignToNormal));

    placementTransform.m_Transform.SetIdentity();
    placementTransform.m_Transform.m_Position = ezSimdConversion::ToVec3(placementPoint.m_vPosition) + offset;
    placementTransform.m_Transform.m_Rotation = ezSimdConversion::ToQuat(qToNormalRot * qYawRot);
    placementTransform.m_Transform.m_Scale = ezSimdConversion::ToVec3(ezMath::Lerp(m_pLayer->m_vMinScale, m_pLayer->m_vMaxScale, placementPoint.m_fScale));

    placementTransform.m_Color = ezColor::White;

    placementTransform.m_uiObjectIndex = rng.UIntInRange(uiNumObjects);
    placementTransform.m_uiPointIndex = placementPoint.m_uiPointIndex;
  }
}
