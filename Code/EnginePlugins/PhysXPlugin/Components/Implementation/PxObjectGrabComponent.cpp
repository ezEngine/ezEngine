#include <PhysXPluginPCH.h>

#include <Components/PxDynamicActorComponent.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Components/PxObjectGrabComponent.h>
#include <PhysXPlugin/Joints/Px6DOFJointComponent.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>

using namespace physx;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxGrabObjectComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("BreakDistance", m_fBreakDistance)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("SpringStiffness", m_fSpringStiffness)->AddAttributes(new ezDefaultValueAttribute(50.0f)),
    EZ_MEMBER_PROPERTY("SpringDamping", m_fSpringDamping)->AddAttributes(new ezDefaultValueAttribute(10.0f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(GrabNearbyObject),
    EZ_SCRIPT_FUNCTION_PROPERTY(HasObjectGrabbed),
    EZ_SCRIPT_FUNCTION_PROPERTY(DropGrabbedObject),
    EZ_SCRIPT_FUNCTION_PROPERTY(ThrowGrabbedObject),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Special"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezPxGrabObjectComponent::ezPxGrabObjectComponent() = default;

void ezPxGrabObjectComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_fBreakDistance;
  s << m_fSpringStiffness;
  s << m_fSpringDamping;
}

void ezPxGrabObjectComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_fBreakDistance;
  s >> m_fSpringStiffness;
  s >> m_fSpringDamping;
}

static ezPxDynamicActorComponent* FindActor(ezGameObject* pObject)
{
  while (pObject != nullptr)
  {
    ezPxDynamicActorComponent* pActor;
    if (pObject->TryGetComponentOfBaseType(pActor))
    {
      if (pActor->GetKinematic())
        return nullptr;

      return pActor;
    }

    pObject = pObject->GetParent();
  }

  return nullptr;
}

bool ezPxGrabObjectComponent::GrabNearbyObject()
{
  if (!m_hJoint.IsInvalidated())
    return false;

  auto pOwner = GetOwner();

  // TODO: make configurable / allow multiple / prefer some over others (?)
  // e.g. have one marker that also orients, another that just grabs without rotation (requires to adjust joint rotation)
  auto marker = ezSpatialData::RegisterCategory("GrabObjectMarker");

  ezHybridArray<ezGameObject*, 16> objects;

  GetWorld()->GetSpatialSystem()->FindObjectsInSphere(ezBoundingSphere(pOwner->GetGlobalPosition(), 1.0f), marker.GetBitmask(), objects);

  if (objects.IsEmpty())
    return false;

  const ezVec3 vOwnPos = pOwner->GetGlobalPosition();

  float fClosest = ezMath::MaxValue<float>();
  ezGameObject* pClosestMarker = nullptr;
  ezPxDynamicActorComponent* pClosestActor = nullptr;

  for (auto pObj : objects)
  {
    const float fDist = (vOwnPos - pObj->GetGlobalPosition()).GetLengthSquared();

    if (fDist < fClosest)
    {
      if (auto pActor = FindActor(pObj))
      {
        pClosestMarker = pObj;
        pClosestActor = pActor;
        fClosest = fDist;
      }
    }
  }

  if (pClosestActor == nullptr)
    return false;

  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();

  ezPxDynamicActorComponent* pParentActor = nullptr;
  {
    if (!pOwner->TryGetComponentOfBaseType(pParentActor))
      return false;

    if (!pParentActor->GetKinematic())
      return false;
  }

  EZ_PX_WRITE_LOCK(*(pClosestActor->GetPxActor()->getScene()));

  m_hGrabbedActor = pClosestActor->GetHandle();
  m_fPrevMass = pClosestActor->GetMass();
  pClosestActor->SetMass(0.1f);

  {
    ezPx6DOFJointComponent* pJoint = nullptr;
    m_hJoint = GetWorld()->GetOrCreateComponentManager<ezPx6DOFJointComponentManager>()->CreateComponent(pOwner, pJoint);

    // TODO: swing/twist leeway (also to prevent object from flying away)
    // locked rotation feels better than springy rotation, but can result in objects being flung away on pickup
    // could probably use spring at pickup, wait till object is rotated nicely, and then lock the rotation axis
    pJoint->SetFreeAngularAxis(ezPxAxis::All);
    pJoint->SetSwingLimitMode(ezPxJointLimitMode::SoftLimit);
    pJoint->SetSwingStiffness(m_fSpringStiffness);
    pJoint->SetSwingDamping(m_fSpringDamping);
    pJoint->SetTwistLimitMode(ezPxJointLimitMode::SoftLimit);
    pJoint->SetTwistStiffness(m_fSpringStiffness);
    pJoint->SetTwistDamping(m_fSpringDamping);

    pJoint->SetFreeLinearAxis(ezPxAxis::X | ezPxAxis::Y | ezPxAxis::Z);
    pJoint->SetLinearLimitMode(ezPxJointLimitMode::SoftLimit);

    // TODO: use drive ?
    pJoint->SetLinearStiffness(m_fSpringStiffness);
    pJoint->SetLinearDamping(m_fSpringDamping);

    pJoint->SetParentActor(pParentActor->GetOwner()->GetHandle());
    pJoint->SetChildActor(pClosestActor->GetOwner()->GetHandle());
    pJoint->SetChildActorAnchor(pClosestMarker->GetHandle());

    pClosestActor->GetPxActor()->getSolverIterationCounts(m_uiPrevPosIterations, m_uiPrevVelIterations);
    pClosestActor->GetPxActor()->setSolverIterationCounts(16, 16);
  }

  m_AboveBreakdistanceSince = GetWorld()->GetClock().GetAccumulatedTime();

  return true;
}

bool ezPxGrabObjectComponent::HasObjectGrabbed() const
{
  return !m_hJoint.IsInvalidated();
}

void ezPxGrabObjectComponent::DropGrabbedObject()
{
  ReleaseGrabbedObject();
}

void ezPxGrabObjectComponent::ThrowGrabbedObject(/*const ezVec3& vRelativeDir*/)
{
  ezPxDynamicActorComponent* pActor;
  if (GetWorld()->TryGetComponent(m_hGrabbedActor, pActor))
  {
    // TODO: fix TypeScript crash with parameter
    pActor->AddLinearImpulse(GetOwner()->GetGlobalDirForwards());
  }

  ReleaseGrabbedObject();
}

void ezPxGrabObjectComponent::BreakObjectGrab()
{
  ReleaseGrabbedObject();

  ezMsgPhysicsJointBroke msg;
  msg.m_hJointObject = GetOwner()->GetHandle();

  GetOwner()->PostEventMessage(msg, this, ezTime::Zero());
}

void ezPxGrabObjectComponent::ReleaseGrabbedObject()
{
  if (m_hJoint.IsInvalidated())
    return;

  ezPxDynamicActorComponent* pActor;
  if (GetWorld()->TryGetComponent(m_hGrabbedActor, pActor))
  {
    EZ_PX_WRITE_LOCK(*(pActor->GetPxActor()->getScene()));

    pActor->SetMass(m_fPrevMass); // TODO: not sure that's the best way to reset the mass
    pActor->GetPxActor()->setSolverIterationCounts(m_uiPrevPosIterations, m_uiPrevVelIterations);
  }

  ezComponent* pJoint;
  if (GetWorld()->TryGetComponent(m_hJoint, pJoint))
  {
    pJoint->GetOwningManager()->DeleteComponent(pJoint);
  }

  m_hJoint.Invalidate();
  m_hGrabbedActor.Invalidate();
}

void ezPxGrabObjectComponent::Update()
{
  if (m_hJoint.IsInvalidated())
    return;

  ezPxDynamicActorComponent* pActor;
  if (!GetWorld()->TryGetComponent(m_hGrabbedActor, pActor))
  {
    ReleaseGrabbedObject();
    return;
  }

  ezComponent* pJoint;
  if (!GetWorld()->TryGetComponent(m_hJoint, pJoint))
  {
    ReleaseGrabbedObject();
    return;
  }

  const ezVec3 vActorPos = pActor->GetOwner()->GetGlobalPosition();
  const ezVec3 vJointPos = pJoint->GetOwner()->GetGlobalPosition();
  const float fDistance = (vActorPos - vJointPos).GetLength();

  if (fDistance < m_fBreakDistance)
  {
    m_AboveBreakdistanceSince = GetWorld()->GetClock().GetAccumulatedTime();
  }
  else if (fDistance > m_fBreakDistance * 2.0f) // TODO: make this configurable?
  {
    BreakObjectGrab();
    return;
  }
  else
  {
    // TODO: make this configurable?
    if (GetWorld()->GetClock().GetAccumulatedTime() - m_AboveBreakdistanceSince > ezTime::Seconds(1.0))
    {
      BreakObjectGrab();
      return;
    }
  }
}
