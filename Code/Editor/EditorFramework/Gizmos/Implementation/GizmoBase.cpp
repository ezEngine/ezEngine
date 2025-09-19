#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <Foundation/Utilities/GraphicsUtils.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGizmo, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezGizmo::ezGizmo()
{
  m_bVisible = false;
  m_Transformation.SetIdentity();
  m_Transformation.m_vScale.SetZero();
}

void ezGizmo::SetVisible(bool bVisible)
{
  if (m_bVisible == bVisible)
    return;

  m_bVisible = bVisible;

  OnVisibleChanged(m_bVisible);
}

void ezGizmo::SetTransformation(const ezTransform& transform)
{
  if (m_Transformation.IsIdentical(transform))
    return;

  m_Transformation = transform;

  OnTransformationChanged(m_Transformation);
}

void ezGizmo::GetInverseViewProjectionMatrix(ezMat4& out_mInvViewProj) const
{
  if (m_pCamera == nullptr || m_vViewport.x == 0.0f || m_vViewport.y == 0.0f)
  {
    out_mInvViewProj.SetIdentity();
    return;
  }

  ezMat4 mView = m_pCamera->GetViewMatrix();
  ezMat4 mProj;
  m_pCamera->GetProjectionMatrix((float)m_vViewport.x / (float)m_vViewport.y, mProj);
  ezMat4 mViewProj = mProj * mView;
  out_mInvViewProj = mViewProj.GetInverse();
}

ezResult ezGizmo::GetPointOnPlane(const ezPlane& plane, const ezVec2I32& vScreenPos, const ezMat4& mInvViewProj, ezVec3& out_Result) const
{
  out_Result = ezVec3::MakeZero();

  ezVec3 vPos, vRayDir;
  if (ezGraphicsUtils::ConvertScreenPosToWorldPos(mInvViewProj, 0, 0, m_vViewport.x, m_vViewport.y, ezVec3(vScreenPos.x, vScreenPos.y, 0), vPos, &vRayDir).Failed())
    return EZ_FAILURE;

  ezVec3 vIntersection;
  if (!plane.GetRayIntersection(m_pCamera->GetPosition(), vRayDir, nullptr, &vIntersection))
    return EZ_FAILURE;

  out_Result = vIntersection;
  return EZ_SUCCESS;
}

ezResult ezGizmo::GetPointOnAxis(const ezVec3& vStartPos, const ezVec3& vAxis, const ezVec2I32& vScreenPos, const ezMat4& mInvViewProj, ezVec3& out_Result, float* out_pProjectedLength /*= nullptr*/) const
{
  out_Result = vStartPos;

  ezVec3 vPos, vRayDir;
  if (ezGraphicsUtils::ConvertScreenPosToWorldPos(mInvViewProj, 0, 0, m_vViewport.x, m_vViewport.y, ezVec3(vScreenPos.x, vScreenPos.y, 0), vPos, &vRayDir).Failed())
    return EZ_FAILURE;

  const ezVec3 vPlaneTangent = vAxis.CrossRH(m_pCamera->GetDirForwards()).GetNormalized();
  const ezVec3 vPlaneNormal = vAxis.CrossRH(vPlaneTangent);

  ezPlane Plane;
  Plane = ezPlane::MakeFromNormalAndPoint(vPlaneNormal, vStartPos);

  ezVec3 vIntersection;
  if (!Plane.GetRayIntersection(m_pCamera->GetPosition(), vRayDir, nullptr, &vIntersection))
    return EZ_FAILURE;

  const ezVec3 vDirAlongRay = vIntersection - vStartPos;
  const float fProjectedLength = vDirAlongRay.Dot(vAxis);
  if (out_pProjectedLength != nullptr)
  {
    *out_pProjectedLength = fProjectedLength;
  }

  out_Result = vStartPos + fProjectedLength * vAxis;
  return EZ_SUCCESS;
}
