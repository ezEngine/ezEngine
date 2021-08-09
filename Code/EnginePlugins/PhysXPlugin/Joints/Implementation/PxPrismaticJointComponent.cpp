#include <PhysXPlugin/PhysXPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Joints/PxPrismaticJointComponent.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>

using namespace physx;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxPrismaticJointComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_ACCESSOR_PROPERTY("LimitMode", ezPxJointLimitMode, GetLimitMode, SetLimitMode),
    EZ_ACCESSOR_PROPERTY("LowerLimit", GetLowerLimitDistance, SetLowerLimitDistance),
    EZ_ACCESSOR_PROPERTY("UpperLimit", GetUpperLimitDistance, SetUpperLimitDistance),
    EZ_ACCESSOR_PROPERTY("SpringStiffness", GetSpringStiffness, SetSpringStiffness)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("SpringDamping", GetSpringDamping, SetSpringDamping)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 1.0f, ezColor::Orange, nullptr, "UpperLimit"),
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 1.0f, ezColor::Teal, nullptr, "LowerLimit"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPxPrismaticJointComponent::ezPxPrismaticJointComponent() = default;
ezPxPrismaticJointComponent::~ezPxPrismaticJointComponent() = default;

void ezPxPrismaticJointComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_fLowerLimitDistance;
  s << m_fUpperLimitDistance;
  s << m_fSpringStiffness;
  s << m_fSpringDamping;

  // version 2
  s << m_LimitMode;
}

void ezPxPrismaticJointComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_fLowerLimitDistance;
  s >> m_fUpperLimitDistance;
  s >> m_fSpringStiffness;
  s >> m_fSpringDamping;

  if (uiVersion >= 2)
  {
    s >> m_LimitMode;
  }
}

void ezPxPrismaticJointComponent::SetLimitMode(ezPxJointLimitMode::Enum mode)
{
  m_LimitMode = mode;
  QueueApplySettings();
}

void ezPxPrismaticJointComponent::SetLowerLimitDistance(float f)
{
  m_fLowerLimitDistance = f;
  QueueApplySettings();
}

void ezPxPrismaticJointComponent::SetUpperLimitDistance(float f)
{
  m_fUpperLimitDistance = f;
  QueueApplySettings();
}

void ezPxPrismaticJointComponent::SetSpringStiffness(float f)
{
  m_fSpringStiffness = f;
  QueueApplySettings();
}

void ezPxPrismaticJointComponent::SetSpringDamping(float f)
{
  m_fSpringDamping = f;
  QueueApplySettings();
}

void ezPxPrismaticJointComponent::ApplySettings()
{
  ezPxJointComponent::ApplySettings();

  auto pJoint = static_cast<PxPrismaticJoint*>(m_pJoint);

  pJoint->setPrismaticJointFlag(PxPrismaticJointFlag::eLIMIT_ENABLED, m_LimitMode != ezPxJointLimitMode::NoLimit);

  if (m_LimitMode != ezPxJointLimitMode::NoLimit)
  {
    float low = m_fLowerLimitDistance;
    float high = m_fUpperLimitDistance;

    if (low > high)
      ezMath::Swap(low, high);

    PxJointLinearLimitPair limit(low, high, PxSpring(0, 0));

    if (m_LimitMode == ezPxJointLimitMode::SoftLimit)
    {
      limit.stiffness = ezMath::Max(0.1f, m_fSpringStiffness);
      limit.damping = ezMath::Max(0.1f, m_fSpringDamping);
    }
    else
    {
      limit.restitution = ezMath::Clamp(m_fSpringStiffness, 0.0f, 1.0f);
      limit.bounceThreshold = m_fSpringDamping;
    }

    pJoint->setLimit(limit);
  }
}

void ezPxPrismaticJointComponent::CreateJointType(
  PxRigidActor* actor0, const PxTransform& localFrame0, PxRigidActor* actor1, const PxTransform& localFrame1)
{
  m_pJoint = PxPrismaticJointCreate(*(ezPhysX::GetSingleton()->GetPhysXAPI()), actor0, localFrame0, actor1, localFrame1);
}
