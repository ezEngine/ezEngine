#include <AiPlugin/Navigation/Steering.h>

void ezAiSteering::Calculate(float fTimeDiff, ezDebugRendererContext ctxt)
{
  const float fRunSpeed = m_fMaxSpeed;
  const float fJogSpeed = m_fMaxSpeed * 0.75f;
  const float fWalkSpeed = m_fMaxSpeed * 0.5f;

  const float fBrakingDistanceRun = 1.2f * (ezMath::Square(fRunSpeed) / (2.0f * m_fDecceleration));
  const float fBrakingDistanceJog = 1.2f * (ezMath::Square(fJogSpeed) / (2.0f * m_fDecceleration));
  const float fBrakingDistanceWalk = 1.2f * (ezMath::Square(fWalkSpeed) / (2.0f * m_fDecceleration));

  if (m_Info.m_fArrivalDistance <= fBrakingDistanceWalk)
    m_fMaxSpeed = 0.0f;
  else if (m_Info.m_fArrivalDistance < fBrakingDistanceJog)
    m_fMaxSpeed = ezMath::Min(m_fMaxSpeed, fWalkSpeed);
  else if (m_Info.m_fArrivalDistance < fBrakingDistanceRun)
    m_fMaxSpeed = ezMath::Min(m_fMaxSpeed, fJogSpeed);

  if (m_Info.m_AbsRotationTowardsWaypoint > ezAngle::MakeFromDegree(80))
    m_fMaxSpeed = 0.0f;
  else if (m_Info.m_AbsRotationTowardsWaypoint > ezAngle::MakeFromDegree(55))
    m_fMaxSpeed = ezMath::Min(m_fMaxSpeed, fWalkSpeed);
  else if (m_Info.m_AbsRotationTowardsWaypoint > ezAngle::MakeFromDegree(30))
    m_fMaxSpeed = ezMath::Min(m_fMaxSpeed, fJogSpeed);

  ezAngle maxRotation = m_Info.m_AbsRotationTowardsWaypoint;

  if (m_Info.m_fDistanceToWaypoint <= fBrakingDistanceRun)
  {
    if (m_Info.m_MaxAbsRotationAfterWaypoint > ezAngle::MakeFromDegree(40))
      m_fMaxSpeed = ezMath::Min(m_fMaxSpeed, fWalkSpeed);
    else if (m_Info.m_MaxAbsRotationAfterWaypoint > ezAngle::MakeFromDegree(20))
      m_fMaxSpeed = ezMath::Min(m_fMaxSpeed, fJogSpeed);

    maxRotation = ezMath::Max(maxRotation, m_Info.m_MaxAbsRotationAfterWaypoint);
  }

  ezVec3 vLookDir = m_qRotation * ezVec3::MakeAxisX();
  vLookDir.z = 0;
  vLookDir.Normalize();

  float fCurSpeed = m_vVelocity.GetAsVec2().GetLength();

  ezAngle turnSpeed = m_MinTurnSpeed;

  {
    const float fTurnRadius = 1.0f; // ezMath::Clamp(m_Info.m_fWaypointCorridorWidth, 0.5f, 5.0f);
    const float fCircumference = 2.0f * ezMath::Pi<float>() * fTurnRadius;
    const float fCircleFraction = maxRotation / ezAngle::MakeFromDegree(360);
    const float fTurnDistance = fCircumference * fCircleFraction;
    const float fTurnDuration = fTurnDistance / fCurSpeed;

    turnSpeed = ezMath::Max(m_MinTurnSpeed, maxRotation / fTurnDuration);
  }

  // ezDebugRenderer::DrawInfoText(ctxt, ezDebugRenderer::ScreenPlacement::BottomLeft, "Steering", ezFmt("Turn Speed: {}", turnSpeed));
  // ezDebugRenderer::DrawInfoText(ctxt, ezDebugRenderer::ScreenPlacement::BottomLeft, "Steering", ezFmt("Corridor Width: {}", m_Info.m_fWaypointCorridorWidth));

  if (!m_Info.m_vDirectionTowardsWaypoint.IsZero())
  {
    const ezVec3 vTargetDir = m_Info.m_vDirectionTowardsWaypoint.GetAsVec3(0);
    ezVec3 vRotAxis = vLookDir.CrossRH(vTargetDir);
    vRotAxis.NormalizeIfNotZero(ezVec3::MakeAxisZ()).IgnoreResult();
    const ezAngle toRotate = ezMath::Min(vLookDir.GetAngleBetween(vTargetDir), fTimeDiff * turnSpeed);
    const ezQuat qRot = ezQuat::MakeFromAxisAndAngle(vRotAxis, toRotate);

    vLookDir = qRot * vLookDir;

    m_qRotation = ezQuat::MakeShortestRotation(ezVec3::MakeAxisX(), vLookDir);
  }


  if (fCurSpeed < m_fMaxSpeed)
  {
    fCurSpeed += fTimeDiff * m_fAcceleration;
    fCurSpeed = ezMath::Min(fCurSpeed, m_fMaxSpeed);
  }
  else if (fCurSpeed > m_fMaxSpeed)
  {
    fCurSpeed -= fTimeDiff * m_fDecceleration;
    fCurSpeed = ezMath::Max(fCurSpeed, m_fMaxSpeed);
  }

  ezVec3 vDir = vLookDir;
  vDir *= fCurSpeed;
  vDir *= fTimeDiff;
  m_vPosition += vDir;
}
