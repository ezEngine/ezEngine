#include <GameUtils/PCH.h>
#include <GameUtils/Components/RotorComponent.h>

float CalculateAcceleratedMovement(float fDistanceInMeters, float fAcceleration, float fMaxVelocity, float fDeceleration, float fTimeSinceStartInSec);

EZ_BEGIN_COMPONENT_TYPE(ezRotorComponent, ezTransformComponent, 1, ezRotorComponentManager);
  EZ_BEGIN_PROPERTIES
    EZ_ENUM_MEMBER_PROPERTY("Axis", ezTransformComponentAxis, m_Axis),
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
}

void ezRotorComponent::Update()
{
  if (m_Flags.IsAnySet(ezTransformComponentFlags::Autorun) && !m_Flags.IsAnySet(ezTransformComponentFlags::Paused))
  {
    ezVec3 vAxis;

    switch (m_Axis)
    {
    case ezTransformComponentAxis::PosX:
      vAxis.Set(1, 0, 0);
      break;
    case ezTransformComponentAxis::PosY:
      vAxis.Set(0, 1, 0);
      break;
    case ezTransformComponentAxis::PosZ:
      vAxis.Set(0, 0, 1);
      break;
    case ezTransformComponentAxis::NegX:
      vAxis.Set(-1, 0, 0);
      break;
    case ezTransformComponentAxis::NegY:
      vAxis.Set(0, -1, 0);
      break;
    case ezTransformComponentAxis::NegZ:
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
      m_AnimationTime += ezClock::Get()->GetTimeDiff();

      /// \todo This will probably give precision issues pretty quickly

      ezQuat qRot;
      qRot.SetFromAxisAndAngle(vAxis, ezAngle::Degree(m_fAnimationSpeed * (float)m_AnimationTime.GetSeconds()));

      GetOwner()->SetLocalRotation(qRot * -m_LastRotation * GetOwner()->GetLocalRotation());
      m_LastRotation = qRot;
    }
  }

}



