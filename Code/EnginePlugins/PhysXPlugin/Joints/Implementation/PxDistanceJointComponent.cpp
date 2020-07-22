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
    EZ_ACCESSOR_PROPERTY("MinDistance", GetMinDistance, SetMinDistance),
    EZ_ACCESSOR_PROPERTY("MaxDistance", GetMaxDistance, SetMaxDistance)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_ACCESSOR_PROPERTY("SpringStiffness", GetSpringStiffness, SetSpringStiffness),
    EZ_ACCESSOR_PROPERTY("SpringDamping", GetSpringDamping, SetSpringDamping)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_ACCESSOR_PROPERTY("SpringTolerance", GetSpringTolerance, SetSpringTolerance)->AddAttributes(new ezDefaultValueAttribute(0.05f)),
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
  QueueApplySettings();
}

void ezPxDistanceJointComponent::SetMaxDistance(float value)
{
  m_fMaxDistance = value;
  QueueApplySettings();
}

void ezPxDistanceJointComponent::SetSpringStiffness(float value)
{
  m_fSpringStiffness = value;
  QueueApplySettings();
}

void ezPxDistanceJointComponent::SetSpringDamping(float value)
{
  m_fSpringDamping = value;
  QueueApplySettings();
}

void ezPxDistanceJointComponent::SetSpringTolerance(float value)
{
  m_fSpringTolerance = value;
  QueueApplySettings();
}

void ezPxDistanceJointComponent::ApplySettings()
{
  ezPxJointComponent::ApplySettings();

  PxDistanceJoint* pJoint = static_cast<PxDistanceJoint*>(m_pJoint);

  pJoint->setMinDistance(m_fMinDistance);
  pJoint->setMaxDistance(m_fMaxDistance);
  pJoint->setStiffness(m_fSpringStiffness);
  pJoint->setDamping(ezMath::Max(0.0f, m_fSpringDamping));
  pJoint->setTolerance(ezMath::Max(0.0f, m_fSpringTolerance));

  pJoint->setDistanceJointFlag(PxDistanceJointFlag::eMIN_DISTANCE_ENABLED, m_fMinDistance > 0.0f);
  pJoint->setDistanceJointFlag(PxDistanceJointFlag::eMAX_DISTANCE_ENABLED, m_fMaxDistance > m_fMinDistance);
  pJoint->setDistanceJointFlag(PxDistanceJointFlag::eSPRING_ENABLED, m_fSpringStiffness > 0.0f);
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

void ezPxDistanceJointComponent::CreateJointType(
  PxRigidActor* actor0, const PxTransform& localFrame0, PxRigidActor* actor1, const PxTransform& localFrame1)
{
  m_pJoint = PxDistanceJointCreate(*(ezPhysX::GetSingleton()->GetPhysXAPI()), actor0, localFrame0, actor1, localFrame1);
}
