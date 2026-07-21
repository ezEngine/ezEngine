#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Navigation/SteeringUtils.h>
#include <Foundation/Math/Mat3.h>

namespace
{
  // Rebuilds qRotation from vForward alone, choosing right/up so that up is as close to world up
  // (+Z) as geometrically possible - i.e. no roll. vForward is assumed to be normalized.
  void MakeUprightOrientation(ezQuat& inout_qRotation, const ezVec3& vForward)
  {
    ezVec3 vRight = ezVec3::MakeAxisZ().CrossRH(vForward);
    if (vRight.NormalizeIfNotZero(inout_qRotation * ezVec3::MakeAxisY()).Failed())
    {
      // vForward is ~parallel to world up (looking straight up/down) *and* the object's previous
      // right vector happened to be too - fall back to a fixed axis rather than leaving it zero.
      vRight.NormalizeIfNotZero(ezVec3::MakeAxisY()).IgnoreResult();
    }

    ezVec3 vUp = vForward.CrossRH(vRight);
    vUp.NormalizeIfNotZero(ezVec3::MakeAxisZ()).IgnoreResult();

    ezMat3 mRot = ezMat3::MakeIdentity();
    mRot.SetColumn(0, vForward);
    mRot.SetColumn(1, vRight);
    mRot.SetColumn(2, vUp);

    inout_qRotation = ezQuat::MakeFromMat3(mRot);
  }
} // namespace

ezAngle ezAiSteeringUtils::TurnTowards(ezQuat& inout_qRotation, const ezVec3& vTargetDirection, ezAngle maxAngularSpeed, float fTimeDiff)
{
  const ezVec3 vCurrentForward = inout_qRotation * ezVec3::MakeAxisX();

  ezVec3 vNewForward = vCurrentForward;
  ezAngle remaining = ezAngle::MakeFromRadian(0);

  ezVec3 vTargetDir = vTargetDirection;
  if (vTargetDir.NormalizeIfNotZero(ezVec3::MakeZero()).Succeeded())
  {
    const ezAngle angleBetween = vCurrentForward.GetAngleBetween(vTargetDir);

    if (angleBetween > ezAngle::MakeFromDegree(0.01f))
    {
      ezVec3 vRotAxis = vCurrentForward.CrossRH(vTargetDir);
      // Handles the near-180-degree case (cross product is ~zero): fall back to the object's own
      // up axis, so a full reversal turns around a sensible axis regardless of orientation.
      vRotAxis.NormalizeIfNotZero(inout_qRotation * ezVec3::MakeAxisZ()).IgnoreResult();

      const ezAngle maxStep = maxAngularSpeed * fTimeDiff;
      const ezAngle toRotate = ezMath::Min(angleBetween, maxStep);

      const ezQuat qDelta = ezQuat::MakeFromAxisAndAngle(vRotAxis, toRotate);
      vNewForward = qDelta * vCurrentForward;

      remaining = ezMath::Max(angleBetween - toRotate, ezAngle::MakeFromRadian(0));
    }
  }

  // Always re-derive the orientation from the (possibly unchanged) forward direction, so the
  // object stays upright / any existing roll is removed even when it isn't actively turning.
  MakeUprightOrientation(inout_qRotation, vNewForward);

  return remaining;
}

ezAngle ezAiSteeringUtils::GetAngleTowards(const ezQuat& qRotation, const ezVec3& vTargetDirection)
{
  ezVec3 vTargetDir = vTargetDirection;
  if (vTargetDir.NormalizeIfNotZero(ezVec3::MakeZero()).Failed())
    return ezAngle::MakeFromRadian(0);

  const ezVec3 vCurrentForward = qRotation * ezVec3::MakeAxisX();
  return vCurrentForward.GetAngleBetween(vTargetDir);
}

float ezAiSteeringUtils::ApplyAcceleration(float fCurrentSpeed, float fTargetSpeed, float fAcceleration, float fDeceleration, float fTimeDiff)
{
  if (fCurrentSpeed < fTargetSpeed)
  {
    return ezMath::Min(fCurrentSpeed + fAcceleration * fTimeDiff, fTargetSpeed);
  }
  else
  {
    return ezMath::Max(fCurrentSpeed - fDeceleration * fTimeDiff, fTargetSpeed);
  }
}
