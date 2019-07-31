#include <PhysXPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Components/PxDynamicActorComponent.h>
#include <PhysXPlugin/Joints/PxJointComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>

using namespace physx;

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezPxJointComponent, 2)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("BreakForce", m_fBreakForce),
    EZ_MEMBER_PROPERTY("BreakTorque", m_fBreakTorque),
    EZ_MEMBER_PROPERTY("PairCollision", m_bPairCollision),
    EZ_ACCESSOR_PROPERTY("ParentActor", DummyGetter, SetParentActor)->AddAttributes(new ezGameObjectReferenceAttribute()),
    EZ_ACCESSOR_PROPERTY("ChildActor", DummyGetter, SetChildActor)->AddAttributes(new ezGameObjectReferenceAttribute()),
  }
  EZ_END_PROPERTIES;
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Joints"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

ezPxJointComponent::ezPxJointComponent()
{
  m_fBreakForce = 0.0f;
  m_fBreakTorque = 0.0f;
  m_bPairCollision = false;
  m_pJoint = nullptr;
}

ezPxJointComponent::~ezPxJointComponent() = default;

void ezPxJointComponent::OnSimulationStarted()
{
  PxRigidActor* pActorA = nullptr;
  PxRigidActor* pActorB = nullptr;

  if (FindParentBody(pActorA).Failed())
    return;

  if (FindChildBody(pActorB).Failed())
    return;

  const PxTransform tLocalToActorA = ezPxConversionUtils::ToTransform(m_localFrameA);
  const PxTransform tLocalToActorB = ezPxConversionUtils::ToTransform(m_localFrameB);

  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
  EZ_PX_WRITE_LOCK(*(pModule->GetPxScene()));

  m_pJoint = CreateJointType(pActorA, tLocalToActorA, pActorB, tLocalToActorB);
  EZ_ASSERT_DEV(m_pJoint != nullptr, "Joint creation failed");

  const float fBreakForce = m_fBreakForce <= 0.0f ? ezMath::MaxValue<float>() : m_fBreakForce;
  const float fBreakTorque = m_fBreakTorque <= 0.0f ? ezMath::MaxValue<float>() : m_fBreakTorque;

  m_pJoint->setBreakForce(fBreakForce, fBreakTorque);
  m_pJoint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, m_bPairCollision);
}

void ezPxJointComponent::Deinitialize()
{
  if (m_pJoint != nullptr)
  {
    ezPhysXWorldModule* pModule = GetWorld()->GetModule<ezPhysXWorldModule>();

    if (pModule->GetPxScene())
    {
      EZ_PX_WRITE_LOCK(*(pModule->GetPxScene()));

      m_pJoint->release();
    }

    m_pJoint = nullptr;
  }

  SUPER::Deinitialize();
}

void ezPxJointComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_fBreakForce;
  s << m_fBreakTorque;
  s << m_bPairCollision;

  stream.WriteGameObjectHandle(m_hActorA);
  stream.WriteGameObjectHandle(m_hActorB);

  s << m_localFrameA;
  s << m_localFrameB;
}

void ezPxJointComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());


  auto& s = stream.GetStream();

  s >> m_fBreakForce;
  s >> m_fBreakTorque;
  s >> m_bPairCollision;

  if (uiVersion >= 2)
  {
    m_hActorA = stream.ReadGameObjectHandle();
    m_hActorB = stream.ReadGameObjectHandle();

    s >> m_localFrameA;
    s >> m_localFrameB;
  }
}

void ezPxJointComponent::SetParentActor(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetUserFlag(0, false);
  m_hActorA = resolver(szReference);
}

void ezPxJointComponent::SetChildActor(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetUserFlag(1, false);
  m_hActorB = resolver(szReference);
}

void ezPxJointComponent::SetActors(ezGameObjectHandle hActorA, const ezTransform& localFrameA, ezGameObjectHandle hActorB,
  const ezTransform& localFrameB)
{
  m_hActorA = hActorA;
  m_hActorB = hActorB;

  // prevent FindParentBody() and FindChildBody() from overwriting the local frames
  SetUserFlag(0, true);
  SetUserFlag(1, true);

  m_localFrameA = localFrameA;
  m_localFrameB = localFrameB;
}

ezResult ezPxJointComponent::FindParentBody(physx::PxRigidActor*& pActor)
{
  ezGameObject* pObject = nullptr;
  ezPxDynamicActorComponent* pRbComp = nullptr;

  if (!m_hActorA.IsInvalidated())
  {
    if (!GetWorld()->TryGetObject(m_hActorA, pObject))
    {
      ezLog::Error("{0} '{1}' references a non-existing object as its parent actor. Joint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
      return EZ_FAILURE;
    }

    if (!pObject->TryGetComponentOfBaseType(pRbComp))
    {
      ezLog::Error("{0} '{1}' references an object without a ezPxDynamicActorComponent as its parent actor. Joint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
      return EZ_FAILURE;
    }
  }
  else
  {
    pObject = GetOwner()->GetParent();

    if (pObject == nullptr || !pObject->TryGetComponentOfBaseType(pRbComp))
    {
      pActor = nullptr;

      if (GetUserFlag(0) == false)
      {
        // m_localFrameA is now valid
        SetUserFlag(0, true);
        m_localFrameA = GetOwner()->GetGlobalTransform();
      }
      return EZ_SUCCESS;
    }
  }

  pRbComp->EnsureSimulationStarted();
  pActor = pRbComp->GetActor();

  if (pActor == nullptr)
  {
    ezLog::Error("{0} '{1}' references an object with an invalid ezPxDynamicActorComponent as its parent actor. Joint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
    return EZ_FAILURE;
  }

  m_hActorA = pObject->GetHandle();

  if (GetUserFlag(0) == false)
  {
    // m_localFrameA is now valid
    SetUserFlag(0, true);
    m_localFrameA.SetLocalTransform(pObject->GetGlobalTransform(), GetOwner()->GetGlobalTransform());
  }

  return EZ_SUCCESS;
}

ezResult ezPxJointComponent::FindChildBody(physx::PxRigidActor*& pActor)
{
  ezGameObject* pObject = nullptr;
  ezPxDynamicActorComponent* pRbComp = nullptr;

  if (!m_hActorB.IsInvalidated())
  {
    if (!GetWorld()->TryGetObject(m_hActorB, pObject))
    {
      ezLog::Error("{0} '{1}' references a non-existing object as its child actor. Joint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
      return EZ_FAILURE;
    }

    if (!pObject->TryGetComponentOfBaseType(pRbComp))
    {
      ezLog::Error("{0} '{1}' references an object without a ezPxDynamicActorComponent as its child actor. Joint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
      return EZ_FAILURE;
    }
  }
  else
  {
    if (GetOwner()->TryGetComponentOfBaseType(pRbComp))
    {
      pObject = GetOwner();
    }
    else
    {
      for (auto itChild = GetOwner()->GetChildren(); itChild.IsValid(); itChild.Next())
      {
        if (itChild->TryGetComponentOfBaseType(pRbComp))
        {
          pObject = itChild;
          break;
        }
      }
    }
  }

  if (pObject == nullptr || pRbComp == nullptr)
  {
    ezLog::Error("{0} '{1}' does not have a direct child with a ezPxDynamicActorComponent component. Joint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
    return EZ_FAILURE;
  }

  if (pRbComp->GetKinematic())
  {
    ezLog::Error("{0} '{1}' has a child with a ezPxDynamicActorComponent which is set to be kinematic. Joint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
    return EZ_FAILURE;
  }

  pRbComp->EnsureSimulationStarted();
  pActor = pRbComp->GetActor();

  if (pActor == nullptr)
  {
    ezLog::Error("{0} '{1}' references an object with an invalid ezPxDynamicActorComponent as its child actor. Joint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
    return EZ_FAILURE;
  }

  m_hActorB = pObject->GetHandle();

  if (GetUserFlag(1) == false)
  {
    // m_localFrameB is now valid
    SetUserFlag(0, true);
    m_localFrameB.SetLocalTransform(pObject->GetGlobalTransform(), GetOwner()->GetGlobalTransform());
  }

  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Joints_Implementation_PxJointComponent);
