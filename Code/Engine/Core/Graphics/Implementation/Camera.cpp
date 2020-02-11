#include <CorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <World/CoordinateSystem.h>

class RemapCoordinateSystemProvider : public ezCoordinateSystemProvider
{
public:
  RemapCoordinateSystemProvider()
    : ezCoordinateSystemProvider(nullptr)
  {
  }

  virtual void GetCoordinateSystem(const ezVec3& vGlobalPosition, ezCoordinateSystem& out_CoordinateSystem) const override
  {
    out_CoordinateSystem.m_vForwardDir = ezBasisAxis::GetBasisVector(m_ForwardAxis);
    out_CoordinateSystem.m_vRightDir = ezBasisAxis::GetBasisVector(m_RightAxis);
    out_CoordinateSystem.m_vUpDir = ezBasisAxis::GetBasisVector(m_UpAxis);
  }

  ezBasisAxis::Enum m_ForwardAxis = ezBasisAxis::PositiveX;
  ezBasisAxis::Enum m_RightAxis = ezBasisAxis::PositiveY;
  ezBasisAxis::Enum m_UpAxis = ezBasisAxis::PositiveZ;
};

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezCameraMode, 1)
  EZ_ENUM_CONSTANT(ezCameraMode::PerspectiveFixedFovX),
  EZ_ENUM_CONSTANT(ezCameraMode::PerspectiveFixedFovY),
  EZ_ENUM_CONSTANT(ezCameraMode::OrthoFixedWidth),
  EZ_ENUM_CONSTANT(ezCameraMode::OrthoFixedHeight),
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

ezCamera::ezCamera()
{
  m_vCameraPosition[0].SetZero();
  m_vCameraPosition[1].SetZero();
  m_mViewMatrix[0].SetIdentity();
  m_mViewMatrix[1].SetIdentity();
  m_mStereoProjectionMatrix[0].SetIdentity();
  m_mStereoProjectionMatrix[1].SetIdentity();

  SetCoordinateSystem(ezBasisAxis::PositiveX, ezBasisAxis::PositiveY, ezBasisAxis::PositiveZ);
}

void ezCamera::SetCoordinateSystem(ezBasisAxis::Enum forwardAxis, ezBasisAxis::Enum rightAxis, ezBasisAxis::Enum upAxis)
{
  auto provider = EZ_DEFAULT_NEW(RemapCoordinateSystemProvider);
  provider->m_ForwardAxis = forwardAxis;
  provider->m_RightAxis = rightAxis;
  provider->m_UpAxis = upAxis;

  m_CoordinateSystem = provider;
}

void ezCamera::SetCoordinateSystem(const ezSharedPtr<ezCoordinateSystemProvider>& provider)
{
  m_CoordinateSystem = provider;
}

ezVec3 ezCamera::GetPosition(ezCameraEye eye) const
{
  return MapInternalToExternal(m_vCameraPosition[static_cast<int>(eye)]);
}

ezVec3 ezCamera::GetDirForwards(ezCameraEye eye) const
{
  ezVec3 decFwd, decRight, decUp, decPos;
  ezGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], ezHandedness::LeftHanded);

  return MapInternalToExternal(decFwd);
}

ezVec3 ezCamera::GetDirUp(ezCameraEye eye) const
{
  ezVec3 decFwd, decRight, decUp, decPos;
  ezGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], ezHandedness::LeftHanded);

  return MapInternalToExternal(decUp);
}

ezVec3 ezCamera::GetDirRight(ezCameraEye eye) const
{
  ezVec3 decFwd, decRight, decUp, decPos;
  ezGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], ezHandedness::LeftHanded);

  return MapInternalToExternal(decRight);
}

ezVec3 ezCamera::InternalGetPosition(ezCameraEye eye) const
{
  return m_vCameraPosition[static_cast<int>(eye)];
}

ezVec3 ezCamera::InternalGetDirForwards(ezCameraEye eye) const
{
  ezVec3 decFwd, decRight, decUp, decPos;
  ezGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], ezHandedness::LeftHanded);

  return decFwd;
}

ezVec3 ezCamera::InternalGetDirUp(ezCameraEye eye) const
{
  ezVec3 decFwd, decRight, decUp, decPos;
  ezGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], ezHandedness::LeftHanded);

  return decUp;
}

ezVec3 ezCamera::InternalGetDirRight(ezCameraEye eye) const
{
  ezVec3 decFwd, decRight, decUp, decPos;
  ezGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], ezHandedness::LeftHanded);

  return -decRight;
}

ezVec3 ezCamera::MapExternalToInternal(const ezVec3& v) const
{
  if (m_CoordinateSystem)
  {
    ezCoordinateSystem system;
    m_CoordinateSystem->GetCoordinateSystem(m_vCameraPosition[0], system);

    ezMat3 m;
    m.SetRow(0, system.m_vForwardDir);
    m.SetRow(1, system.m_vRightDir);
    m.SetRow(2, system.m_vUpDir);

    return m * v;
  }

  return v;
}

ezVec3 ezCamera::MapInternalToExternal(const ezVec3& v) const
{
  if (m_CoordinateSystem)
  {
    ezCoordinateSystem system;
    m_CoordinateSystem->GetCoordinateSystem(m_vCameraPosition[0], system);

    ezMat3 m;
    m.SetColumn(0, system.m_vForwardDir);
    m.SetColumn(1, system.m_vRightDir);
    m.SetColumn(2, system.m_vUpDir);

    return m * v;
  }

  return v;
}

ezAngle ezCamera::GetFovX(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == ezCameraMode::PerspectiveFixedFovX)
    return ezAngle::Degree(m_fFovOrDim);

  if (m_Mode == ezCameraMode::PerspectiveFixedFovY)
    return ezMath::ATan(ezMath::Tan(ezAngle::Degree(m_fFovOrDim) * 0.5f) * fAspectRatioWidthDivHeight) * 2.0f;

  // TODO: HACK
  if (m_Mode == ezCameraMode::Stereo)
    return ezAngle::Degree(90);

  EZ_REPORT_FAILURE("You cannot get the camera FOV when it is not a perspective camera.");
  return ezAngle();
}

ezAngle ezCamera::GetFovY(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == ezCameraMode::PerspectiveFixedFovX)
    return ezMath::ATan(ezMath::Tan(ezAngle::Degree(m_fFovOrDim) * 0.5f) / fAspectRatioWidthDivHeight) * 2.0f;

  if (m_Mode == ezCameraMode::PerspectiveFixedFovY)
    return ezAngle::Degree(m_fFovOrDim);

  // TODO: HACK
  if (m_Mode == ezCameraMode::Stereo)
    return ezAngle::Degree(90);

  EZ_REPORT_FAILURE("You cannot get the camera FOV when it is not a perspective camera.");
  return ezAngle();
}


float ezCamera::GetDimensionX(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == ezCameraMode::OrthoFixedWidth)
    return m_fFovOrDim;

  if (m_Mode == ezCameraMode::OrthoFixedHeight)
    return m_fFovOrDim * fAspectRatioWidthDivHeight;

  EZ_REPORT_FAILURE("You cannot get the camera dimensions when it is not an orthographic camera.");
  return 0;
}


float ezCamera::GetDimensionY(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == ezCameraMode::OrthoFixedWidth)
    return m_fFovOrDim / fAspectRatioWidthDivHeight;

  if (m_Mode == ezCameraMode::OrthoFixedHeight)
    return m_fFovOrDim;

  EZ_REPORT_FAILURE("You cannot get the camera dimensions when it is not an orthographic camera.");
  return 0;
}

void ezCamera::SetCameraMode(ezCameraMode::Enum Mode, float fFovOrDim, float fNearPlane, float fFarPlane)
{
  // early out if no change
  if (m_Mode == Mode && m_fFovOrDim == fFovOrDim && m_fNearPlane == fNearPlane && m_fFarPlane == fFarPlane)
  {
    return;
  }

  m_Mode = Mode;
  m_fFovOrDim = fFovOrDim;
  m_fNearPlane = fNearPlane;
  m_fFarPlane = fFarPlane;

  m_fAspectOfPrecomputedStereoProjection = -1.0f;

  CameraSettingsChanged();
}

void ezCamera::SetStereoProjection(const ezMat4& mProjectionLeftEye, const ezMat4& mProjectionRightEye, float fAspectRatioWidthDivHeight)
{
  m_mStereoProjectionMatrix[static_cast<int>(ezCameraEye::Left)] = mProjectionLeftEye;
  m_mStereoProjectionMatrix[static_cast<int>(ezCameraEye::Right)] = mProjectionRightEye;
  m_fAspectOfPrecomputedStereoProjection = fAspectRatioWidthDivHeight;

  CameraSettingsChanged();
}

void ezCamera::LookAt(const ezVec3& vCameraPos0, const ezVec3& vTargetPos0, const ezVec3& vUp0)
{
  const ezVec3 vCameraPos = MapExternalToInternal(vCameraPos0);
  const ezVec3 vTargetPos = MapExternalToInternal(vTargetPos0);
  const ezVec3 vUp = MapExternalToInternal(vUp0);

  if (m_Mode == ezCameraMode::Stereo)
  {
    EZ_REPORT_FAILURE("ezCamera::LookAt is not possible for stereo cameras.");
    return;
  }

  m_mViewMatrix[0] = ezGraphicsUtils::CreateLookAtViewMatrix(vCameraPos, vTargetPos, vUp, ezHandedness::LeftHanded);
  m_mViewMatrix[1] = m_mViewMatrix[0];
  m_vCameraPosition[1] = m_vCameraPosition[0] = vCameraPos;

  CameraOrientationChanged(true, true);
}

void ezCamera::SetViewMatrix(const ezMat4& mLookAtMatrix, ezCameraEye eye)
{
  const int iEyeIdx = static_cast<int>(eye);

  m_mViewMatrix[iEyeIdx] = mLookAtMatrix;

  ezVec3 decFwd, decRight, decUp;
  ezGraphicsUtils::DecomposeViewMatrix(m_vCameraPosition[iEyeIdx], decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], ezHandedness::LeftHanded);

  if (m_Mode != ezCameraMode::Stereo)
  {
    m_mViewMatrix[1 - iEyeIdx] = m_mViewMatrix[iEyeIdx];
    m_vCameraPosition[1 - iEyeIdx] = m_vCameraPosition[iEyeIdx];
  }

  CameraOrientationChanged(true, true);
}

void ezCamera::GetProjectionMatrix(
  float fAspectRatioWidthDivHeight, ezMat4& out_projectionMatrix, ezCameraEye eye, ezClipSpaceDepthRange::Enum depthRange) const
{
  switch (m_Mode)
  {
    case ezCameraMode::PerspectiveFixedFovX:
      out_projectionMatrix = ezGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(ezAngle::Degree(m_fFovOrDim),
        fAspectRatioWidthDivHeight, m_fNearPlane, m_fFarPlane, depthRange, ezClipSpaceYMode::Regular, ezHandedness::LeftHanded);
      break;

    case ezCameraMode::PerspectiveFixedFovY:
      out_projectionMatrix = ezGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(ezAngle::Degree(m_fFovOrDim),
        fAspectRatioWidthDivHeight, m_fNearPlane, m_fFarPlane, depthRange, ezClipSpaceYMode::Regular, ezHandedness::LeftHanded);
      break;

    case ezCameraMode::OrthoFixedWidth:
      out_projectionMatrix = ezGraphicsUtils::CreateOrthographicProjectionMatrix(m_fFovOrDim, m_fFovOrDim / fAspectRatioWidthDivHeight,
        m_fNearPlane, m_fFarPlane, depthRange, ezClipSpaceYMode::Regular, ezHandedness::LeftHanded);
      break;

    case ezCameraMode::OrthoFixedHeight:
      out_projectionMatrix = ezGraphicsUtils::CreateOrthographicProjectionMatrix(m_fFovOrDim * fAspectRatioWidthDivHeight, m_fFovOrDim,
        m_fNearPlane, m_fFarPlane, depthRange, ezClipSpaceYMode::Regular, ezHandedness::LeftHanded);
      break;

    case ezCameraMode::Stereo:
      if (ezMath::IsEqual(m_fAspectOfPrecomputedStereoProjection, fAspectRatioWidthDivHeight, ezMath::LargeEpsilon<float>()))
        out_projectionMatrix = m_mStereoProjectionMatrix[static_cast<int>(eye)];
      else
      {
        // Evade to FixedFovY
        out_projectionMatrix = ezGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(ezAngle::Degree(m_fFovOrDim),
          fAspectRatioWidthDivHeight, m_fNearPlane, m_fFarPlane, depthRange, ezClipSpaceYMode::Regular, ezHandedness::LeftHanded);
      }
      break;

    default:
      EZ_REPORT_FAILURE("Invalid Camera Mode {0}", (int)m_Mode);
  }
}

void ezCamera::CameraSettingsChanged()
{
  EZ_ASSERT_DEV(m_Mode != ezCameraMode::None, "Invalid Camera Mode.");
  EZ_ASSERT_DEV(m_fNearPlane < m_fFarPlane, "Near and Far Plane are invalid.");
  EZ_ASSERT_DEV(m_fFovOrDim > 0.0f, "FOV or Camera Dimension is invalid.");

  ++m_uiSettingsModificationCounter;
}

void ezCamera::MoveLocally(float fForward, float fRight, float fUp)
{
  m_mViewMatrix[0].SetTranslationVector(m_mViewMatrix[0].GetTranslationVector() - ezVec3(fRight, fUp, fForward));
  m_mViewMatrix[1].SetTranslationVector(m_mViewMatrix[0].GetTranslationVector());

  ezVec3 decFwd, decRight, decUp, decPos;
  ezGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[0], ezHandedness::LeftHanded);

  m_vCameraPosition[0] = m_vCameraPosition[1] = decPos;

  CameraOrientationChanged(true, false);
}

void ezCamera::MoveGlobally(float fForward, float fRight, float fUp)
{
  ezVec3 vMove(fForward, fRight, fUp);

  ezVec3 decFwd, decRight, decUp, decPos;
  ezGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[0], ezHandedness::LeftHanded);

  m_vCameraPosition[0] += vMove;
  m_vCameraPosition[1] = m_vCameraPosition[0];

  m_mViewMatrix[0] = ezGraphicsUtils::CreateViewMatrix(m_vCameraPosition[0], decFwd, decRight, decUp, ezHandedness::LeftHanded);

  m_mViewMatrix[1].SetTranslationVector(m_mViewMatrix[0].GetTranslationVector());

  CameraOrientationChanged(true, false);
}

void ezCamera::ClampRotationAngles(bool bLocalSpace, ezAngle& forwardAxis, ezAngle& rightAxis, ezAngle& upAxis)
{
  if (bLocalSpace)
  {
    if (rightAxis.GetRadian() != 0.0f)
    {
      // Limit how much the camera can look up and down, to prevent it from overturning

      const float fDot = InternalGetDirForwards().Dot(ezVec3(0, 0, -1));
      const ezAngle fCurAngle = ezMath::ACos(fDot) - ezAngle::Degree(90.0f);
      const ezAngle fNewAngle = fCurAngle + rightAxis;

      const ezAngle fAllowedAngle = ezMath::Clamp(fNewAngle, ezAngle::Degree(-85.0f), ezAngle::Degree(85.0f));

      rightAxis = fAllowedAngle - fCurAngle;
    }
  }
}

void ezCamera::RotateLocally(ezAngle forwardAxis, ezAngle rightAxis, ezAngle upAxis)
{
  ClampRotationAngles(true, forwardAxis, rightAxis, upAxis);

  ezVec3 vDirForwards = InternalGetDirForwards();
  ezVec3 vDirUp = InternalGetDirUp();
  ezVec3 vDirRight = InternalGetDirRight();

  if (forwardAxis.GetRadian() != 0.0f)
  {
    ezMat3 m;
    m.SetRotationMatrix(vDirForwards, forwardAxis);

    vDirUp = m * vDirUp;
    vDirRight = m * vDirRight;
  }

  if (rightAxis.GetRadian() != 0.0f)
  {
    ezMat3 m;
    m.SetRotationMatrix(vDirRight, rightAxis);

    vDirUp = m * vDirUp;
    vDirForwards = m * vDirForwards;
  }

  if (upAxis.GetRadian() != 0.0f)
  {
    ezMat3 m;
    m.SetRotationMatrix(vDirUp, upAxis);

    vDirRight = m * vDirRight;
    vDirForwards = m * vDirForwards;
  }

  // Using ezGraphicsUtils::CreateLookAtViewMatrix is not only easier, it also has the advantage that we end up always with orthonormal
  // vectors.
  auto vPos = InternalGetPosition();
  m_mViewMatrix[0] = ezGraphicsUtils::CreateLookAtViewMatrix(vPos, vPos + vDirForwards, vDirUp, ezHandedness::LeftHanded);
  m_mViewMatrix[1] = m_mViewMatrix[0];

  CameraOrientationChanged(false, true);
}

void ezCamera::RotateGlobally(ezAngle forwardAxis, ezAngle rightAxis, ezAngle upAxis)
{
  ClampRotationAngles(false, forwardAxis, rightAxis, upAxis);

  ezVec3 vDirForwards = InternalGetDirForwards();
  ezVec3 vDirUp = InternalGetDirUp();

  if (forwardAxis.GetRadian() != 0.0f)
  {
    ezMat3 m;
    m.SetRotationMatrixX(forwardAxis);

    vDirUp = m * vDirUp;
    vDirForwards = m * vDirForwards;
  }

  if (rightAxis.GetRadian() != 0.0f)
  {
    ezMat3 m;
    m.SetRotationMatrixY(rightAxis);

    vDirUp = m * vDirUp;
    vDirForwards = m * vDirForwards;
  }

  if (upAxis.GetRadian() != 0.0f)
  {
    ezMat3 m;
    m.SetRotationMatrixZ(upAxis);

    vDirUp = m * vDirUp;
    vDirForwards = m * vDirForwards;
  }

  // Using ezGraphicsUtils::CreateLookAtViewMatrix is not only easier, it also has the advantage that we end up always with orthonormal
  // vectors.
  auto vPos = InternalGetPosition();
  m_mViewMatrix[0] = ezGraphicsUtils::CreateLookAtViewMatrix(vPos, vPos + vDirForwards, vDirUp, ezHandedness::LeftHanded);
  m_mViewMatrix[1] = m_mViewMatrix[0];

  CameraOrientationChanged(false, true);
}



EZ_STATICLINK_FILE(Core, Core_Graphics_Implementation_Camera);
