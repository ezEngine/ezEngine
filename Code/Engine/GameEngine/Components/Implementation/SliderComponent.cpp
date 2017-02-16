#include <PCH.h>
#include <GameEngine/Components/SliderComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

float CalculateAcceleratedMovement(float fDistanceInMeters, float fAcceleration, float fMaxVelocity, float fDeceleration, ezTime& fTimeSinceStartInSec);

EZ_BEGIN_COMPONENT_TYPE(ezSliderComponent, 2)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Axis", ezBasisAxis, m_Axis),
    EZ_MEMBER_PROPERTY("Distance", m_fDistanceToTravel),
    EZ_MEMBER_PROPERTY("Acceleration", m_fAcceleration),
    EZ_MEMBER_PROPERTY("Deceleration", m_fDeceleration),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

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
      m_AnimationTime -= GetWorld()->GetClock().GetTimeDiff();
    else
      m_AnimationTime += GetWorld()->GetClock().GetTimeDiff();

    const float fNewDistance = CalculateAcceleratedMovement(m_fDistanceToTravel, m_fAcceleration, m_fAnimationSpeed, m_fDeceleration, m_AnimationTime);

    const float fDistanceDiff = fNewDistance - m_fLastDistance;

    GetOwner()->SetLocalPosition(GetOwner()->GetLocalPosition() + GetOwner()->GetLocalRotation() * vAxis * fDistanceDiff);

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



void ezSliderComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_fDistanceToTravel;
  s << m_fAcceleration;
  s << m_fDeceleration;
  s << m_Axis.GetValue();
  s << m_fLastDistance;
}


void ezSliderComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_fDistanceToTravel;
  s >> m_fAcceleration;
  s >> m_fDeceleration;
  ezBasisAxis::StorageType axis;
  s >> axis;
  m_Axis.SetValue(axis);
  s >> m_fLastDistance;

}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezSliderComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezSliderComponentPatch_1_2()
    : ezGraphPatch(ezGetStaticRTTI<ezSliderComponent>(), 2) {}

  virtual void Patch(ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    // Base class
    PatchBaseClass(pGraph, pNode, ezGetStaticRTTI<ezTransformComponent>(), 2);
  }
};

ezSliderComponentPatch_1_2 g_ezSliderComponentPatch_1_2;


EZ_STATICLINK_FILE(GameUtils, GameUtils_Components_SliderComponent);

