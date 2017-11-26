#include <PCH.h>
#include <PhysXPlugin/Joints/PxJointComponent.h>
#include <PhysXPlugin/Components/PxDynamicActorComponent.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
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

ezPxJointComponent::~ezPxJointComponent()
{

}

void ezPxJointComponent::OnSimulationStarted()
{
  // Actors have not been setup from the outside, try to find them in the hierarchy
  if (m_hActorA.IsInvalidated() && m_hActorB.IsInvalidated())
  {
    FindActorsInHierarchy();
  }

  PxRigidActor* pActorA = nullptr;
  PxRigidActor* pActorB = nullptr;

  {
    ezGameObject* pActorObjectA = nullptr;
    ezGameObject* pActorObjectB = nullptr;
    GetWorld()->TryGetObject(m_hActorA, pActorObjectA);
    GetWorld()->TryGetObject(m_hActorB, pActorObjectB);

    ezPxDynamicActorComponent* pActorComponentA = nullptr;
    if (pActorObjectA != nullptr && pActorObjectA->TryGetComponentOfBaseType(pActorComponentA))
    {
      pActorComponentA->EnsureSimulationStarted();
      pActorA = pActorComponentA->GetActor();
    }

    ezPxDynamicActorComponent* pActorComponentB = nullptr;
    if (pActorObjectB != nullptr && pActorObjectB->TryGetComponentOfBaseType(pActorComponentB))
    {
      pActorComponentB->EnsureSimulationStarted();
      pActorB = pActorComponentB->GetActor();
    }
  }

  PxTransform tLocalToActorA = ezPxConversionUtils::ToTransform(m_localFrameA);
  PxTransform tLocalToActorB = ezPxConversionUtils::ToTransform(m_localFrameB);

  {
    ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
    EZ_PX_WRITE_LOCK(*(pModule->GetPxScene()));

    m_pJoint = CreateJointType(pActorA, tLocalToActorA, pActorB, tLocalToActorB);
    EZ_ASSERT_DEV(m_pJoint != nullptr, "Joint creation failed");

    const float fBreakForce = m_fBreakForce <= 0.0f ? ezMath::BasicType<float>::MaxValue() : m_fBreakForce;
    const float fBreakTorque = m_fBreakTorque <= 0.0f ? ezMath::BasicType<float>::MaxValue() : m_fBreakTorque;

    m_pJoint->setBreakForce(fBreakForce, fBreakTorque);
    m_pJoint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, m_bPairCollision);
  }
}

void ezPxJointComponent::Deinitialize()
{
  if (m_pJoint != nullptr)
  {
    ezPhysXWorldModule* pModule = GetWorld()->GetModule<ezPhysXWorldModule>();
    EZ_PX_WRITE_LOCK(*(pModule->GetPxScene()));

    m_pJoint->release();
    m_pJoint = nullptr;
  }
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

void ezPxJointComponent::SetActors(ezGameObjectHandle hActorA, const ezTransform& localFrameA, ezGameObjectHandle hActorB, const ezTransform& localFrameB)
{
  m_hActorA = hActorA;
  m_hActorB = hActorB;

  m_localFrameA = localFrameA;
  m_localFrameB = localFrameB;
}

void ezPxJointComponent::FindActorsInHierarchy()
{
  auto pOwner = GetOwner();
  auto pParent = pOwner->GetParent();
  ezGameObject* pChild = nullptr;

  ezPxDynamicActorComponent* pParentRbComp = nullptr;
  ezPxDynamicActorComponent* pChildRbComp = nullptr;

  if (pParent != nullptr)
  {
    pParent->TryGetComponentOfBaseType<ezPxDynamicActorComponent>(pParentRbComp);
  }

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
    return;
  }

  if (pChildRbComp->GetKinematic())
  {
    ezLog::Error("{0} '{1}' has a child with a ezPxDynamicActorComponent which is set to be kinematic. Joint is ignored.", GetDynamicRTTI()->GetTypeName(), pOwner->GetName());
    return;
  }

  const ezTransform tOwn = pOwner->GetGlobalTransform();

  if (pParentRbComp)
  {
    m_localFrameA.SetLocalTransform(pParent->GetGlobalTransform(), tOwn);
    m_hActorA = pParent->GetHandle();
  }
  else
  {
    m_localFrameA = tOwn;
  }

  m_localFrameB.SetLocalTransform(pChild->GetGlobalTransform(), tOwn);
  m_hActorB = pChild->GetHandle();
}

EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Joints_Implementation_PxJointComponent);

