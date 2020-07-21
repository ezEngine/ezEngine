#include <PhysXPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Joints/PxSphericalJointComponent.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>

using namespace physx;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxSphericalJointComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_ACCESSOR_PROPERTY("LimitMode", ezPxJointLimitMode, GetLimitMode, SetLimitMode),
    EZ_ACCESSOR_PROPERTY("ConeLimitY", GetConeLimitY, SetConeLimitY)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(0), ezAngle::Degree(+179))),
    EZ_ACCESSOR_PROPERTY("ConeLimitZ", GetConeLimitZ, SetConeLimitZ)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(0), ezAngle::Degree(+179))),
    EZ_ACCESSOR_PROPERTY("SpringStiffness", GetSpringStiffness, SetSpringStiffness)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("SpringDamping", GetSpringDamping, SetSpringDamping)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.2, ezColor::SlateGray)
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPxSphericalJointComponent::ezPxSphericalJointComponent() = default;
ezPxSphericalJointComponent::~ezPxSphericalJointComponent() = default;

void ezPxSphericalJointComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  // version 2
  s << m_LimitMode;

  s << m_ConeLimitY;
  s << m_ConeLimitZ;

  // version 2
  s << m_fSpringStiffness;
  s << m_fSpringDamping;
}

void ezPxSphericalJointComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  if (uiVersion < 2)
  {
    bool m_bLimitRotation;
    s >> m_bLimitRotation;
    m_LimitMode = m_bLimitRotation ? ezPxJointLimitMode::HardLimit : ezPxJointLimitMode::NoLimit;
  }
  else
  {
    s >> m_LimitMode;
  }

  s >> m_ConeLimitY;
  s >> m_ConeLimitZ;

  if (uiVersion >= 2)
  {
    s >> m_fSpringStiffness;
    s >> m_fSpringDamping;
  }
}

void ezPxSphericalJointComponent::SetLimitMode(ezPxJointLimitMode::Enum mode)
{
  m_LimitMode = mode;
  QueueApplySettings();
}

void ezPxSphericalJointComponent::SetConeLimitY(ezAngle v)
{
  m_ConeLimitY = v;
  QueueApplySettings();
}

void ezPxSphericalJointComponent::SetConeLimitZ(ezAngle v)
{
  m_ConeLimitZ = v;
  QueueApplySettings();
}

void ezPxSphericalJointComponent::SetSpringStiffness(float f)
{
  m_fSpringStiffness = f;
  QueueApplySettings();
}

void ezPxSphericalJointComponent::SetSpringDamping(float f)
{
  m_fSpringDamping = f;
  QueueApplySettings();
}

void ezPxSphericalJointComponent::ApplySettings()
{
  ezPxJointComponent::ApplySettings();

  PxSphericalJoint* pJoint = static_cast<PxSphericalJoint*>(m_pJoint);

  pJoint->setSphericalJointFlag(PxSphericalJointFlag::eLIMIT_ENABLED, m_LimitMode != ezPxJointLimitMode::NoLimit);

  if (m_LimitMode != ezPxJointLimitMode::NoLimit)
  {
    PxJointLimitCone limit(m_ConeLimitY.GetRadian(), m_ConeLimitZ.GetRadian(), 0.01f);

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

    pJoint->setLimitCone(limit);
  }
}

void ezPxSphericalJointComponent::CreateJointType(PxRigidActor* actor0, const PxTransform& localFrame0, PxRigidActor* actor1,
  const PxTransform& localFrame1)
{
  m_pJoint = PxSphericalJointCreate(*(ezPhysX::GetSingleton()->GetPhysXAPI()), actor0, localFrame0, actor1, localFrame1);
}
