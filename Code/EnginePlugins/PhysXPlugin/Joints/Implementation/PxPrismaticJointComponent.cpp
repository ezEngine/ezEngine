#include <PhysXPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Joints/PxPrismaticJointComponent.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>

using namespace physx;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxPrismaticJointComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("LowerLimit", GetLowerLimitDistance, SetLowerLimitDistance),
    EZ_ACCESSOR_PROPERTY("UpperLimit", GetUpperLimitDistance, SetUpperLimitDistance),
    EZ_ACCESSOR_PROPERTY("Stiffness", GetSpringStiffness, SetSpringStiffness)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("Damping", GetSpringDamping, SetSpringDamping)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
  }
  EZ_END_PROPERTIES;
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
}

void ezPxPrismaticJointComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_fLowerLimitDistance;
  s >> m_fUpperLimitDistance;
  s >> m_fSpringStiffness;
  s >> m_fSpringDamping;
}

void ezPxPrismaticJointComponent::SetLowerLimitDistance(float f)
{
  m_fLowerLimitDistance = f;
  ApplyLimits(m_pJoint);
}

void ezPxPrismaticJointComponent::SetUpperLimitDistance(float f)
{
  m_fUpperLimitDistance = f;
  ApplyLimits(m_pJoint);
}

void ezPxPrismaticJointComponent::SetSpringStiffness(float f)
{
  m_fSpringStiffness = f;
  ApplyLimits(m_pJoint);
}

void ezPxPrismaticJointComponent::SetSpringDamping(float f)
{
  m_fSpringDamping = f;
  ApplyLimits(m_pJoint);
}

void ezPxPrismaticJointComponent::ApplyLimits(physx::PxJoint* pJoint0)
{
  if (pJoint0 == nullptr)
    return;

  auto pJoint = static_cast<PxPrismaticJoint*>(pJoint0);

  if (m_fLowerLimitDistance < m_fUpperLimitDistance)
  {
    pJoint->setPrismaticJointFlag(PxPrismaticJointFlag::eLIMIT_ENABLED, true);

    if (m_fSpringStiffness <= 0)
    {
      PxJointLinearLimitPair limit(m_fLowerLimitDistance, m_fUpperLimitDistance, PxSpring(0, 0));
      pJoint->setLimit(limit);
    }
    else
    {
      PxJointLinearLimitPair limit(m_fLowerLimitDistance, m_fUpperLimitDistance, PxSpring(ezMath::Max(0.0f, m_fSpringStiffness), ezMath::Max(0.0f, m_fSpringDamping)));
      pJoint->setLimit(limit);
    }
  }
  else
  {
    pJoint->setPrismaticJointFlag(PxPrismaticJointFlag::eLIMIT_ENABLED, false);
  }
}

PxJoint* ezPxPrismaticJointComponent::CreateJointType(PxRigidActor* actor0, const PxTransform& localFrame0, PxRigidActor* actor1,
  const PxTransform& localFrame1)
{
  PxPrismaticJoint* pJoint = PxPrismaticJointCreate(*(ezPhysX::GetSingleton()->GetPhysXAPI()), actor0, localFrame0, actor1, localFrame1);

  if (pJoint == nullptr)
    return nullptr;

  ApplyLimits(pJoint);

  return pJoint;
}
