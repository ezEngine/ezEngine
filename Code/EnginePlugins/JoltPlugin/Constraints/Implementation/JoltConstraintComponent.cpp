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
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezJoltConstraintComponent, 2)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("PairCollision", GetPairCollision, SetPairCollision)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("ParentActor", DummyGetter, SetParentActorReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
    EZ_ACCESSOR_PROPERTY("ChildActor", DummyGetter, SetChildActorReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
    EZ_ACCESSOR_PROPERTY("ChildActorAnchor", DummyGetter, SetChildActorAnchorReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
    EZ_ACCESSOR_PROPERTY("BreakForce", GetBreakForce, SetBreakForce),
    EZ_ACCESSOR_PROPERTY("BreakTorque", GetBreakTorque, SetBreakTorque),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Jolt/Constraints"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezJoltMsgDisconnectConstraints, OnJoltMsgDisconnectConstraints),
  }
  EZ_END_MESSAGEHANDLERS;
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

void ezJoltConstraintComponent::BreakConstraint()
{
  if (m_pConstraint == nullptr)
    return;

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
  pModule->GetJoltSystem()->RemoveConstraint(m_pConstraint);

  pModule->m_BreakableConstraints.Remove(GetHandle());

  // wake up the joined bodies, so that removing a constraint doesn't let them hang in the air
  {
    JPH::BodyID bodies[2] = {JPH::BodyID(JPH::BodyID::cInvalidBodyID), JPH::BodyID(JPH::BodyID::cInvalidBodyID)};
    ezInt32 iBodies = 0;

    if (!m_hActorA.IsInvalidated())
    {
      ezGameObject* pObject = nullptr;
      ezJoltDynamicActorComponent* pRbComp = nullptr;

      if (GetWorld()->TryGetObject(m_hActorA, pObject) && pObject->IsActive() && pObject->TryGetComponentOfBaseType(pRbComp))
      {
        bodies[iBodies] = JPH::BodyID(pRbComp->GetJoltBodyID());
        ++iBodies;

        pRbComp->RemoveConstraint(GetHandle());
      }
    }

    if (!m_hActorB.IsInvalidated())
    {
      ezGameObject* pObject = nullptr;
      ezJoltDynamicActorComponent* pRbComp = nullptr;

      if (GetWorld()->TryGetObject(m_hActorB, pObject) && pObject->IsActive() && pObject->TryGetComponentOfBaseType(pRbComp))
      {
        bodies[iBodies] = JPH::BodyID(pRbComp->GetJoltBodyID());
        ++iBodies;

        pRbComp->RemoveConstraint(GetHandle());
      }
    }

    if (iBodies > 0)
    {
      pModule->GetJoltSystem()->GetBodyInterface().ActivateBodies(bodies, iBodies);
    }
  }

  m_pConstraint->Release();
  m_pConstraint = nullptr;
}

void ezJoltConstraintComponent::SetBreakForce(float value)
{
  m_fBreakForce = value;
  QueueApplySettings();
}

void ezJoltConstraintComponent::SetBreakTorque(float value)
{
  m_fBreakTorque = value;
  QueueApplySettings();
}

void ezJoltConstraintComponent::SetPairCollision(bool value)
{
  m_bPairCollision = value;
  QueueApplySettings();
}

void ezJoltConstraintComponent::OnSimulationStarted()
{
  ezUInt32 uiBodyIdA = ezInvalidIndex;
  ezUInt32 uiBodyIdB = ezInvalidIndex;

  ezJoltDynamicActorComponent* pRbParent = nullptr;
  ezJoltDynamicActorComponent* pRbChild = nullptr;

  if (FindParentBody(uiBodyIdA, pRbParent).Failed())
    return;

  if (FindChildBody(uiBodyIdB, pRbChild).Failed())
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

    if (pRbParent)
    {
      pRbParent->AddConstraint(GetHandle());
    }

    if (pRbChild)
    {
      pRbChild->AddConstraint(GetHandle());
    }
  }
}

void ezJoltConstraintComponent::OnDeactivated()
{
  BreakConstraint();

  SUPER::OnDeactivated();
}

void ezJoltConstraintComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_bPairCollision;

  inout_stream.WriteGameObjectHandle(m_hActorA);
  inout_stream.WriteGameObjectHandle(m_hActorB);

  s << m_LocalFrameA;
  s << m_LocalFrameB;

  inout_stream.WriteGameObjectHandle(m_hActorBAnchor);

  s << m_fBreakForce;
  s << m_fBreakTorque;
}

void ezJoltConstraintComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_bPairCollision;

  m_hActorA = inout_stream.ReadGameObjectHandle();
  m_hActorB = inout_stream.ReadGameObjectHandle();

  s >> m_LocalFrameA;
  s >> m_LocalFrameB;

  m_hActorBAnchor = inout_stream.ReadGameObjectHandle();

  if (uiVersion >= 2)
  {
    s >> m_fBreakForce;
    s >> m_fBreakTorque;
  }
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

  if (m_fBreakForce > 0.0f || m_fBreakTorque > 0.0f)
  {
    ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
    pModule->m_BreakableConstraints.Insert(GetHandle());
  }
  else
  {
    ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
    pModule->m_BreakableConstraints.Remove(GetHandle());
  }
}

void ezJoltConstraintComponent::OnJoltMsgDisconnectConstraints(ezJoltMsgDisconnectConstraints& ref_msg)
{
  BreakConstraint();
}

ezResult ezJoltConstraintComponent::FindParentBody(ezUInt32& out_uiJoltBodyID, ezJoltDynamicActorComponent*& pRbComp)
{
  ezGameObject* pObject = nullptr;
  pRbComp = nullptr;

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
    else
    {
      EZ_ASSERT_DEBUG(pObject != nullptr, "pRbComp and pObject should always be valid together");
      if (GetUserFlag(0) == true)
      {
        ezTransform globalFrame = m_LocalFrameA;

        // m_localFrameA is already valid
        // assume it was in global space and move it into local space of the found parent
        m_LocalFrameA = ezTransform::MakeLocalTransform(pRbComp->GetOwner()->GetGlobalTransform(), globalFrame);
        m_LocalFrameA.m_vPosition = m_LocalFrameA.m_vPosition.CompMul(pObject->GetGlobalScaling());
      }
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
    m_LocalFrameA = ezTransform::MakeLocalTransform(pObject->GetGlobalTransform(), GetOwner()->GetGlobalTransform());
    m_LocalFrameA.m_vPosition = m_LocalFrameA.m_vPosition.CompMul(pObject->GetGlobalScaling());
  }

  return EZ_SUCCESS;
}

ezResult ezJoltConstraintComponent::FindChildBody(ezUInt32& out_uiJoltBodyID, ezJoltDynamicActorComponent*& pRbComp)
{
  ezGameObject* pObject = nullptr;
  pRbComp = nullptr;

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
    m_LocalFrameB = ezTransform::MakeLocalTransform(pObject->GetGlobalTransform(), pAnchorObject->GetGlobalTransform());
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
      res = ezTransform::MakeGlobalTransform(pObject->GetGlobalTransform(), m_LocalFrameA);
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
      res = ezTransform::MakeGlobalTransform(pObject->GetGlobalTransform(), m_LocalFrameB);
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


EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_JoltConstraintComponent);
