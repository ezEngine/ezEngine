#include <PhysXPluginPCH.h>

#include <Components/PxDynamicActorComponent.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/GrabbableItemComponent.h>
#include <PhysXPlugin/Components/PxObjectGrabComponent.h>
#include <PhysXPlugin/Joints/Px6DOFJointComponent.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>

using namespace physx;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxGrabObjectComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("SpringStiffness", m_fSpringStiffness)->AddAttributes(new ezDefaultValueAttribute(50.0f)),
    EZ_MEMBER_PROPERTY("SpringDamping", m_fSpringDamping)->AddAttributes(new ezDefaultValueAttribute(10.0f)),
    EZ_MEMBER_PROPERTY("BreakDistance", m_fBreakDistance)->AddAttributes(new ezDefaultValueAttribute(0.5f)),
    EZ_ACCESSOR_PROPERTY("AttachTo", DummyGetter, SetAttachToReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(GrabNearbyObject),
    EZ_SCRIPT_FUNCTION_PROPERTY(HasObjectGrabbed),
    EZ_SCRIPT_FUNCTION_PROPERTY(DropGrabbedObject),
    EZ_SCRIPT_FUNCTION_PROPERTY(ThrowGrabbedObject, In, "Direction"),
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
ezPxGrabObjectComponent::~ezPxGrabObjectComponent() = default;

void ezPxGrabObjectComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_fBreakDistance;
  s << m_fSpringStiffness;
  s << m_fSpringDamping;

  stream.WriteGameObjectHandle(m_hAttachTo);
}

void ezPxGrabObjectComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_fBreakDistance;
  s >> m_fSpringStiffness;
  s >> m_fSpringDamping;

  m_hAttachTo = stream.ReadGameObjectHandle();
}

bool ezPxGrabObjectComponent::GrabNearbyObject()
{
  if (!m_hJoint.IsInvalidated())
    return false;

  auto pOwner = GetOwner();

  ezPxDynamicActorComponent* pClosestActor = nullptr;
  {
    ezPhysicsWorldModuleInterface* pPhysicsModule = GetWorld()->GetOrCreateModule<ezPhysicsWorldModuleInterface>();

    ezPhysicsCastResult hit;
    ezPhysicsQueryParameters queryParam;
    queryParam.m_bIgnoreInitialOverlap = true;
    queryParam.m_uiCollisionLayer = 0; // TODO
    queryParam.m_ShapeTypes = ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic;

    if (!pPhysicsModule->Raycast(hit, pOwner->GetGlobalPosition(), pOwner->GetGlobalDirForwards().GetNormalized(), 5.0f /* TODO*/, queryParam))
      return false;

    ezGameObject* pActorObj;
    if (!GetWorld()->TryGetObject(hit.m_hActorObject, pActorObj))
      return false;

    if (!pActorObj->TryGetComponentOfBaseType(pClosestActor))
      return false;

    if (pClosestActor->GetKinematic())
      return false;

    ezGrabbableItemComponent* pGrabbable;
    if (!pActorObj->TryGetComponentOfBaseType(pGrabbable))
      return false;

    m_ChildAnchorLocal.SetIdentity();

    if (!pGrabbable->m_GrabPoints.IsEmpty())
    {
      float fBestScore = 0.0f;
      ezUInt32 uiBestPoint = 0;

      const ezTransform parentTransform = pActorObj->GetGlobalTransform();

      for (ezUInt32 i = 0; i < pGrabbable->m_GrabPoints.GetCount(); ++i)
      {
        const ezVec3 vGlobalPos = parentTransform.TransformPosition(pGrabbable->m_GrabPoints[i].m_vLocalPosition);
        const ezQuat qGlobalRot = parentTransform.m_qRotation * pGrabbable->m_GrabPoints[i].m_qLocalRotation;

        const ezVec3 vDir = qGlobalRot * ezVec3(1, 0, 0);

        float fScore = 1.0f / (vGlobalPos - pOwner->GetGlobalPosition()).GetLengthSquared();

        if (fScore > fBestScore)
        {
          fBestScore = fScore;
          uiBestPoint = i;
        }
      }

      m_ChildAnchorLocal.m_vPosition = pGrabbable->m_GrabPoints[uiBestPoint].m_vLocalPosition;
      m_ChildAnchorLocal.m_qRotation = pGrabbable->m_GrabPoints[uiBestPoint].m_qLocalRotation;
    }
  }

  ezPxDynamicActorComponent* pParentActor = nullptr;
  ezGameObject* pAttachToObj = nullptr;
  {
    if (!GetWorld()->TryGetObject(m_hAttachTo, pAttachToObj))
      return false;

    if (!pAttachToObj->TryGetComponentOfBaseType(pParentActor))
      return false;

    if (!pParentActor->GetKinematic())
      return false;
  }

  EZ_PX_WRITE_LOCK(*(pClosestActor->GetPxActor()->getScene()));

  m_hGrabbedActor = pClosestActor->GetHandle();
  m_fPrevMass = pClosestActor->GetMass();
  pClosestActor->SetMass(0.1f);

  m_bPrevGravity = !pClosestActor->GetDisableGravity();
  pClosestActor->SetDisableGravity(true);

  {
    ezPx6DOFJointComponent* pJoint = nullptr;
    m_hJoint = GetWorld()->GetOrCreateComponentManager<ezPx6DOFJointComponentManager>()->CreateComponent(pAttachToObj, pJoint);

    // TODO: swing/twist leeway (also to prevent object from flying away)
    // locked rotation feels better than springy rotation, but can result in objects being flung away on pickup
    // could probably use spring at pickup, wait till object is rotated nicely, and then lock the rotation axis
    pJoint->SetFreeAngularAxis(ezPxAxis::All);
    //pJoint->SetSwingLimitMode(ezPxJointLimitMode::HardLimit);
    //pJoint->SetSwingLimit(ezAngle::Degree(175));
    //pJoint->SetSwingStiffness(m_fSpringStiffness);
    //pJoint->SetSwingDamping(m_fSpringDamping);
    //pJoint->SetTwistLimitMode(ezPxJointLimitMode::HardLimit);
    //pJoint->SetLowerTwistLimit(ezAngle::Degree(-175));
    //pJoint->SetUpperTwistLimit(ezAngle::Degree(+175));
    //pJoint->SetTwistStiffness(m_fSpringStiffness);
    //pJoint->SetTwistDamping(m_fSpringDamping);

    pJoint->SetFreeLinearAxis(ezPxAxis::X | ezPxAxis::Y | ezPxAxis::Z);
    pJoint->SetLinearLimitMode(ezPxJointLimitMode::SoftLimit);

    // TODO: use drive ?
    pJoint->SetLinearStiffness(m_fSpringStiffness);
    pJoint->SetLinearDamping(m_fSpringDamping);

    pJoint->SetActors(pParentActor->GetOwner()->GetHandle(), ezTransform::IdentityTransform(), pClosestActor->GetOwner()->GetHandle(), m_ChildAnchorLocal);

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

void ezPxGrabObjectComponent::ThrowGrabbedObject(const ezVec3& vRelativeDir)
{
  ezPxDynamicActorComponent* pActor;
  if (GetWorld()->TryGetComponent(m_hGrabbedActor, pActor))
  {
    pActor->AddLinearImpulse(GetOwner()->GetGlobalRotation() * vRelativeDir);
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

void ezPxGrabObjectComponent::SetAttachToReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hAttachTo = resolver(szReference, GetHandle(), "AttachTo");
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
    pActor->SetDisableGravity(!m_bPrevGravity);
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
  ezPx6DOFJointComponent* pJoint;
  if (!GetWorld()->TryGetComponent(m_hGrabbedActor, pActor) ||
      !GetWorld()->TryGetComponent(m_hJoint, pJoint))
  {
    ReleaseGrabbedObject();
    return;
  }

  const ezVec3 vAnchorPos = pActor->GetOwner()->GetGlobalTransform().TransformPosition(m_ChildAnchorLocal.m_vPosition);
  const ezVec3 vJointPos = pJoint->GetOwner()->GetGlobalPosition();
  const float fDistance = (vAnchorPos - vJointPos).GetLength();

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

  if (PxD6Joint* pJointD6 = static_cast<PxD6Joint*>(pJoint->GetPxJoint()))
  {
    pJointD6->setDrive(PxD6Drive::eSLERP, PxD6JointDrive(m_fSpringStiffness, m_fSpringDamping, ezMath::MaxValue<float>(), true));
    //pJointD6->setDrive(PxD6Drive::eSWING, PxD6JointDrive(m_fSpringStiffness, m_fSpringDamping, ezMath::MaxValue<float>(), true));
    //pJointD6->setDrive(PxD6Drive::eTWIST, PxD6JointDrive(m_fSpringStiffness, m_fSpringDamping, ezMath::MaxValue<float>(), true));
  }
}
