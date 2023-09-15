#pragma once

#include <AiPlugin/Navigation/Navigation.h>
#include <Foundation/Math/Angle.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Vec3.h>

/// Work in progress, do not use.
///
/// Attempt to implement a steering behavior.
struct ezAiSteering
{
  ezVec3 m_vPosition = ezVec3::MakeZero();
  ezQuat m_qRotation = ezQuat::MakeIdentity();
  ezVec3 m_vVelocity = ezVec3::MakeZero();
  float m_fMaxSpeed = 6.0f;
  float m_fAcceleration = 5.0f;
  float m_fDecceleration = 10.0f;
  ezAngle m_MinTurnSpeed = ezAngle::MakeFromDegree(180);

  ezAiSteeringInfo m_Info;

  void Calculate(float fTimeDiff, ezDebugRendererContext ctxt);
};
