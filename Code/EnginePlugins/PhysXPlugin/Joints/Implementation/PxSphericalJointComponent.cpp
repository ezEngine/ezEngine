#include <PhysXPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Joints/PxSphericalJointComponent.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>

using namespace physx;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxSphericalJointComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("LimitRotation", GetLimitRotation, SetLimitRotation),
    EZ_ACCESSOR_PROPERTY("ConeLimitY", GetConeLimitY, SetConeLimitY)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(0), ezAngle::Degree(+179))),
    EZ_ACCESSOR_PROPERTY("ConeLimitZ", GetConeLimitZ, SetConeLimitZ)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(0), ezAngle::Degree(+179))),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPxSphericalJointComponent::ezPxSphericalJointComponent() = default;
ezPxSphericalJointComponent::~ezPxSphericalJointComponent() = default;

void ezPxSphericalJointComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_bLimitRotation;
  s << m_ConeLimitY;
  s << m_ConeLimitZ;
}

void ezPxSphericalJointComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_bLimitRotation;
  s >> m_ConeLimitY;
  s >> m_ConeLimitZ;
}


void ezPxSphericalJointComponent::SetLimitRotation(bool b)
{
  m_bLimitRotation = b;
  ApplyConeLimit(m_pJoint);
}

void ezPxSphericalJointComponent::SetConeLimitY(ezAngle v)
{
  m_ConeLimitY = v;
  ApplyConeLimit(m_pJoint);
}

void ezPxSphericalJointComponent::SetConeLimitZ(ezAngle v)
{
  m_ConeLimitZ = v;
  ApplyConeLimit(m_pJoint);
}

void ezPxSphericalJointComponent::ApplyConeLimit(PxJoint* pJoint0)
{
  if (pJoint0 == nullptr)
    return;

  PxSphericalJoint* pJoint = static_cast<PxSphericalJoint*>(pJoint0);

  pJoint->setSphericalJointFlag(PxSphericalJointFlag::eLIMIT_ENABLED, m_bLimitRotation);

  if (m_bLimitRotation)
  {
    pJoint->setLimitCone(PxJointLimitCone(m_ConeLimitY.GetRadian(), m_ConeLimitZ.GetRadian(), 0.01f));
  }
}

PxJoint* ezPxSphericalJointComponent::CreateJointType(PxRigidActor* actor0, const PxTransform& localFrame0, PxRigidActor* actor1,
  const PxTransform& localFrame1)
{
  PxSphericalJoint* pJoint = PxSphericalJointCreate(*(ezPhysX::GetSingleton()->GetPhysXAPI()), actor0, localFrame0, actor1, localFrame1);

  if (pJoint == nullptr)
    return nullptr;

  ApplyConeLimit(pJoint);

  return pJoint;
}
