#include <GameUtils/PCH.h>
#include <GameUtils/Components/SliderComponent.h>

float CalculateAcceleratedMovement(float fDistanceInMeters, float fAcceleration, float fMaxVelocity, float fDeceleration, float fTimeSinceStartInSec);

EZ_BEGIN_COMPONENT_TYPE(ezSliderComponent, ezTransformComponent, 1, ezSliderComponentManager);
  EZ_BEGIN_PROPERTIES
    EZ_ENUM_MEMBER_PROPERTY("Axis", ezTransformComponentAxis, m_Axis),
    EZ_MEMBER_PROPERTY("Distance", m_fDistanceToTravel),
    EZ_MEMBER_PROPERTY("Acceleration", m_fAcceleration),
    EZ_MEMBER_PROPERTY("Deceleration", m_fDeceleration),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezSliderComponent::ezSliderComponent()
{
  m_fDistanceToTravel = 1.0f;
  m_fAcceleration = 0.0f;
  m_fDeceleration = 0.0;
  m_fLastDistance = 0.0f;
}

void ezSliderComponent::Update()
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

    ezTime fTime = m_AnimationTime;

    if (m_Flags.IsAnySet(ezTransformComponentFlags::AnimationReversed))
      fTime -= ezClock::Get()->GetTimeDiff();
    else
      fTime += ezClock::Get()->GetTimeDiff();

    const float fNewDistance = CalculateAcceleratedMovement(m_fDistanceToTravel, m_fAcceleration, m_fAnimationSpeed, m_fDeceleration, (float) fTime.GetSeconds());

    const float fDistanceDiff = fNewDistance - m_fLastDistance;

    ezVec3 vPos = GetOwner()->GetLocalPosition();
    vPos += GetOwner()->GetLocalRotation() * (vAxis * fDistanceDiff);
    GetOwner()->SetLocalPosition(vPos);

    m_AnimationTime = fTime;
    m_fLastDistance = fNewDistance;

    if (!m_Flags.IsAnySet(ezTransformComponentFlags::AnimationReversed))
    {
      if (fNewDistance >= m_fDistanceToTravel)
      {
        if (m_Flags.IsAnySet(ezTransformComponentFlags::AutoReturnEnd))
          m_Flags.Add(ezTransformComponentFlags::AnimationReversed);
        else
        {
          m_Flags.Remove(ezTransformComponentFlags::Autorun);

          if (m_Flags.IsAnySet(ezTransformComponentFlags::AutoToggleDirection))
            m_Flags.Add(ezTransformComponentFlags::AnimationReversed);
        }

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

        //if (PrepareEvent("ANIMATOR_OnReachStart"))
          //RaiseEvent();
      }
    }
  }
}

