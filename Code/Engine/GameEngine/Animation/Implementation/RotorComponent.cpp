#include <GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Animation/RotorComponent.h>

float CalculateAcceleratedMovement(
  float fDistanceInMeters, float fAcceleration, float fMaxVelocity, float fDeceleration, ezTime& fTimeSinceStartInSec);

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezRotorComponent, 3, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Axis", ezBasisAxis, m_Axis),
    EZ_MEMBER_PROPERTY("AxisDeviation", m_AxisDeviation)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(-180), ezAngle::Degree(180))),
    EZ_MEMBER_PROPERTY("DegreesToRotate", m_iDegreeToRotate),
    EZ_MEMBER_PROPERTY("Acceleration", m_fAcceleration),
    EZ_MEMBER_PROPERTY("Deceleration", m_fDeceleration),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezRotorComponent::ezRotorComponent() = default;
ezRotorComponent::~ezRotorComponent() = default;

void ezRotorComponent::Update()
{
  if (m_Flags.IsAnySet(ezTransformComponentFlags::Running) &&
      m_fAnimationSpeed > 0.0f)
  {
    if (m_Flags.IsAnySet(ezTransformComponentFlags::AnimationReversed))
      m_AnimationTime -= GetWorld()->GetClock().GetTimeDiff();
    else
      m_AnimationTime += GetWorld()->GetClock().GetTimeDiff();

    if (m_iDegreeToRotate > 0)
    {
      const float fNewDistance =
        CalculateAcceleratedMovement((float)m_iDegreeToRotate, m_fAcceleration, m_fAnimationSpeed, m_fDeceleration, m_AnimationTime);

      ezQuat qRotation;
      qRotation.SetFromAxisAndAngle(m_vRotationAxis, ezAngle::Degree(fNewDistance));

      GetOwner()->SetLocalRotation(GetOwner()->GetLocalRotation() * -m_LastRotation * qRotation);

      m_LastRotation = qRotation;

      if (!m_Flags.IsAnySet(ezTransformComponentFlags::AnimationReversed))
      {
        if (fNewDistance >= m_iDegreeToRotate)
        {
          if (!m_Flags.IsSet(ezTransformComponentFlags::AutoReturnEnd))
          {
            m_Flags.Remove(ezTransformComponentFlags::Running);
          }

          m_Flags.Add(ezTransformComponentFlags::AnimationReversed);

          /// \todo Scripting integration
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
            m_Flags.Remove(ezTransformComponentFlags::Running);
          }

          m_Flags.Remove(ezTransformComponentFlags::AnimationReversed);

          /// \todo Scripting integration
          // if (PrepareEvent("ANIMATOR_OnReachStart"))
          // RaiseEvent();
        }
      }
    }
    else
    {
      /// \todo This will probably give precision issues pretty quickly

      ezQuat qRotation;
      qRotation.SetFromAxisAndAngle(m_vRotationAxis, ezAngle::Degree(m_fAnimationSpeed * GetWorld()->GetClock().GetTimeDiff().GetSeconds()));

      GetOwner()->SetLocalRotation(GetOwner()->GetLocalRotation() * qRotation);
    }
  }
}

void ezRotorComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_iDegreeToRotate;
  s << m_fAcceleration;
  s << m_fDeceleration;
  s << m_Axis.GetValue();
  s << m_LastRotation;
  s << m_AxisDeviation;
}


void ezRotorComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_iDegreeToRotate;
  s >> m_fAcceleration;
  s >> m_fDeceleration;
  s >> m_Axis;
  s >> m_LastRotation;

  if (uiVersion >= 3)
  {
    s >> m_AxisDeviation;
  }
}

void ezRotorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  switch (m_Axis)
  {
    case ezBasisAxis::PositiveX:
      m_vRotationAxis.Set(1, 0, 0);
      break;
    case ezBasisAxis::PositiveY:
      m_vRotationAxis.Set(0, 1, 0);
      break;
    case ezBasisAxis::PositiveZ:
      m_vRotationAxis.Set(0, 0, 1);
      break;
    case ezBasisAxis::NegativeX:
      m_vRotationAxis.Set(-1, 0, 0);
      break;
    case ezBasisAxis::NegativeY:
      m_vRotationAxis.Set(0, -1, 0);
      break;
    case ezBasisAxis::NegativeZ:
      m_vRotationAxis.Set(0, 0, -1);
      break;
  }

  if (m_AxisDeviation.GetRadian() != 0.0f)
  {
    if (m_AxisDeviation > ezAngle::Degree(179))
    {
      m_vRotationAxis = ezVec3::CreateRandomDirection(GetWorld()->GetRandomNumberGenerator());
    }
    else
    {
      m_vRotationAxis = ezVec3::CreateRandomDeviation(GetWorld()->GetRandomNumberGenerator(), m_AxisDeviation, m_vRotationAxis);

      if (m_AxisDeviation.GetRadian() > 0 && GetWorld()->GetRandomNumberGenerator().Bool())
        m_vRotationAxis = -m_vRotationAxis;
    }
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezRotorComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezRotorComponentPatch_1_2()
    : ezGraphPatch("ezRotorComponent", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    // Base class
    context.PatchBaseClass("ezTransformComponent", 2, true);

    // this class
    pNode->RenameProperty("Degrees to Rotate", "DegreesToRotate");
  }
};

ezRotorComponentPatch_1_2 g_ezRotorComponentPatch_1_2;


EZ_STATICLINK_FILE(GameEngine, GameEngine_Components_Implementation_RotorComponent);
