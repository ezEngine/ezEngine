#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Joints/PxJointComponent.h>
#include <PhysXPlugin/Components/PxDynamicActorComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

using namespace physx;

EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezPxJointComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("BreakForce", m_fBreakForce),
    EZ_MEMBER_PROPERTY("BreakTorque", m_fBreakTorque),
    EZ_MEMBER_PROPERTY("PairCollision", m_bPairCollision),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Joints"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_ABSTRACT_COMPONENT_TYPE

ezPxJointComponent::ezPxJointComponent()
{
  m_fBreakForce = 0.0f;
  m_fBreakTorque = 0.0f;
  m_bPairCollision = false;
  m_pJoint = nullptr;
}


void ezPxJointComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_fBreakForce;
  s << m_fBreakTorque;
  s << m_bPairCollision;
}


void ezPxJointComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());


  auto& s = stream.GetStream();

  s >> m_fBreakForce;
  s >> m_fBreakTorque;
  s >> m_bPairCollision;
}


void ezPxJointComponent::Deinitialize()
{
  if (m_pJoint)
  {
    m_pJoint->release();
    m_pJoint = nullptr;
  }
}

PxJoint* ezPxJointComponent::SetupJoint()
{
  auto pOwner = GetOwner();
  auto pParent = pOwner->GetParent();
  ezGameObject* pChild = nullptr;

  ezPxDynamicActorComponent* pParentRbComp = nullptr;
  ezPxDynamicActorComponent* pChildRbComp = nullptr;

  if (pParent)
    pParent->TryGetComponentOfBaseType<ezPxDynamicActorComponent>(pParentRbComp);

  if (pOwner->TryGetComponentOfBaseType<ezPxDynamicActorComponent>(pChildRbComp))
  {
    pChild = pOwner;
  }
  else
  {
    for (auto itChild = pOwner->GetChildren(); itChild.IsValid(); itChild.Next())
    {
      if (itChild->TryGetComponentOfBaseType<ezPxDynamicActorComponent>(pChildRbComp))
      {
        pChild = itChild;
        break;
      }
    }
  }

  if (pChildRbComp == nullptr)
  {
    ezLog::Error("{0} '{1}' does not have a direct child with a ezPxDynamicActorComponent component. Joint is ignored.", GetDynamicRTTI()->GetTypeName(), pOwner->GetName());
    return nullptr;
  }

  if (pChildRbComp->GetKinematic())
  {
    ezLog::Error("{0} '{1}' has a child with a ezPxDynamicActorComponent which is set to be kinematic. Joint is ignored.", GetDynamicRTTI()->GetTypeName(), pOwner->GetName());
    return nullptr;
  }

  if (pParentRbComp != nullptr)
    pParentRbComp->EnsureSimulationStarted();

  pChildRbComp->EnsureSimulationStarted();

  PxRigidActor* pParentActor = pParentRbComp ? pParentRbComp->GetActor() : nullptr;
  PxRigidActor* pChildActor = pChildRbComp->GetActor();

  if (pChildActor == nullptr)
  {
    ezLog::Error("{0} '{1}' has an invalid child. Joint is ignored.", GetDynamicRTTI()->GetTypeName(), pOwner->GetName());
    return nullptr;
  }

  ezTransform tLocalToParent;
  ezTransform tLocalToChild;
  const ezTransform tOwn = pOwner->GetGlobalTransform();

  if (pParentRbComp)
  {
    ezTransform tParent = pParent->GetGlobalTransform();
    tParent.Invert();

    tLocalToParent = tParent * tOwn;
  }
  else
  {
    tLocalToParent = tOwn;
  }

  {
    ezTransform tChild = pChild->GetGlobalTransform();
    tChild.Invert();
    tLocalToChild = tChild * tOwn;
  }

  PxTransform tPxLocalToParent = ezPxConversionUtils::ToTransform(tLocalToParent);
  PxTransform tPxLocalToChild = ezPxConversionUtils::ToTransform(tLocalToChild);

  m_pJoint = CreateJointType(pParentActor, tPxLocalToParent, pChildActor, tPxLocalToChild);
  EZ_ASSERT_DEV(m_pJoint != nullptr, "Joint creation failed");

  const float fBreakForce = m_fBreakForce <= 0.0f ? ezMath::BasicType<float>::MaxValue() : m_fBreakForce;
  const float fBreakTorque = m_fBreakTorque <= 0.0f ? ezMath::BasicType<float>::MaxValue() : m_fBreakTorque;

  m_pJoint->setBreakForce(fBreakForce, fBreakTorque);
  m_pJoint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, m_bPairCollision);

  return m_pJoint;
}



EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Joints_Implementation_PxJointComponent);

