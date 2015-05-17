#include <GameUtils/PCH.h>
#include <GameUtils/Components/TransformComponent.h>

float CalculateAcceleratedMovement(float fDistanceInMeters, float fAcceleration, float fMaxVelocity, float fDeceleration, float fTimeSinceStartInSec);

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTransformComponent, ezComponent, 1, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Speed", m_fAnimationSpeed), // How many units per second the animation should do.
    EZ_ACCESSOR_PROPERTY("Run at Startup", GetAnimatingAtStartup, SetAnimatingAtStartup), // Whether the animation should start right away.
    EZ_ACCESSOR_PROPERTY("Reverse at Start", GetAutoReturnStart, SetAutoReturnStart), // If true, it will not stop at the end, but turn around and continue.
    EZ_ACCESSOR_PROPERTY("Reverse at End", GetAutoReturnEnd, SetAutoReturnEnd), // If true, after coming back to the start point, the animation won't stop but turn around and continue.
    EZ_ACCESSOR_PROPERTY("Auto-Toggle Direction", GetAutoToggleDirection, SetAutoToggleDirection), // If true, the animation might stop at start/end points, but set toggle its direction state. Triggering the animation again, means it will run in the reverse direction.
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezTransformComponent::ezTransformComponent()
{
  m_fAnimationSpeed = 1.0f;
  m_AnimationTime.SetZero();
}

void ezTransformComponent::ResumeAnimation()
{
  m_Flags.Add(ezTransformComponentFlags::Autorun);
  m_Flags.Remove(ezTransformComponentFlags::Paused);
}

void ezTransformComponent::SetAnimationPaused(bool bPaused)
{
  m_Flags.AddOrRemove(ezTransformComponentFlags::Paused, bPaused);
}

void ezTransformComponent::SetDirectionForwards(bool bForwards)
{
  m_Flags.AddOrRemove(ezTransformComponentFlags::AnimationReversed, !bForwards);
}

void ezTransformComponent::ReverseDirection()
{
  m_Flags.AddOrRemove(ezTransformComponentFlags::AnimationReversed, !m_Flags.IsAnySet(ezTransformComponentFlags::AnimationReversed));
}

bool ezTransformComponent::IsDirectionForwards() const
{
  return !m_Flags.IsAnySet(ezTransformComponentFlags::AnimationReversed);
}

bool ezTransformComponent::IsAnimationRunning() const
{
  return m_Flags.IsAnySet(ezTransformComponentFlags::Autorun);
}

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezRotorComponentAxis, 1)
  EZ_ENUM_CONSTANTS(ezRotorComponentAxis::PosX, ezRotorComponentAxis::PosY, ezRotorComponentAxis::PosZ, ezRotorComponentAxis::NegX, ezRotorComponentAxis::NegY, ezRotorComponentAxis::NegZ)
EZ_END_STATIC_REFLECTED_ENUM()

EZ_BEGIN_COMPONENT_TYPE(ezRotorTransformComponent, ezTransformComponent, 1, ezRotorTransformComponentManager);
  EZ_BEGIN_PROPERTIES
    EZ_ENUM_MEMBER_PROPERTY("Axis", ezRotorComponentAxis, m_Axis),
    EZ_MEMBER_PROPERTY("Degrees to Rotate", m_iDegreeToRotate),
    EZ_MEMBER_PROPERTY("Acceleration", m_fAcceleration),
    EZ_MEMBER_PROPERTY("Deceleration", m_fDeceleration),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezRotorTransformComponent::ezRotorTransformComponent()
{
  m_LastRotation.SetIdentity();
  m_iDegreeToRotate = 0;
  m_fAcceleration = 1.0f;
  m_fDeceleration = 1.0f;
}

void ezRotorTransformComponent::Update()
{
  if (m_Flags.IsAnySet(ezTransformComponentFlags::Autorun) && !m_Flags.IsAnySet(ezTransformComponentFlags::Paused))
  {
    ezVec3 vAxis;

    switch (m_Axis)
    {
    case ezRotorComponentAxis::PosX:
      vAxis.Set(1, 0, 0);
      break;
    case ezRotorComponentAxis::PosY:
      vAxis.Set(0, 1, 0);
      break;
    case ezRotorComponentAxis::PosZ:
      vAxis.Set(0, 0, 1);
      break;
    case ezRotorComponentAxis::NegX:
      vAxis.Set(-1, 0, 0);
      break;
    case ezRotorComponentAxis::NegY:
      vAxis.Set(0, -1, 0);
      break;
    case ezRotorComponentAxis::NegZ:
      vAxis.Set(0, 0, -1);
      break;
    }

    if (m_iDegreeToRotate > 0)
    {
      ezTime fTime = m_AnimationTime;

      if (m_Flags.IsAnySet(ezTransformComponentFlags::AnimationReversed))
        fTime -= ezClock::Get()->GetTimeDiff();
      else
        fTime += ezClock::Get()->GetTimeDiff();

      const float fNewDistance = CalculateAcceleratedMovement((float)m_iDegreeToRotate, m_fAcceleration, m_fAnimationSpeed, m_fDeceleration, (float) fTime.GetSeconds());

      m_AnimationTime = fTime;

      ezQuat qRotation;
      qRotation.SetFromAxisAndAngle(vAxis, ezAngle::Degree(fNewDistance));

      GetOwner()->SetLocalRotation(qRotation * -m_LastRotation * GetOwner()->GetLocalRotation());

      m_LastRotation = qRotation;

      if (!m_Flags.IsAnySet(ezTransformComponentFlags::AnimationReversed))
      {
        if (fNewDistance >= m_iDegreeToRotate)
        {
          if (m_Flags.IsAnySet(ezTransformComponentFlags::AutoReturnEnd))
            m_Flags.Add(ezTransformComponentFlags::AnimationReversed);
          else
          {
            m_Flags.Remove(ezTransformComponentFlags::Autorun);

            if (m_Flags.IsAnySet(ezTransformComponentFlags::AutoToggleDirection))
              m_Flags.Add(ezTransformComponentFlags::AnimationReversed);
          }

          /// \todo Scripting integration
          //if (PrepareEvent("ANIMATOR_OnReachEnd"))
            //RaiseEvent();
        }
      }
      else
      {
        if (fNewDistance <= 0.0f)
        {
          if (m_Flags.IsAnySet(ezTransformComponentFlags::AutoReturnStart))
            m_Flags.Remove(ezTransformComponentFlags::AnimationReversed);
          else
          {
            m_Flags.Remove(ezTransformComponentFlags::Autorun);

            if (m_Flags.IsAnySet(ezTransformComponentFlags::AutoToggleDirection))
              m_Flags.Remove(ezTransformComponentFlags::AnimationReversed);
          }

          /// \todo Scripting integration
          //if (PrepareEvent("ANIMATOR_OnReachStart"))
            //RaiseEvent();
        }
      }
    }
    else
    {
      ezQuat qRot;
      qRot.SetFromAxisAndAngle(vAxis, ezAngle::Degree(m_fAnimationSpeed * (float) ezClock::Get()->GetTimeDiff().GetSeconds()));

      GetOwner()->SetLocalRotation(qRot * GetOwner()->GetLocalRotation());
    }
  }

}




/*! Distance should be given in meters, but can be anything else, too. E.g. "angles" or "radians". All other values need to use the same units.
For example, when distance is given in angles, acceleration has to be in "angles per square seconds". Deceleration can be positive or negative, internally
the absolute value is used. Distance, acceleration, max velocity and time need to be positive. Time is expected to be "in seconds".
The returned value is 0, if time is negative. It is clamped to fDistanceInMeters, if time is too big.
*/

float CalculateAcceleratedMovement(float fDistanceInMeters, float fAcceleration, float fMaxVelocity, float fDeceleration, float fTimeSinceStartInSec)
{
  // linear motion, if no acceleration or deceleration is present
  if ((fAcceleration <= 0.0f) && (fDeceleration <= 0.0f))
    return ezMath::Clamp(fMaxVelocity * fTimeSinceStartInSec, 0.0f, fDistanceInMeters);

  // do some sanity-checks
  if ((fTimeSinceStartInSec <= 0.0f) || (fMaxVelocity <= 0.0f) || (fDistanceInMeters <= 0.0f))
    return 0.0f;

  // calculate the duration and distance of accelerated movement
  float fAccTime = 0.0f;
  if (fAcceleration > 0.0f)
    fAccTime = fMaxVelocity / fAcceleration;
  float fAccDist = fMaxVelocity * fAccTime * 0.5f;

  // calculate the duration and distance of decelerated movement
  float fDecTime = 0.0f;
  if (fDeceleration > 0.0f)
    fDecTime = fMaxVelocity / fDeceleration;
  float fDecDist = fMaxVelocity * fDecTime * 0.5f;

  // if acceleration and deceleration take longer, than the whole path is long
  if (fAccDist + fDecDist > fDistanceInMeters)
  {
    float fFactor = fDistanceInMeters / (fAccDist + fDecDist);

    // shorten the acceleration path
    if (fAcceleration > 0.0f)
    {
      fAccDist *= fFactor;
      fAccTime = ezMath::Sqrt(2 * fAccDist / fAcceleration);
    }

    // shorten the deceleration path
    if (fDeceleration > 0.0f)
    {
      fDecDist *= fFactor;
      fDecTime = ezMath::Sqrt(2 * fDecDist / fDeceleration);
    }
  }

  // if the time is still within the acceleration phase, return accelerated distance
  if (fTimeSinceStartInSec <= fAccTime)
    return 0.5f * fAcceleration * ezMath::Square(fTimeSinceStartInSec);

  // calculate duration and length of the path, that has maximum velocity
  const float fMaxVelDistance = fDistanceInMeters - (fAccDist + fDecDist);
  const float fMaxVelTime = fMaxVelDistance / fMaxVelocity;

  // if the time is within this phase, return the accelerated path plus the constant velocity path
  if (fTimeSinceStartInSec <= fAccTime + fMaxVelTime)
    return fAccDist + (fTimeSinceStartInSec - fAccTime) * fMaxVelocity;

  // if the time is, however, outside the whole path, just return the upper end
  if (fTimeSinceStartInSec >= fAccTime + fMaxVelTime + fDecTime)
    return fDistanceInMeters;

  // calculate the time into the decelerated movement
  const float fDecTime2 = fTimeSinceStartInSec - (fAccTime + fMaxVelTime);

  // return the distance with the decelerated movement
  return fDistanceInMeters - 0.5f * fDeceleration * ezMath::Square(fDecTime - fDecTime2);
}