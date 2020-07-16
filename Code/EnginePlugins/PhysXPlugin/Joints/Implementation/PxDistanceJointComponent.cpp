#include <PhysXPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Joints/PxDistanceJointComponent.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>

using namespace physx;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxDistanceJointComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MinDistance", m_fMinDistance),
    EZ_MEMBER_PROPERTY("MaxDistance", m_fMaxDistance)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("SpringStiffness", m_fSpringStiffness),
    EZ_MEMBER_PROPERTY("SpringDamping", m_fSpringDamping)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("SpringTolerance", m_fSpringTolerance)->AddAttributes(new ezDefaultValueAttribute(0.05f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezSphereVisualizerAttribute("MinDistance", nullptr, ezColor::IndianRed),
    new ezSphereVisualizerAttribute("MaxDistance", nullptr, ezColor::LightSkyBlue),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPxDistanceJointComponent::ezPxDistanceJointComponent() = default;
ezPxDistanceJointComponent::~ezPxDistanceJointComponent() = default;

void ezPxDistanceJointComponent::SetMinDistance(float value)
{
  m_fMinDistance = value;
  ApplyLimits();
}

void ezPxDistanceJointComponent::SetMaxDistance(float value)
{
  m_fMaxDistance = value;
  ApplyLimits();
}

void ezPxDistanceJointComponent::SetSpringStiffness(float value)
{
  m_fSpringStiffness = value;
  ApplyLimits();
}

void ezPxDistanceJointComponent::SetSpringDamping(float value)
{
  m_fSpringDamping = value;
  ApplyLimits();
}

void ezPxDistanceJointComponent::SetSpringTolerance(float value)
{
  m_fSpringTolerance = value;
  ApplyLimits();
}

void ezPxDistanceJointComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_fMinDistance;
  s << m_fMaxDistance;
  s << m_fSpringStiffness;
  s << m_fSpringDamping;
  s << m_fSpringTolerance;
}

void ezPxDistanceJointComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_fMinDistance;
  s >> m_fMaxDistance;
  s >> m_fSpringStiffness;
  s >> m_fSpringDamping;
  s >> m_fSpringTolerance;
}

void ezPxDistanceJointComponent::CreateJointType(PxRigidActor* actor0, const PxTransform& localFrame0, PxRigidActor* actor1, const PxTransform& localFrame1)
{
  m_pJoint = PxDistanceJointCreate(*(ezPhysX::GetSingleton()->GetPhysXAPI()), actor0, localFrame0, actor1, localFrame1);

  ApplyLimits();
}

void ezPxDistanceJointComponent::ApplyLimits()
{
  PxDistanceJoint* pJoint = static_cast<PxDistanceJoint*>(m_pJoint);
  if (pJoint == nullptr)
    return;

  pJoint->setMinDistance(m_fMinDistance);
  pJoint->setMaxDistance(m_fMaxDistance);
  pJoint->setStiffness(m_fSpringStiffness);
  pJoint->setDamping(ezMath::Max(0.0f, m_fSpringDamping));
  pJoint->setTolerance(ezMath::Max(0.0f, m_fSpringTolerance));

  pJoint->setDistanceJointFlag(PxDistanceJointFlag::eMIN_DISTANCE_ENABLED, m_fMinDistance > 0.0f);
  pJoint->setDistanceJointFlag(PxDistanceJointFlag::eMAX_DISTANCE_ENABLED, m_fMaxDistance > m_fMinDistance);
  pJoint->setDistanceJointFlag(PxDistanceJointFlag::eSPRING_ENABLED, m_fSpringStiffness > 0.0f);
}


EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Joints_Implementation_PxDistanceJointComponent);
