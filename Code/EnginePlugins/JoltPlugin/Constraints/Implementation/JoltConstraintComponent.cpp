#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Body/BodyLockMulti.h>
#include <Jolt/Physics/Constraints/Constraint.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Constraints/JoltConstraintComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezJoltConstraintComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    //EZ_ACCESSOR_PROPERTY("BreakForce", GetBreakForce, SetBreakForce),
    //EZ_ACCESSOR_PROPERTY("BreakTorque", GetBreakTorque, SetBreakTorque),
    EZ_ACCESSOR_PROPERTY("PairCollision", GetPairCollision, SetPairCollision)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("ParentActor", DummyGetter, SetParentActorReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
    EZ_ACCESSOR_PROPERTY("ChildActor", DummyGetter, SetChildActorReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
    EZ_ACCESSOR_PROPERTY("ChildActorAnchor", DummyGetter, SetChildActorAnchorReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Jolt/Constraints"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezJoltConstraintLimitMode, 1)
  EZ_ENUM_CONSTANTS(ezJoltConstraintLimitMode::NoLimit, ezJoltConstraintLimitMode::HardLimit/*, ezJoltConstraintLimitMode::SoftLimit*/)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezJoltConstraintDriveMode, 1)
  EZ_ENUM_CONSTANTS(ezJoltConstraintDriveMode::NoDrive, ezJoltConstraintDriveMode::DriveVelocity, ezJoltConstraintDriveMode::DrivePosition)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

ezJoltConstraintComponent::ezJoltConstraintComponent() = default;
ezJoltConstraintComponent::~ezJoltConstraintComponent() = default;

// void ezJoltConstraintComponent::SetBreakForce(float value)
//{
//   m_fBreakForce = value;
//   QueueApplySettings();
// }
//
// void ezJoltConstraintComponent::SetBreakTorque(float value)
//{
//   m_fBreakTorque = value;
//   QueueApplySettings();
// }

void ezJoltConstraintComponent::SetPairCollision(bool value)
{
  m_bPairCollision = value;
  QueueApplySettings();
}

void ezJoltConstraintComponent::OnSimulationStarted()
{
  ezUInt32 uiBodyIdA = ezInvalidIndex;
  ezUInt32 uiBodyIdB = ezInvalidIndex;

  if (FindParentBody(uiBodyIdA).Failed())
    return;

  if (FindChildBody(uiBodyIdB).Failed())
    return;

  if (uiBodyIdB == ezInvalidIndex)
    return;

  if (uiBodyIdA == uiBodyIdB)
  {
    ezLog::Error("Constraint can't be linked to the same body twice");
    return;
  }

  m_LocalFrameA.m_qRotation.Normalize();
  m_LocalFrameB.m_qRotation.Normalize();

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();

  {
    JPH::BodyID bodies[2] = {JPH::BodyID(uiBodyIdA), JPH::BodyID(uiBodyIdB)};
    JPH::BodyLockMultiWrite bodyLock(pModule->GetJoltSystem()->GetBodyLockInterface(), bodies, 2);

    if (uiBodyIdB != ezInvalidIndex && bodyLock.GetBody(1) != nullptr)
    {
      if (uiBodyIdA != ezInvalidIndex && bodyLock.GetBody(0) != nullptr)
      {
        CreateContstraintType(bodyLock.GetBody(0), bodyLock.GetBody(1));

        pModule->EnableJoinedBodiesCollisions(bodyLock.GetBody(0)->GetCollisionGroup().GetGroupID(), bodyLock.GetBody(1)->GetCollisionGroup().GetGroupID(), m_bPairCollision);
      }
      else
      {
        CreateContstraintType(&JPH::Body::sFixedToWorld, bodyLock.GetBody(1));
      }
    }
  }

  if (m_pConstraint)
  {
    m_pConstraint->AddRef();
    pModule->GetJoltSystem()->AddConstraint(m_pConstraint);
    ApplySettings();
  }
}

void ezJoltConstraintComponent::OnDeactivated()
{
  if (m_pConstraint != nullptr)
  {
    ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
    pModule->GetJoltSystem()->RemoveConstraint(m_pConstraint);

    // pModule->m_BreakableConstraints.Remove(m_pConstraint->getConstraint());

    m_pConstraint->Release();
    m_pConstraint = nullptr;
  }

  SUPER::OnDeactivated();
}

void ezJoltConstraintComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  // s << m_fBreakForce;
  // s << m_fBreakTorque;
  s << m_bPairCollision;

  stream.WriteGameObjectHandle(m_hActorA);
  stream.WriteGameObjectHandle(m_hActorB);

  s << m_LocalFrameA;
  s << m_LocalFrameB;

  stream.WriteGameObjectHandle(m_hActorBAnchor);
}

void ezJoltConstraintComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());


  auto& s = stream.GetStream();

  // s >> m_fBreakForce;
  // s >> m_fBreakTorque;
  s >> m_bPairCollision;

  m_hActorA = stream.ReadGameObjectHandle();
  m_hActorB = stream.ReadGameObjectHandle();

  s >> m_LocalFrameA;
  s >> m_LocalFrameB;

  m_hActorBAnchor = stream.ReadGameObjectHandle();
}

void ezJoltConstraintComponent::SetParentActorReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetParentActor(resolver(szReference, GetHandle(), "ParentActor"));
}

void ezJoltConstraintComponent::SetChildActorReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetChildActor(resolver(szReference, GetHandle(), "ChildActor"));
}

void ezJoltConstraintComponent::SetChildActorAnchorReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetUserFlag(1, false); // local frame B is not valid
  m_hActorBAnchor = resolver(szReference, GetHandle(), "ChildActorAnchor");
}

void ezJoltConstraintComponent::SetParentActor(ezGameObjectHandle hActor)
{
  SetUserFlag(0, false); // local frame A is not valid
  m_hActorA = hActor;
}

void ezJoltConstraintComponent::SetChildActor(ezGameObjectHandle hActor)
{
  SetUserFlag(1, false); // local frame B is not valid
  m_hActorB = hActor;
}

void ezJoltConstraintComponent::SetChildActorAnchor(ezGameObjectHandle hActor)
{
  SetUserFlag(1, false); // local frame B is not valid
  m_hActorBAnchor = hActor;
}

void ezJoltConstraintComponent::SetActors(ezGameObjectHandle hActorA, const ezTransform& localFrameA, ezGameObjectHandle hActorB, const ezTransform& localFrameB)
{
  m_hActorA = hActorA;
  m_hActorB = hActorB;

  // prevent FindParentBody() and FindChildBody() from overwriting the local frames
  // local frame A and B are already valid
  SetUserFlag(0, true);
  SetUserFlag(1, true);

  m_LocalFrameA = localFrameA;
  m_LocalFrameB = localFrameB;
}

void ezJoltConstraintComponent::ApplySettings()
{
  SetUserFlag(2, false);

  // const float fBreakForce = m_fBreakForce <= 0.0f ? ezMath::MaxValue<float>() : m_fBreakForce;
  // const float fBreakTorque = m_fBreakTorque <= 0.0f ? ezMath::MaxValue<float>() : m_fBreakTorque;
  // m_pConstraint->setBreakForce(fBreakForce, fBreakTorque);

  // if (m_fBreakForce > 0.0f || m_fBreakTorque > 0.0f)
  //{
  //   ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
  //   pModule->m_BreakableConstraints[m_pConstraint->getConstraint()] = GetHandle();
  // }

  // m_pConstraint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, m_bPairCollision);

  // JoltRigidActor* pActor0 = nullptr;
  // JoltRigidActor* pActor1 = nullptr;
  // m_pConstraint->getActors(pActor0, pActor1);

  // if (pActor0 && pActor0->is<PxRigidDynamic>() && !static_cast<PxRigidDynamic*>(pActor0)->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
  //   static_cast<PxRigidDynamic*>(pActor0)->wakeUp();
  // if (pActor1 && pActor1->is<PxRigidDynamic>() && !static_cast<PxRigidDynamic*>(pActor1)->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
  //   static_cast<PxRigidDynamic*>(pActor1)->wakeUp();
}

ezResult ezJoltConstraintComponent::FindParentBody(ezUInt32& out_uiJoltBodyID)
{
  ezGameObject* pObject = nullptr;
  ezJoltDynamicActorComponent* pRbComp = nullptr;

  if (!m_hActorA.IsInvalidated())
  {
    if (!GetWorld()->TryGetObject(m_hActorA, pObject) || !pObject->IsActive())
    {
      ezLog::Error("{0} '{1}' parent reference is a non-existing object. Constraint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
      return EZ_FAILURE;
    }

    if (!pObject->TryGetComponentOfBaseType(pRbComp))
    {
      ezLog::Error("{0} '{1}' parent reference is an object without a ezJoltDynamicActorComponent. Constraint is ignored.", GetDynamicRTTI()->GetTypeName(),
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
      out_uiJoltBodyID = ezInvalidIndex;

      if (GetUserFlag(0) == false)
      {
        // m_localFrameA is now valid
        SetUserFlag(0, true);
        m_LocalFrameA = GetOwner()->GetGlobalTransform();
      }
      return EZ_SUCCESS;
    }
  }

  pRbComp->EnsureSimulationStarted();
  out_uiJoltBodyID = pRbComp->GetJoltBodyID();

  if (out_uiJoltBodyID == ezInvalidIndex)
  {
    ezLog::Error("{0} '{1}' parent reference is an object with an invalid ezJoltDynamicActorComponent. Constraint is ignored.",
      GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
    return EZ_FAILURE;
  }

  m_hActorA = pObject->GetHandle();

  if (GetUserFlag(0) == false)
  {
    // m_localFrameA is now valid
    SetUserFlag(0, true);
    m_LocalFrameA.SetLocalTransform(pObject->GetGlobalTransform(), GetOwner()->GetGlobalTransform());
    m_LocalFrameA.m_vPosition = m_LocalFrameA.m_vPosition.CompMul(pObject->GetGlobalScaling());
  }

  return EZ_SUCCESS;
}

ezResult ezJoltConstraintComponent::FindChildBody(ezUInt32& out_uiJoltBodyID)
{
  ezGameObject* pObject = nullptr;
  ezJoltDynamicActorComponent* pRbComp = nullptr;

  if (m_hActorB.IsInvalidated())
  {
    ezLog::Error("{0} '{1}' has no child reference. Constraint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
    return EZ_FAILURE;
  }

  if (!GetWorld()->TryGetObject(m_hActorB, pObject) || !pObject->IsActive())
  {
    ezLog::Error("{0} '{1}' child reference is a non-existing object. Constraint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
    return EZ_FAILURE;
  }

  if (!pObject->TryGetComponentOfBaseType(pRbComp))
  {
    // this makes it possible to link the Constraint to a prefab, because it may skip the top level hierarchy of the prefab
    pObject = pObject->SearchForChildByNameSequence("/", ezGetStaticRTTI<ezJoltDynamicActorComponent>());

    if (pObject == nullptr)
    {
      ezLog::Error("{0} '{1}' child reference is an object without a ezJoltDynamicActorComponent. Constraint is ignored.", GetDynamicRTTI()->GetTypeName(),
        GetOwner()->GetName());
      return EZ_FAILURE;
    }

    pObject->TryGetComponentOfBaseType(pRbComp);
  }

  pRbComp->EnsureSimulationStarted();
  out_uiJoltBodyID = pRbComp->GetJoltBodyID();

  if (out_uiJoltBodyID == ezInvalidIndex)
  {
    ezLog::Error("{0} '{1}' child reference is an object with an invalid ezJoltDynamicActorComponent. Constraint is ignored.",
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
        ezLog::Error("{0} '{1}' anchor reference is a non-existing object. Constraint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
        return EZ_FAILURE;
      }
    }

    // m_localFrameB is now valid
    SetUserFlag(1, true);
    m_LocalFrameB.SetLocalTransform(pObject->GetGlobalTransform(), pAnchorObject->GetGlobalTransform());
    m_LocalFrameB.m_vPosition = m_LocalFrameB.m_vPosition.CompMul(pObject->GetGlobalScaling());
  }

  return EZ_SUCCESS;
}

ezTransform ezJoltConstraintComponent::ComputeParentBodyGlobalFrame() const
{
  if (!m_hActorA.IsInvalidated())
  {
    const ezGameObject* pObject = nullptr;
    if (GetWorld()->TryGetObject(m_hActorA, pObject))
    {
      ezTransform res;
      res.SetGlobalTransform(pObject->GetGlobalTransform(), m_LocalFrameA);
      return res;
    }
  }

  return m_LocalFrameA;
}

ezTransform ezJoltConstraintComponent::ComputeChildBodyGlobalFrame() const
{
  if (!m_hActorB.IsInvalidated())
  {
    const ezGameObject* pObject = nullptr;
    if (GetWorld()->TryGetObject(m_hActorB, pObject))
    {
      ezTransform res;
      res.SetGlobalTransform(pObject->GetGlobalTransform(), m_LocalFrameB);
      return res;
    }
  }

  return m_LocalFrameB;
}

void ezJoltConstraintComponent::QueueApplySettings()
{
  if (m_pConstraint == nullptr)
    return;

  // already in queue ?
  if (GetUserFlag(2))
    return;

  SetUserFlag(2, true);

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
  pModule->m_RequireUpdate.PushBack(GetHandle());
}
