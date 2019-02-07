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
    EZ_MEMBER_PROPERTY("MaxDistance", m_fMaxDistance),
    EZ_MEMBER_PROPERTY("SpringStiffness", m_fSpringStiffness),
    EZ_MEMBER_PROPERTY("SpringDamping", m_fSpringDamping),
    EZ_MEMBER_PROPERTY("SpringTolerance", m_fSpringTolerance),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPxDistanceJointComponent::ezPxDistanceJointComponent()
{
  m_fMinDistance = 0.0f;
  m_fMaxDistance = 1.0f;
  m_fSpringStiffness = 0.0f;
  m_fSpringDamping = 0.0f;
  m_fSpringTolerance = 0.0f;
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

PxJoint* ezPxDistanceJointComponent::CreateJointType(PxRigidActor* actor0, const PxTransform& localFrame0, PxRigidActor* actor1,
                                                     const PxTransform& localFrame1)
{
  PxDistanceJoint* pJoint = PxDistanceJointCreate(*(ezPhysX::GetSingleton()->GetPhysXAPI()), actor0, localFrame0, actor1, localFrame1);

  if (pJoint != nullptr)
  {
    pJoint->setMinDistance(m_fMinDistance);
    pJoint->setMaxDistance(m_fMaxDistance);
    pJoint->setStiffness(m_fSpringStiffness);
    pJoint->setDamping(ezMath::Max(0.0f, m_fSpringDamping));
    pJoint->setTolerance(ezMath::Max(0.0f, m_fSpringTolerance));

    pJoint->setDistanceJointFlag(PxDistanceJointFlag::eMIN_DISTANCE_ENABLED, m_fMinDistance > 0.0f);
    pJoint->setDistanceJointFlag(PxDistanceJointFlag::eMAX_DISTANCE_ENABLED, m_fMaxDistance > m_fMinDistance);
    pJoint->setDistanceJointFlag(PxDistanceJointFlag::eSPRING_ENABLED, m_fSpringStiffness > 0.0f);
  }

  return pJoint;
}



EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Joints_Implementation_PxDistanceJointComponent);
