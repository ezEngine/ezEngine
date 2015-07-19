#include <GameUtils/PCH.h>
#include <GameUtils/Components/SliderComponent.h>

float CalculateAcceleratedMovement(float fDistanceInMeters, float fAcceleration, float fMaxVelocity, float fDeceleration, ezTime& fTimeSinceStartInSec);

EZ_BEGIN_COMPONENT_TYPE(ezSliderComponent, ezTransformComponent, 1, ezSliderComponentManager);
  EZ_BEGIN_PROPERTIES
    EZ_ENUM_MEMBER_PROPERTY("Axis", ezBasisAxis, m_Axis),
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
  m_Axis = ezBasisAxis::PositiveX;
  m_fLastDistance = 0.0f;
}

void ezSliderComponent::Update()
{
  if (m_Flags.IsAnySet(ezTransformComponentFlags::Autorun) && !m_Flags.IsAnySet(ezTransformComponentFlags::Paused))
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

    if (m_Flags.IsAnySet(ezTransformComponentFlags::AnimationReversed))
      m_AnimationTime -= ezClock::Get()->GetTimeDiff();
    else
      m_AnimationTime += ezClock::Get()->GetTimeDiff();

    const float fNewDistance = CalculateAcceleratedMovement(m_fDistanceToTravel, m_fAcceleration, m_fAnimationSpeed, m_fDeceleration, m_AnimationTime);

    const float fDistanceDiff = fNewDistance - m_fLastDistance;

    GetOwner()->SetLocalPosition(GetOwner()->GetLocalPosition() + vAxis * fDistanceDiff);

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

