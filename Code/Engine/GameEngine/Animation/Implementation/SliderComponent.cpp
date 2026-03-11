#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/SliderComponent.h>

float CalculateAcceleratedMovement(
  float fDistanceInMeters, float fAcceleration, float fMaxVelocity, float fDeceleration, ezTime& ref_timeSinceStartInSec);

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSliderComponent, 3, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Axis", ezBasisAxis, m_Axis)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::PositiveZ)),
    EZ_MEMBER_PROPERTY("Distance", m_fDistanceToTravel)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("Acceleration", m_fAcceleration)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Deceleration", m_fDeceleration)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("RandomStart", m_RandomStart)->AddAttributes(new ezClampValueAttribute(ezTime::MakeZero(), ezVariant())),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezDirectionVisualizerAttribute("Axis", 1.0, ezColor::MediumPurple, nullptr, "Distance")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSliderComponent::ezSliderComponent() = default;
ezSliderComponent::~ezSliderComponent() = default;

void ezSliderComponent::Update()
{
  if (m_Flags.IsAnySet(ezTransformComponentFlags::CurrentlyRunning))
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
        if (!m_Flags.IsSet(ezTransformComponentFlags::AutoReturnEnd))
        {
          m_Flags.Remove(ezTransformComponentFlags::CurrentlyRunning);
        }

        m_Flags.Add(ezTransformComponentFlags::AnimationReversed);

        // if (PrepareEvent("ANIMATOR_OnReachEnd"))
        // RaiseEvent();
      }
    }
    else
    {
      if (fNewDistance <= 0.0f)
      {
        if (!m_Flags.IsSet(ezTransformComponentFlags::AutoReturnStart))
        {
          m_Flags.Remove(ezTransformComponentFlags::CurrentlyRunning);
        }

        m_Flags.Remove(ezTransformComponentFlags::AnimationReversed);

        // if (PrepareEvent("ANIMATOR_OnReachStart"))
        // RaiseEvent();
      }
    }
  }
}

void ezSliderComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // reset to start state
  m_fLastDistance = 0.0f;

  if (m_RandomStart.IsPositive())
  {
    m_AnimationTime = ezTime::MakeFromSeconds(GetWorld()->GetRandomNumberGenerator().DoubleMinMax(0.0, m_RandomStart.GetSeconds()));
  }
}

void ezSliderComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_fDistanceToTravel;
  s << m_fAcceleration;
  s << m_fDeceleration;
  s << m_Axis.GetValue();
  s << m_fLastDistance;
  s << m_RandomStart;
}


void ezSliderComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_fDistanceToTravel;
  s >> m_fAcceleration;
  s >> m_fDeceleration;
  s >> m_Axis;
  s >> m_fLastDistance;

  if (uiVersion >= 3)
  {
    s >> m_RandomStart;
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezSliderComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezSliderComponentPatch_1_2()
    : ezGraphPatch("ezSliderComponent", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    // Base class
    ref_context.PatchBaseClass("ezTransformComponent", 2, true);
  }
};

ezSliderComponentPatch_1_2 g_ezSliderComponentPatch_1_2;


EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_SliderComponent);
