#include <GameUtils/PCH.h>
#include <GameUtils/Components/RotorComponent.h>

float CalculateAcceleratedMovement(float fDistanceInMeters, float fAcceleration, float fMaxVelocity, float fDeceleration, ezTime& fTimeSinceStartInSec);

EZ_BEGIN_COMPONENT_TYPE(ezRotorComponent, ezTransformComponent, 1, ezRotorComponentManager);
  EZ_BEGIN_PROPERTIES
    EZ_ENUM_MEMBER_PROPERTY("Axis", ezBasisAxis, m_Axis),
    EZ_MEMBER_PROPERTY("Degrees to Rotate", m_iDegreeToRotate),
    EZ_MEMBER_PROPERTY("Acceleration", m_fAcceleration),
    EZ_MEMBER_PROPERTY("Deceleration", m_fDeceleration),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezRotorComponent::ezRotorComponent()
{
  m_LastRotation.SetIdentity();
  m_iDegreeToRotate = 0;
  m_fAcceleration = 1.0f;
  m_fDeceleration = 1.0f;
  m_Axis = ezBasisAxis::PositiveZ;
}

void ezRotorComponent::Update()
{
  if (m_Flags.IsAnySet(ezTransformComponentFlags::Autorun) && !m_Flags.IsAnySet(ezTransformComponentFlags::Paused) && m_fAnimationSpeed > 0.0f)
  {
    ezVec3 vAxis;

    switch (m_Axis)
    {
    case ezBasisAxis::PositiveX:
      vAxis.Set(1, 0, 0);
      break;
    case ezBasisAxis::PositiveY:
      vAxis.Set(0, 1, 0);
      break;
    case ezBasisAxis::PositiveZ:
      vAxis.Set(0, 0, 1);
      break;
    case ezBasisAxis::NegativeX:
      vAxis.Set(-1, 0, 0);
      break;
    case ezBasisAxis::NegativeY:
      vAxis.Set(0, -1, 0);
      break;
    case ezBasisAxis::NegativeZ:
      vAxis.Set(0, 0, -1);
      break;
    }

    if (m_iDegreeToRotate > 0)
    {
      if (m_Flags.IsAnySet(ezTransformComponentFlags::AnimationReversed))
        m_AnimationTime -= ezClock::Get()->GetTimeDiff();
      else
        m_AnimationTime += ezClock::Get()->GetTimeDiff();

      const float fNewDistance = CalculateAcceleratedMovement((float)m_iDegreeToRotate, m_fAcceleration, m_fAnimationSpeed, m_fDeceleration, m_AnimationTime);

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
      m_AnimationTime += ezClock::Get()->GetTimeDiff();

      /// \todo This will probably give precision issues pretty quickly

      ezQuat qRot;
      qRot.SetFromAxisAndAngle(vAxis, ezAngle::Degree(m_fAnimationSpeed * (float)m_AnimationTime.GetSeconds()));

      GetOwner()->SetLocalRotation(qRot * -m_LastRotation * GetOwner()->GetLocalRotation());
      m_LastRotation = qRot;
    }
  }

}



