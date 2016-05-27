#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Joints/PxDistanceJointComponent.h>
#include <PhysXPlugin/PhysXWorldModule.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezPxDistanceJointComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Min Distance", m_fMinDistance),
    EZ_MEMBER_PROPERTY("Max Distance", m_fMaxDistance),
    EZ_MEMBER_PROPERTY("Spring Stiffness", m_fSpringStiffness),
    EZ_MEMBER_PROPERTY("Spring Damping", m_fSpringDamping),
    EZ_MEMBER_PROPERTY("Spring Tolerance", m_fSpringTolerance),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

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

void ezPxDistanceJointComponent::OnSimulationStarted()
{
  PxDistanceJoint* pJoint = static_cast<PxDistanceJoint*>(SetupJoint());

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

PxJoint* ezPxDistanceJointComponent::CreateJointType(PxPhysics& api, PxRigidActor* actor0, const PxTransform& localFrame0, PxRigidActor* actor1, const PxTransform& localFrame1)
{
  return PxDistanceJointCreate(api, actor0, localFrame0, actor1, localFrame1);
}

