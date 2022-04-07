#include <PhysXPlugin/PhysXPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Components/PxDynamicActorComponent.h>
#include <PhysXPlugin/Joints/PxJointComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>

using namespace physx;

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezPxJointComponent, 3)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("BreakForce", GetBreakForce, SetBreakForce),
    EZ_ACCESSOR_PROPERTY("BreakTorque", GetBreakTorque, SetBreakTorque),
    EZ_ACCESSOR_PROPERTY("PairCollision", GetPairCollision, SetPairCollision),
    EZ_ACCESSOR_PROPERTY("ParentActor", DummyGetter, SetParentActorReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
    EZ_ACCESSOR_PROPERTY("ChildActor", DummyGetter, SetChildActorReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
    EZ_ACCESSOR_PROPERTY("ChildActorAnchor", DummyGetter, SetChildActorAnchorReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/PhysX/Constraints"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezPxJointLimitMode, 1)
  EZ_ENUM_CONSTANTS(ezPxJointLimitMode::NoLimit, ezPxJointLimitMode::HardLimit, ezPxJointLimitMode::SoftLimit)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezPxJointDriveMode, 1)
  EZ_ENUM_CONSTANTS(ezPxJointDriveMode::NoDrive, ezPxJointDriveMode::DriveAndSpin, ezPxJointDriveMode::DriveAndBrake)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

ezPxJointComponent::ezPxJointComponent() = default;
ezPxJointComponent::~ezPxJointComponent() = default;

void ezPxJointComponent::SetBreakForce(float value)
{
  m_fBreakForce = value;
  QueueApplySettings();
}

void ezPxJointComponent::SetBreakTorque(float value)
{
  m_fBreakTorque = value;
  QueueApplySettings();
}

void ezPxJointComponent::SetPairCollision(bool value)
{
  m_bPairCollision = value;
  QueueApplySettings();
}

void ezPxJointComponent::OnSimulationStarted()
{
  PxRigidActor* pActorA = nullptr;
  PxRigidActor* pActorB = nullptr;

  if (FindParentBody(pActorA).Failed())
    return;

  if (FindChildBody(pActorB).Failed())
    return;

  m_localFrameA.m_qRotation.Normalize();
  m_localFrameB.m_qRotation.Normalize();

  const PxTransform tLocalToActorA = ezPxConversionUtils::ToTransform(m_localFrameA);
  const PxTransform tLocalToActorB = ezPxConversionUtils::ToTransform(m_localFrameB);

  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
  EZ_PX_WRITE_LOCK(*(pModule->GetPxScene()));

  CreateJointType(pActorA, tLocalToActorA, pActorB, tLocalToActorB);
  EZ_ASSERT_DEV(m_pJoint != nullptr, "Joint creation failed");

  ApplySettings();
}

void ezPxJointComponent::OnDeactivated()
{
  if (m_pJoint != nullptr)
  {
    ezPhysXWorldModule* pModule = GetWorld()->GetModule<ezPhysXWorldModule>();

    pModule->m_BreakableJoints.Remove(m_pJoint->getConstraint());

    if (pModule->GetPxScene())
    {
      EZ_PX_WRITE_LOCK(*(pModule->GetPxScene()));

      m_pJoint->release();
    }

    m_pJoint = nullptr;
  }

  SUPER::OnDeactivated();
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

  stream.WriteGameObjectHandle(m_hActorBAnchor);
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

  if (uiVersion >= 3)
  {
    m_hActorBAnchor = stream.ReadGameObjectHandle();
  }
}

void ezPxJointComponent::SetParentActorReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetParentActor(resolver(szReference, GetHandle(), "ParentActor"));
}

void ezPxJointComponent::SetChildActorReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetChildActor(resolver(szReference, GetHandle(), "ChildActor"));
}

void ezPxJointComponent::SetChildActorAnchorReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetUserFlag(1, false); // local frame B is not valid
  m_hActorBAnchor = resolver(szReference, GetHandle(), "ChildActorAnchor");
}

void ezPxJointComponent::SetParentActor(ezGameObjectHandle hActor)
{
  SetUserFlag(0, false); // local frame A is not valid
  m_hActorA = hActor;
}

void ezPxJointComponent::SetChildActor(ezGameObjectHandle hActor)
{
  SetUserFlag(1, false); // local frame B is not valid
  m_hActorB = hActor;
}

void ezPxJointComponent::SetChildActorAnchor(ezGameObjectHandle hActor)
{
  SetUserFlag(1, false); // local frame B is not valid
  m_hActorBAnchor = hActor;
}

void ezPxJointComponent::SetActors(ezGameObjectHandle hActorA, const ezTransform& localFrameA, ezGameObjectHandle hActorB, const ezTransform& localFrameB)
{
  m_hActorA = hActorA;
  m_hActorB = hActorB;

  // prevent FindParentBody() and FindChildBody() from overwriting the local frames
  // local frame A and B are already valid
  SetUserFlag(0, true);
  SetUserFlag(1, true);

  m_localFrameA = localFrameA;
  m_localFrameB = localFrameB;
}

void ezPxJointComponent::ApplySettings()
{
  SetUserFlag(2, false);

  const float fBreakForce = m_fBreakForce <= 0.0f ? ezMath::MaxValue<float>() : m_fBreakForce;
  const float fBreakTorque = m_fBreakTorque <= 0.0f ? ezMath::MaxValue<float>() : m_fBreakTorque;
  m_pJoint->setBreakForce(fBreakForce, fBreakTorque);

  if (m_fBreakForce > 0.0f || m_fBreakTorque > 0.0f)
  {
    ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
    pModule->m_BreakableJoints[m_pJoint->getConstraint()] = GetHandle();
  }

  m_pJoint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, m_bPairCollision);

  PxRigidActor* pActor0 = nullptr;
  PxRigidActor* pActor1 = nullptr;
  m_pJoint->getActors(pActor0, pActor1);

  if (pActor0 && pActor0->is<PxRigidDynamic>() && !static_cast<PxRigidDynamic*>(pActor0)->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
    static_cast<PxRigidDynamic*>(pActor0)->wakeUp();
  if (pActor1 && pActor1->is<PxRigidDynamic>() && !static_cast<PxRigidDynamic*>(pActor1)->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
    static_cast<PxRigidDynamic*>(pActor1)->wakeUp();
}

ezResult ezPxJointComponent::FindParentBody(physx::PxRigidActor*& pActor)
{
  ezGameObject* pObject = nullptr;
  ezPxDynamicActorComponent* pRbComp = nullptr;

  if (!m_hActorA.IsInvalidated())
  {
    if (!GetWorld()->TryGetObject(m_hActorA, pObject) || !pObject->IsActive())
    {
      ezLog::Error("{0} '{1}' parent reference is a non-existing object. Joint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
      return EZ_FAILURE;
    }

    if (!pObject->TryGetComponentOfBaseType(pRbComp))
    {
      ezLog::Error("{0} '{1}' parent reference is an object without a ezPxDynamicActorComponent. Joint is ignored.", GetDynamicRTTI()->GetTypeName(),
        GetOwner()->GetName());
      return EZ_FAILURE;
    }
  }
  else
  {
    pObject = GetOwner();

    while (pObject != nullptr)
    {
      if (pObject->TryGetComponentOfBaseType(pRbComp))
        break;

      pObject = pObject->GetParent();
    }

    if (pRbComp == nullptr)
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
  pActor = pRbComp->GetPxActor();

  if (pActor == nullptr)
  {
    ezLog::Error("{0} '{1}' parent reference is an object with an invalid ezPxDynamicActorComponent. Joint is ignored.",
      GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
    return EZ_FAILURE;
  }

  m_hActorA = pObject->GetHandle();

  if (GetUserFlag(0) == false)
  {
    // m_localFrameA is now valid
    SetUserFlag(0, true);
    m_localFrameA.SetLocalTransform(pObject->GetGlobalTransform(), GetOwner()->GetGlobalTransform());
    m_localFrameA.m_vPosition = m_localFrameA.m_vPosition.CompMul(pObject->GetGlobalScaling());
  }

  return EZ_SUCCESS;
}

ezResult ezPxJointComponent::FindChildBody(physx::PxRigidActor*& pActor)
{
  ezGameObject* pObject = nullptr;
  ezPxDynamicActorComponent* pRbComp = nullptr;

  if (m_hActorB.IsInvalidated())
  {
    ezLog::Error("{0} '{1}' has no child reference. Joint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
    return EZ_FAILURE;
  }

  if (!GetWorld()->TryGetObject(m_hActorB, pObject) || !pObject->IsActive())
  {
    ezLog::Error("{0} '{1}' child reference is a non-existing object. Joint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
    return EZ_FAILURE;
  }

  if (!pObject->TryGetComponentOfBaseType(pRbComp))
  {
    // this makes it possible to link the joint to a prefab, because it may skip the top level hierarchy of the prefab
    pObject = pObject->SearchForChildByNameSequence("/", ezGetStaticRTTI<ezPxDynamicActorComponent>());

    if (pObject == nullptr)
    {
      ezLog::Error("{0} '{1}' child reference is an object without a ezPxDynamicActorComponent. Joint is ignored.", GetDynamicRTTI()->GetTypeName(),
        GetOwner()->GetName());
      return EZ_FAILURE;
    }

    pObject->TryGetComponentOfBaseType(pRbComp);
  }

  // if (pRbComp->GetKinematic())
  //{
  //  ezLog::Error("{0} '{1}' has a child with a ezPxDynamicActorComponent which is set to be kinematic. Joint is ignored.",
  //  GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName()); return EZ_FAILURE;
  //}

  pRbComp->EnsureSimulationStarted();
  pActor = pRbComp->GetPxActor();

  if (pActor == nullptr)
  {
    ezLog::Error("{0} '{1}' child reference is an object with an invalid ezPxDynamicActorComponent. Joint is ignored.",
      GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
    return EZ_FAILURE;
  }

  m_hActorB = pObject->GetHandle();

  if (GetUserFlag(1) == false)
  {
    ezGameObject* pAnchorObject = GetOwner();

    if (!m_hActorBAnchor.IsInvalidated())
    {
      if (!GetWorld()->TryGetObject(m_hActorBAnchor, pAnchorObject))
      {
        ezLog::Error(
          "{0} '{1}' anchor reference is a non-existing object. Joint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
        return EZ_FAILURE;
      }
    }

    // m_localFrameB is now valid
    SetUserFlag(1, true);
    m_localFrameB.SetLocalTransform(pObject->GetGlobalTransform(), pAnchorObject->GetGlobalTransform());
    m_localFrameB.m_vPosition = m_localFrameB.m_vPosition.CompMul(pObject->GetGlobalScaling());
  }

  return EZ_SUCCESS;
}

void ezPxJointComponent::QueueApplySettings()
{
  if (m_pJoint == nullptr)
    return;

  // already in queue ?
  if (GetUserFlag(2))
    return;

  SetUserFlag(2, true);

  if (ezPhysXWorldModule* pModule = GetWorld()->GetModule<ezPhysXWorldModule>())
  {
    pModule->m_RequireUpdate.PushBack(GetHandle());
  }
}

EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Joints_Implementation_PxJointComponent);
