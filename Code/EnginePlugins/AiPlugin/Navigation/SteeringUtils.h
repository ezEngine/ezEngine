#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <Foundation/Math/Angle.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Vec3.h>

/// Small, stateless steering math helpers shared by AI navigation components.
///
/// The forward direction convention matches the rest of the engine's AI code: the local +X axis
/// is "forward" (see ezQuat::MakeShortestRotation(ezVec3::MakeAxisX(), ...) usage throughout
/// AiPlugin).
struct EZ_AIPLUGIN_DLL ezAiSteeringUtils
{
  /// Rotates inout_qRotation so its forward axis turns towards vTargetDirection, at most by
  /// maxAngularSpeed * fTimeDiff.
  ///
  /// vTargetDirection does not need to be normalized. A zero vector leaves the forward direction
  /// unchanged. The resulting orientation always has its local up axis (+Z) aligned as closely as
  /// possible with world up (+Z) - i.e. it never rolls - regardless of whether a zero-angle target
  /// was given, so this also re-levels any existing roll on inout_qRotation every time it is
  /// called (callers that want banking need to apply additional roll themselves, see e.g.
  /// ezAiVoxelNavigationComponent's path-following code).
  ///
  /// Returns the remaining angle between the new forward direction and vTargetDirection after the
  /// clamped step (zero once fully turned), so callers can decide e.g. whether they are facing
  /// closely enough towards the target to start moving.
  static ezAngle TurnTowards(ezQuat& inout_qRotation, const ezVec3& vTargetDirection, ezAngle maxAngularSpeed, float fTimeDiff);

  /// Peek-only companion to TurnTowards(): returns the angle between qRotation's forward axis and
  /// vTargetDirection, without mutating anything.
  static ezAngle GetAngleTowards(const ezQuat& qRotation, const ezVec3& vTargetDirection);

  /// Moves fCurrentSpeed towards fTargetSpeed, using fAcceleration when speeding up and
  /// fDeceleration when slowing down (both expected to be >= 0), clamped by fTimeDiff.
  static float ApplyAcceleration(float fCurrentSpeed, float fTargetSpeed, float fAcceleration, float fDeceleration, float fTimeDiff);
};
