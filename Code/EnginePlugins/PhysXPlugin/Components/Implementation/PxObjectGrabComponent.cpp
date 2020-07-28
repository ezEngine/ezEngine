#include <PhysXPluginPCH.h>

#include <Components/PxDynamicActorComponent.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/GrabbableItemComponent.h>
#include <PhysXPlugin/Components/PxObjectGrabComponent.h>
#include <PhysXPlugin/Joints/Px6DOFJointComponent.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
#include <RendererCore/Debug/DebugRenderer.h>

using namespace physx;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxGrabObjectComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("RaycastDistance", m_fRaycastDistance)->AddAttributes(new ezDefaultValueAttribute(5.0f)),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
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
  s << m_fRaycastDistance;
  s << m_uiCollisionLayer;

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
  s >> m_fRaycastDistance;
  s >> m_uiCollisionLayer;

  m_hAttachTo = stream.ReadGameObjectHandle();
}

bool ezPxGrabObjectComponent::GrabNearbyObject()
{
  if (!m_hJoint.IsInvalidated())
    return false;

  ezPxDynamicActorComponent* pAttachToActor = GetAttachToActor();
  if (pAttachToActor == nullptr)
    return false;

  ezPxDynamicActorComponent* pActorToGrab = FindGrabbableActor();

  if (pActorToGrab == nullptr)
    return false;

  if (DetermineGrabPoint(pActorToGrab).Failed())
    return false;

  EZ_PX_WRITE_LOCK(*(pActorToGrab->GetPxActor()->getScene()));

  AdjustGrabbedActor(pActorToGrab);

  CreateJoint(pAttachToActor, pActorToGrab);

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

  ezPxDynamicActorComponent* pGrabbedActor = nullptr;
  if (GetWorld()->TryGetComponent(m_hGrabbedActor, pGrabbedActor))
  {
    EZ_PX_WRITE_LOCK(*(pGrabbedActor->GetPxActor()->getScene()));

    pGrabbedActor->SetMass(m_fGrabbedActorMass); // TODO: not sure that's the best way to reset the mass
    pGrabbedActor->SetDisableGravity(!m_bGrabbedActorGravity);
    pGrabbedActor->GetPxActor()->setSolverIterationCounts(m_uiGrabbedActorPosIterations, m_uiGrabbedActorVelIterations);
  }

  ezComponent* pJoint = nullptr;
  if (GetWorld()->TryGetComponent(m_hJoint, pJoint))
  {
    pJoint->GetOwningManager()->DeleteComponent(pJoint);
  }

  m_hJoint.Invalidate();
  m_hGrabbedActor.Invalidate();
}

ezPxDynamicActorComponent* ezPxGrabObjectComponent::FindGrabbableActor()
{
  const ezPhysicsWorldModuleInterface* pPhysicsModule = GetWorld()->GetModule<ezPhysicsWorldModuleInterface>();

  if (pPhysicsModule == nullptr)
    return nullptr;

  auto pOwner = GetOwner();

  ezPhysicsCastResult hit;
  ezPhysicsQueryParameters queryParam;
  queryParam.m_bIgnoreInitialOverlap = true;
  queryParam.m_uiCollisionLayer = m_uiCollisionLayer;
  queryParam.m_ShapeTypes = ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic;

  if (!pPhysicsModule->Raycast(hit, pOwner->GetGlobalPosition(), pOwner->GetGlobalDirForwards().GetNormalized(), m_fRaycastDistance, queryParam))
    return nullptr;

  ezGameObject* pActorObj;
  if (!GetWorld()->TryGetObject(hit.m_hActorObject, pActorObj))
    return nullptr;

  ezPxDynamicActorComponent* pActorComp = nullptr;
  if (!pActorObj->TryGetComponentOfBaseType(pActorComp))
    return nullptr;

  if (pActorComp->GetKinematic())
    return nullptr;

  return pActorComp;
}

ezPxDynamicActorComponent* ezPxGrabObjectComponent::GetAttachToActor()
{
  ezPxDynamicActorComponent* pActor = nullptr;
  ezGameObject* pObject = nullptr;

  if (!GetWorld()->TryGetObject(m_hAttachTo, pObject))
    return nullptr;

  if (!pObject->TryGetComponentOfBaseType(pActor))
    return nullptr;

  if (!pActor->GetKinematic())
    return nullptr;

  return pActor;
}

ezResult ezPxGrabObjectComponent::DetermineGrabPoint(ezPxDynamicActorComponent* pActorComp)
{
  m_ChildAnchorLocal.SetIdentity();

  const auto pOwner = GetOwner();
  const auto vOwnerPos = pOwner->GetGlobalPosition();
  const auto vOwnerDir = pOwner->GetGlobalDirForwards();
  const auto vOwnerUp = pOwner->GetGlobalDirUp();
  const auto pActorObj = pActorComp->GetOwner();

  const ezTransform& actorTransform = pActorObj->GetGlobalTransform();
  ezHybridArray<ezGrabbableItemGrabPoint, 16> grabPoints;

  const ezGrabbableItemComponent* pGrabbableItemComp = nullptr;
  if (pActorObj->TryGetComponentOfBaseType(pGrabbableItemComp) && !pGrabbableItemComp->m_GrabPoints.IsEmpty())
  {
    grabPoints = pGrabbableItemComp->m_GrabPoints;
  }
  else if (pActorComp->GetMass() <= m_fAllowGrabAnyObjectWithMass)
  {
    const auto& box = pActorComp->GetOwner()->GetLocalBounds().GetBox();
    const ezVec3& center = box.GetCenter();
    const ezVec3& halfExt = box.GetHalfExtents();

    grabPoints.SetCount(4);
    grabPoints[0].m_vLocalPosition.Set(-halfExt.x, 0, 0);
    grabPoints[0].m_qLocalRotation.SetShortestRotation(ezVec3::UnitXAxis(), ezVec3::UnitXAxis());
    grabPoints[1].m_vLocalPosition.Set(+halfExt.x, 0, 0);
    grabPoints[1].m_qLocalRotation.SetShortestRotation(ezVec3::UnitXAxis(), -ezVec3::UnitXAxis());
    grabPoints[2].m_vLocalPosition.Set(0, -halfExt.y, 0);
    grabPoints[2].m_qLocalRotation.SetShortestRotation(ezVec3::UnitXAxis(), ezVec3::UnitYAxis());
    grabPoints[3].m_vLocalPosition.Set(0, +halfExt.y, 0);
    grabPoints[3].m_qLocalRotation.SetShortestRotation(ezVec3::UnitXAxis(), -ezVec3::UnitYAxis());
    //grabPoints[4].m_vLocalPosition.Set(0, 0, -halfExt.z);
    //grabPoints[4].m_qLocalRotation.SetShortestRotation(ezVec3::UnitXAxis(), ezVec3::UnitZAxis());
    //grabPoints[5].m_vLocalPosition.Set(0, 0, +halfExt.z);
    //grabPoints[5].m_qLocalRotation.SetShortestRotation(ezVec3::UnitXAxis(), -ezVec3::UnitZAxis());

    for (ezUInt32 i = 0; i < grabPoints.GetCount(); ++i)
    {
      grabPoints[i].m_vLocalPosition += center;
    }
  }

  if (grabPoints.IsEmpty())
    return EZ_FAILURE;

  ezUInt32 uiBestPointIndex = 0;
  float fBestScore = -1000.0f;

  for (ezUInt32 i = 0; i < grabPoints.GetCount(); ++i)
  {
    const ezVec3 vGrabPointPos = actorTransform.TransformPosition(grabPoints[i].m_vLocalPosition);
    const ezQuat qGrabPointRot = actorTransform.m_qRotation * grabPoints[i].m_qLocalRotation;
    const ezVec3 vGrabPointDir = qGrabPointRot * ezVec3(1, 0, 0);
    const ezVec3 vGrabPointUp = qGrabPointRot * ezVec3(0, 0, 1);

    float fScore = 1.0f - (vGrabPointPos - vOwnerPos).GetLengthSquared();
    fScore += vGrabPointDir.Dot(vOwnerDir);
    fScore += vGrabPointUp.Dot(vOwnerUp) * 0.5f; // up has less weight than forward

    if (fScore > fBestScore)
    {
      uiBestPointIndex = i;
      fBestScore = fScore;
    }
  }

  m_ChildAnchorLocal.m_vPosition = grabPoints[uiBestPointIndex].m_vLocalPosition;
  m_ChildAnchorLocal.m_qRotation = grabPoints[uiBestPointIndex].m_qLocalRotation;

  return EZ_SUCCESS;
}

void ezPxGrabObjectComponent::AdjustGrabbedActor(ezPxDynamicActorComponent* pActor)
{
  m_hGrabbedActor = pActor->GetHandle();

  m_fGrabbedActorMass = pActor->GetMass();
  pActor->SetMass(0.1f);

  m_bGrabbedActorGravity = !pActor->GetDisableGravity();
  pActor->SetDisableGravity(true);

  pActor->GetPxActor()->getSolverIterationCounts(m_uiGrabbedActorPosIterations, m_uiGrabbedActorVelIterations);
  pActor->GetPxActor()->setSolverIterationCounts(16, 16);
}

void ezPxGrabObjectComponent::CreateJoint(ezPxDynamicActorComponent* pParent, ezPxDynamicActorComponent* pChild)
{
  ezPx6DOFJointComponent* pJoint = nullptr;
  m_hJoint = GetWorld()->GetOrCreateComponentManager<ezPx6DOFJointComponentManager>()->CreateComponent(pParent->GetOwner(), pJoint);

  pJoint->SetFreeAngularAxis(ezPxAxis::All);
  pJoint->SetFreeLinearAxis(ezPxAxis::All);

  ezTransform tAnchor = m_ChildAnchorLocal;
  tAnchor.m_vPosition = tAnchor.m_vPosition.CompMul(pChild->GetOwner()->GetGlobalScaling());
  pJoint->SetActors(pParent->GetOwner()->GetHandle(), ezTransform::IdentityTransform(), pChild->GetOwner()->GetHandle(), tAnchor);
}

void ezPxGrabObjectComponent::DetectDistanceViolation(ezPxDynamicActorComponent* pGrabbedActor, ezPx6DOFJointComponent* pJoint)
{
  if (m_fBreakDistance <= 0)
    return;

  const ezVec3 vAnchorPos = pGrabbedActor->GetOwner()->GetGlobalTransform().TransformPosition(m_ChildAnchorLocal.m_vPosition);
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
}

void ezPxGrabObjectComponent::Update()
{
  if (m_hJoint.IsInvalidated())
    return;

  ezPxDynamicActorComponent* pGrabbedActor;
  ezPx6DOFJointComponent* pJoint;
  if (!GetWorld()->TryGetComponent(m_hGrabbedActor, pGrabbedActor) ||
      !GetWorld()->TryGetComponent(m_hJoint, pJoint))
  {
    BreakObjectGrab();
    return;
  }

  if (PxD6Joint* pJointD6 = static_cast<PxD6Joint*>(pJoint->GetPxJoint()))
  {
    pJointD6->setDrive(PxD6Drive::eX, PxD6JointDrive(m_fSpringStiffness, m_fSpringDamping, ezMath::MaxValue<float>(), true));
    pJointD6->setDrive(PxD6Drive::eY, PxD6JointDrive(m_fSpringStiffness, m_fSpringDamping, ezMath::MaxValue<float>(), true));
    pJointD6->setDrive(PxD6Drive::eZ, PxD6JointDrive(m_fSpringStiffness, m_fSpringDamping, ezMath::MaxValue<float>(), true));
    pJointD6->setDrive(PxD6Drive::eSLERP, PxD6JointDrive(m_fSpringStiffness, m_fSpringDamping, ezMath::MaxValue<float>(), true));
  }

  DetectDistanceViolation(pGrabbedActor, pJoint);
}

// TODO: figure out whether grab point is too far away
// TODO: (script) function to return potential grab object / point
// TODO: prevent character controller from jumping onto grabbed object
// TODO: choose grab point by rotation
// TODO: grab any physics object with low mass
