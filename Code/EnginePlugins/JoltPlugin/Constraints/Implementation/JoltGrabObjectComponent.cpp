#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/GrabbableItemComponent.h>
#include <GameEngine/Physics/ImpulseType.h>
#include <Jolt/Physics/Body/BodyLockMulti.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Character/JoltCharacterControllerComponent.h>
#include <JoltPlugin/Constraints/JoltGrabObjectComponent.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <Physics/Constraints/SixDOFConstraint.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltGrabObjectComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MaxGrabPointDistance", m_fMaxGrabPointDistance)->AddAttributes(new ezDefaultValueAttribute(2.0f)),
    EZ_MEMBER_PROPERTY("CastRadius", m_fCastRadius)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("SpringStiffness", m_fSpringStiffness)->AddAttributes(new ezDefaultValueAttribute(2.0f), new ezClampValueAttribute(1.0f, 60.0f)),
    EZ_MEMBER_PROPERTY("SpringDamping", m_fSpringDamping)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("BreakDistance", m_fBreakDistance)->AddAttributes(new ezDefaultValueAttribute(0.5f)),
    EZ_ACCESSOR_PROPERTY("AttachTo", DummyGetter, SetAttachToReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
    EZ_MEMBER_PROPERTY("GrabAnyObjectWithSize", m_fAllowGrabAnyObjectWithSize)->AddAttributes(new ezDefaultValueAttribute(0.75f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(GrabNearbyObject),
    EZ_SCRIPT_FUNCTION_PROPERTY(HasObjectGrabbed),
    EZ_SCRIPT_FUNCTION_PROPERTY(DropGrabbedObject, In, "uiImpulseType")->AddAttributes(new ezFunctionArgumentAttributes(0, new ezDefaultValueAttribute(0))),
    EZ_SCRIPT_FUNCTION_PROPERTY(ThrowGrabbedObject, In, "vDirection", In, "uiImpulseType")->AddAttributes(new ezFunctionArgumentAttributes(1, new ezDefaultValueAttribute(0))),
    EZ_SCRIPT_FUNCTION_PROPERTY(BreakObjectGrab),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgReleaseObjectGrab, OnMsgReleaseObjectGrab),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Jolt/Constraints"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezJoltGrabObjectComponent::ezJoltGrabObjectComponent() = default;
ezJoltGrabObjectComponent::~ezJoltGrabObjectComponent() = default;

void ezJoltGrabObjectComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_fBreakDistance;
  s << m_fSpringStiffness;
  s << m_fSpringDamping;
  s << m_fMaxGrabPointDistance;
  s << m_fCastRadius;
  s << m_uiCollisionLayer;
  s << m_fAllowGrabAnyObjectWithSize;

  inout_stream.WriteGameObjectHandle(m_hAttachTo);
}

void ezJoltGrabObjectComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_fBreakDistance;
  s >> m_fSpringStiffness;
  s >> m_fSpringDamping;
  s >> m_fMaxGrabPointDistance;
  if (uiVersion >= 2)
  {
    s >> m_fCastRadius;
  }
  s >> m_uiCollisionLayer;
  s >> m_fAllowGrabAnyObjectWithSize;

  m_hAttachTo = inout_stream.ReadGameObjectHandle();
}

bool ezJoltGrabObjectComponent::FindNearbyObject(ezGameObject*& out_pObject, ezTransform& out_localGrabPoint) const
{
  const ezPhysicsWorldModuleInterface* pPhysicsModule = GetWorld()->GetModuleReadOnly<ezPhysicsWorldModuleInterface>();

  if (pPhysicsModule == nullptr)
    return false;

  auto pOwner = GetOwner();

  ezPhysicsCastResult hit;
  ezPhysicsQueryParameters queryParam;
  queryParam.m_bIgnoreInitialOverlap = true;
  queryParam.m_uiCollisionLayer = m_uiCollisionLayer;
  queryParam.m_ShapeTypes = ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic;

  if (m_fCastRadius > 0.0f)
  {
    if (!pPhysicsModule->SweepTestSphere(hit, m_fCastRadius, pOwner->GetGlobalPosition(), pOwner->GetGlobalDirForwards().GetNormalized(), m_fMaxGrabPointDistance * 5.0f, queryParam))
      return false;
  }
  else
  {
    if (!pPhysicsModule->Raycast(hit, pOwner->GetGlobalPosition(), pOwner->GetGlobalDirForwards().GetNormalized(), m_fMaxGrabPointDistance * 5.0f, queryParam))
      return false;
  }

  const ezGameObject* pActorObj = nullptr;
  if (!GetWorld()->TryGetObject(hit.m_hActorObject, pActorObj))
    return false;

  // If we hit a non-kinematic dynamic actor try to find a grab point.
  // If we don't have an dynamic actor or it is kinematic still report the hit so it can be used for other interactions like buttons etc.
  const ezJoltDynamicActorComponent* pActorComp = nullptr;
  if (pActorObj->TryGetComponentOfBaseType(pActorComp) && !pActorComp->GetKinematic())
  {
    if (DetermineGrabPoint(pActorComp, out_localGrabPoint).Failed())
      return false;
  }
  else
  {
    if (hit.m_fDistance > m_fMaxGrabPointDistance)
      return false;

    out_localGrabPoint = ezTransform::MakeIdentity();
  }

  out_pObject = const_cast<ezGameObject*>(pActorObj);
  return true;
}

bool ezJoltGrabObjectComponent::GrabObject(ezGameObject* pObjectToGrab, const ezTransform& localGrabPoint)
{
  if (m_pConstraint != nullptr || pObjectToGrab == nullptr)
    return false;

  const ezTime curTime = GetWorld()->GetClock().GetAccumulatedTime();

  // a cooldown to grab something again after we stood on the held object
  if (m_LastValidTime > curTime)
    return false;

  ezJoltDynamicActorComponent* pAttachToActor = GetAttachToActor();
  if (pAttachToActor == nullptr)
  {
    ezLog::Error("Can't grab object, no target actor to attach it to is set.");
    return false;
  }

  ezJoltDynamicActorComponent* pActorToGrab = nullptr;
  if (!pObjectToGrab->TryGetComponentOfBaseType(pActorToGrab))
    return false;

  if (pActorToGrab->GetKinematic())
    return false;

  if (IsCharacterStandingOnObject(pObjectToGrab->GetHandle()))
    return false;

  ezJoltCharacterControllerComponent* pController;
  if (GetWorld()->TryGetComponent(m_hCharacterControllerComponent, pController))
  {
    pController->SetObjectToIgnore(pActorToGrab->GetObjectFilterID());
  }

  m_ChildAnchorLocal = localGrabPoint;
  m_hGrabbedActor = pActorToGrab->GetHandle();

  CreateJoint(pAttachToActor, pActorToGrab);

  ezMsgObjectGrabbed msg;
  msg.m_bGotGrabbed = true;
  msg.m_hGrabbedBy = GetOwner()->GetHandle();
  pActorToGrab->GetOwner()->SendMessage(msg);

  m_LastValidTime = curTime;

  return true;
}

bool ezJoltGrabObjectComponent::GrabNearbyObject()
{
  ezGameObject* pActorToGrab = nullptr;
  ezTransform localGrabPoint;
  if (!FindNearbyObject(pActorToGrab, localGrabPoint))
    return false;

  return GrabObject(pActorToGrab, localGrabPoint);
}

bool ezJoltGrabObjectComponent::HasObjectGrabbed() const
{
  return m_pConstraint != nullptr;
}

void ezJoltGrabObjectComponent::DropGrabbedObject(ezUInt8 uiImpulseType)
{
  if (uiImpulseType >= ezImpulseTypeConfig::FirstValidKey)
  {
    ezJoltDynamicActorComponent* pGrabbedActor = nullptr;
    if (GetWorld()->TryGetComponent(m_hGrabbedActor, pGrabbedActor))
    {
      const float fImpulse = ezJoltCore::GetImpulseTypeConfig().GetImpulseForWeight(uiImpulseType, pGrabbedActor->m_uiWeightCategory);

      ReleaseGrabbedObject(fImpulse);
      return;
    }
  }

  ReleaseGrabbedObject(0.0f);
}

void ezJoltGrabObjectComponent::ThrowGrabbedObject(const ezVec3& vRelativeDir, ezUInt8 uiImpulseType)
{
  ezComponentHandle hActor = m_hGrabbedActor;
  ReleaseGrabbedObject(0.0f);

  ezJoltDynamicActorComponent* pActor;
  if (GetWorld()->TryGetComponent(hActor, pActor))
  {
    pActor->AddLinearImpulse(GetOwner()->GetGlobalRotation() * vRelativeDir, uiImpulseType);
  }
}

void ezJoltGrabObjectComponent::BreakObjectGrab()
{
  ReleaseGrabbedObject(0.0f);

  ezMsgPhysicsJointBroke msg;
  msg.m_hJointObject = GetOwner()->GetHandle();

  GetOwner()->PostEventMessage(msg, this, ezTime::MakeZero());
}

void ezJoltGrabObjectComponent::SetAttachToReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hAttachTo = resolver(szReference, GetHandle(), "AttachTo");
}

void ezJoltGrabObjectComponent::ReleaseGrabbedObject(float fMaxAllowedImpulse)
{
  if (m_pConstraint == nullptr)
    return;

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();

  ezJoltDynamicActorComponent* pGrabbedActor = nullptr;
  if (GetWorld()->TryGetComponent(m_hGrabbedActor, pGrabbedActor))
  {
    // preserve the owner velocity
    const JPH::Vec3 vParentVelocity = ezJoltConversionUtils::ToVec3(GetOwner()->GetLinearVelocity());

    JPH::BodyLockWrite bodyLock(pModule->GetJoltSystem()->GetBodyLockInterface(), JPH::BodyID(pGrabbedActor->GetJoltBodyID()));
    if (bodyLock.Succeeded())
    {
      auto& motion = *bodyLock.GetBody().GetMotionProperties();

      motion.SetInverseMass(m_fGrabbedActorInverseMass);
      // TODO: this needs to be set as well : motion.SetInverseInertia(m_fGrabbedActorMass);
      motion.SetGravityFactor(m_fGrabbedActorGravity);

      // clamp linear velocities according to maximum impulse
      const JPH::Vec3 vLinear = motion.GetLinearVelocity() - vParentVelocity;
      const JPH::Vec3 vAngular = bodyLock.GetBody().GetMotionProperties()->GetAngularVelocity();

      // divide impulse by mass to get maximal velocity
      const float fMaxVelocity = ezMath::Abs(fMaxAllowedImpulse * m_fGrabbedActorInverseMass);
      const float fSpeed = vLinear.Length();

      // clamp linear velocity
      if (fSpeed > fMaxVelocity)
      {
        const float change = fMaxVelocity / fSpeed;

        motion.SetLinearVelocity(vParentVelocity + vLinear * change);
        motion.SetAngularVelocity(vAngular * change);
      }

      if (pModule->GetJoltSystem()->GetBodyInterfaceNoLock().IsAdded(JPH::BodyID(pGrabbedActor->GetJoltBodyID())))
      {
        pModule->GetJoltSystem()->GetBodyInterfaceNoLock().ActivateBody(JPH::BodyID(pGrabbedActor->GetJoltBodyID()));
      }
    }

    ezMsgObjectGrabbed msg;
    msg.m_bGotGrabbed = false;
    msg.m_hGrabbedBy = GetOwner()->GetHandle();
    pGrabbedActor->GetOwner()->SendMessage(msg);
  }

  ezJoltCharacterControllerComponent* pController;
  if (GetWorld()->TryGetComponent(m_hCharacterControllerComponent, pController))
  {
    pController->ClearObjectToIgnore();
  }

  pModule->GetJoltSystem()->RemoveConstraint(m_pConstraint);

  m_pConstraint->Release();
  m_pConstraint = nullptr;

  m_hGrabbedActor.Invalidate();
}

ezJoltDynamicActorComponent* ezJoltGrabObjectComponent::GetAttachToActor()
{
  ezJoltDynamicActorComponent* pActor = nullptr;
  ezGameObject* pObject = nullptr;

  if (!GetWorld()->TryGetObject(m_hAttachTo, pObject))
    return nullptr;

  if (!pObject->TryGetComponentOfBaseType(pActor))
    return nullptr;

  if (!pActor->GetKinematic())
    return nullptr;

  return pActor;
}

ezResult ezJoltGrabObjectComponent::DetermineGrabPoint(const ezComponent* pActorComp, ezTransform& out_LocalGrabPoint) const
{
  out_LocalGrabPoint.SetIdentity();

  const ezGameObject* pAttachToObject = nullptr;
  if (!GetWorld()->TryGetObject(m_hAttachTo, pAttachToObject))
    return EZ_FAILURE;

  const auto vAttachToPos = pAttachToObject->GetGlobalPosition();
  const auto vOwnerDir = GetOwner()->GetGlobalDirForwards();
  const auto vOwnerUp = GetOwner()->GetGlobalDirUp();
  const auto pActorObj = pActorComp->GetOwner();

  const ezTransform& actorTransform = pActorObj->GetGlobalTransform();
  ezHybridArray<ezGrabbableItemGrabPoint, 16> grabPoints;

  const ezGrabbableItemComponent* pGrabbableItemComp = nullptr;
  if (pActorObj->TryGetComponentOfBaseType(pGrabbableItemComp) && !pGrabbableItemComp->m_GrabPoints.IsEmpty())
  {
    grabPoints = pGrabbableItemComp->m_GrabPoints;
  }
  else
  {
    ezBoundingBoxSphere bounds = pActorComp->GetOwner()->GetLocalBounds();

    if (!bounds.IsValid())
    {
      bounds = ezBoundingSphere::MakeFromCenterAndRadius(ezVec3::MakeZero(), 0.1f);
    }

    const auto& box = bounds.GetBox();
    const ezVec3 ext = box.GetExtents().CompMul(pActorComp->GetOwner()->GetGlobalScaling());

    if (ext.x <= m_fAllowGrabAnyObjectWithSize && ext.y <= m_fAllowGrabAnyObjectWithSize && ext.z <= m_fAllowGrabAnyObjectWithSize)
    {
      const ezVec3 halfExt = box.GetHalfExtents().CompMul(pActorComp->GetOwner()->GetGlobalScaling()) * 0.5f;
      const ezVec3& center = box.GetCenter();

      grabPoints.SetCount(4);
      grabPoints[0].m_vLocalPosition.Set(-halfExt.x, 0, 0);
      grabPoints[0].m_qLocalRotation = ezQuat::MakeShortestRotation(ezVec3::MakeAxisX(), ezVec3::MakeAxisX());
      grabPoints[1].m_vLocalPosition.Set(+halfExt.x, 0, 0);
      grabPoints[1].m_qLocalRotation = ezQuat::MakeShortestRotation(ezVec3::MakeAxisX(), -ezVec3::MakeAxisX());
      grabPoints[2].m_vLocalPosition.Set(0, -halfExt.y, 0);
      grabPoints[2].m_qLocalRotation = ezQuat::MakeShortestRotation(ezVec3::MakeAxisX(), ezVec3::MakeAxisY());
      grabPoints[3].m_vLocalPosition.Set(0, +halfExt.y, 0);
      grabPoints[3].m_qLocalRotation = ezQuat::MakeShortestRotation(ezVec3::MakeAxisX(), -ezVec3::MakeAxisY());
      // grabPoints[4].m_vLocalPosition.Set(0, 0, -halfExt.z);
      // grabPoints[4].m_qLocalRotation = ezQuat::MakeShortestRotation(ezVec3::MakeAxisX(), ezVec3::MakeAxisZ());
      // grabPoints[5].m_vLocalPosition.Set(0, 0, +halfExt.z);
      // grabPoints[5].m_qLocalRotation = ezQuat::MakeShortestRotation(ezVec3::MakeAxisX(), -ezVec3::MakeAxisZ());

      for (ezUInt32 i = 0; i < grabPoints.GetCount(); ++i)
      {
        grabPoints[i].m_vLocalPosition += center;
      }
    }
  }

  if (grabPoints.IsEmpty())
    return EZ_FAILURE;

  const float fMaxDistSqr = ezMath::Square(m_fMaxGrabPointDistance);

  ezUInt32 uiBestPointIndex = ezInvalidIndex;
  float fBestScore = -1000.0f;

  for (ezUInt32 i = 0; i < grabPoints.GetCount(); ++i)
  {
    const ezVec3 vGrabPointPos = actorTransform.TransformPosition(grabPoints[i].m_vLocalPosition);
    const ezQuat qGrabPointRot = actorTransform.m_qRotation * grabPoints[i].m_qLocalRotation;
    const ezVec3 vGrabPointDir = qGrabPointRot * ezVec3(1, 0, 0);
    const ezVec3 vGrabPointUp = qGrabPointRot * ezVec3(0, 0, 1);

    const float fLenSqr = (vGrabPointPos - vAttachToPos).GetLengthSquared();

    if (fLenSqr >= fMaxDistSqr)
      continue;

    float fScore = 1.0f - fLenSqr;
    fScore += vGrabPointDir.Dot(vOwnerDir);
    fScore += vGrabPointUp.Dot(vOwnerUp) * 0.5f; // up has less weight than forward

    if (fScore > fBestScore)
    {
      uiBestPointIndex = i;
      fBestScore = fScore;
    }
  }

  if (uiBestPointIndex >= grabPoints.GetCount())
    return EZ_FAILURE;

  out_LocalGrabPoint.m_vPosition = grabPoints[uiBestPointIndex].m_vLocalPosition;
  out_LocalGrabPoint.m_qRotation = grabPoints[uiBestPointIndex].m_qLocalRotation;

  return EZ_SUCCESS;
}

void ezJoltGrabObjectComponent::CreateJoint(ezJoltDynamicActorComponent* pParent, ezJoltDynamicActorComponent* pChild)
{
  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();

  JPH::BodyID bodies[2] = {JPH::BodyID(pParent->GetJoltBodyID()), JPH::BodyID(pChild->GetJoltBodyID())};
  JPH::BodyLockMultiWrite bodyLock(pModule->GetJoltSystem()->GetBodyLockInterface(), bodies, 2);

  auto pBody0 = bodyLock.GetBody(0);
  auto pBody1 = bodyLock.GetBody(1);

  m_fGrabbedActorInverseMass = pBody1->GetMotionProperties()->GetInverseMass();
  m_fGrabbedActorGravity = pBody1->GetMotionProperties()->GetGravityFactor();

  pBody1->GetMotionProperties()->SetInverseMass(10.0f);
  pBody1->GetMotionProperties()->SetGravityFactor(0.0f);

  JPH::SixDOFConstraintSettings opt;

  {
    const auto diff0 = pBody0->GetPosition() - pBody0->GetCenterOfMassPosition();
    const auto diff1 = pBody1->GetPosition() - pBody1->GetCenterOfMassPosition();

    const JPH::Quat childRot = ezJoltConversionUtils::ToQuat(m_ChildAnchorLocal.m_qRotation);

    opt.mDrawConstraintSize = 0.1f;
    opt.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
    opt.mPosition1 = diff0;
    opt.mPosition2 = diff1 + ezJoltConversionUtils::ToVec3(m_ChildAnchorLocal.m_vPosition);
    opt.mAxisX1 = JPH::Vec3::sAxisX();
    opt.mAxisY1 = JPH::Vec3::sAxisY();
    opt.mAxisX2 = childRot * JPH::Vec3::sAxisX();
    opt.mAxisY2 = childRot * JPH::Vec3::sAxisY();
    opt.MakeFreeAxis(JPH::SixDOFConstraintSettings::EAxis::TranslationX);
    opt.MakeFreeAxis(JPH::SixDOFConstraintSettings::EAxis::TranslationY);
    opt.MakeFreeAxis(JPH::SixDOFConstraintSettings::EAxis::TranslationZ);
    opt.MakeFreeAxis(JPH::SixDOFConstraintSettings::EAxis::RotationX);
    opt.MakeFreeAxis(JPH::SixDOFConstraintSettings::EAxis::RotationY);
    opt.MakeFreeAxis(JPH::SixDOFConstraintSettings::EAxis::RotationZ);

    for (int i = 0; i < 6; ++i)
    {
      opt.mMotorSettings[JPH::SixDOFConstraintSettings::EAxis::TranslationX + i].mSpringSettings.mMode = JPH::ESpringMode::FrequencyAndDamping;
      opt.mMotorSettings[JPH::SixDOFConstraintSettings::EAxis::TranslationX + i].mSpringSettings.mDamping = m_fSpringDamping;
      opt.mMotorSettings[JPH::SixDOFConstraintSettings::EAxis::TranslationX + i].mSpringSettings.mFrequency = m_fSpringStiffness;
    }
  }

  // ezTransform tAnchor = m_ChildAnchorLocal;
  // tAnchor.m_vPosition = tAnchor.m_vPosition.CompMul(pChild->GetOwner()->GetGlobalScaling());
  // pJoint->SetActors(pParent->GetOwner()->GetHandle(), ezTransform::MakeIdentity(), pChild->GetOwner()->GetHandle(), tAnchor);

  m_pConstraint = static_cast<JPH::SixDOFConstraint*>(opt.Create(*bodyLock.GetBody(0), *bodyLock.GetBody(1)));
  m_pConstraint->AddRef();

  for (int i = 0; i < 6; ++i)
  {
    m_pConstraint->SetMotorState((JPH::SixDOFConstraint::EAxis)(JPH::SixDOFConstraint::EAxis::TranslationX + i), JPH::EMotorState::Position);
  }

  pModule->GetJoltSystem()->AddConstraint(m_pConstraint);
  pModule->GetJoltSystem()->GetBodyInterfaceNoLock().ActivateBodies(bodies, 2);
}

void ezJoltGrabObjectComponent::DetectDistanceViolation(ezJoltDynamicActorComponent* pGrabbedActor)
{
  if (m_fBreakDistance <= 0)
    return;

  ezGameObject* pJointObject = nullptr;
  if (!GetWorld()->TryGetObject(m_hAttachTo, pJointObject))
    return;

  const ezVec3 vAnchorPos = pGrabbedActor->GetOwner()->GetGlobalTransform().TransformPosition(m_ChildAnchorLocal.m_vPosition);
  const ezVec3 vJointPos = pJointObject->GetGlobalPosition();
  const float fDistance = (vAnchorPos - vJointPos).GetLength();

  if (fDistance < m_fBreakDistance)
  {
    m_LastValidTime = GetWorld()->GetClock().GetAccumulatedTime();
  }
  else if (fDistance > m_fMaxGrabPointDistance * 1.1f)
  {
    BreakObjectGrab();
    return;
  }
  else
  {
    // TODO: make this configurable?
    if (GetWorld()->GetClock().GetAccumulatedTime() - m_LastValidTime > ezTime::MakeFromSeconds(1.0))
    {
      BreakObjectGrab();
      return;
    }
  }
}

bool ezJoltGrabObjectComponent::IsCharacterStandingOnObject(ezGameObjectHandle hActorToGrab) const
{
  const ezJoltCharacterControllerComponent* pController;
  if (GetWorld()->TryGetComponent(m_hCharacterControllerComponent, pController))
  {
    // TODO
    // if (pController->GetStandingOnActor() == hActorToGrab)
    //{
    //  return true;
    //}
  }

  return false;
}

void ezJoltGrabObjectComponent::OnMsgReleaseObjectGrab(ezMsgReleaseObjectGrab& msg)
{
  if (!msg.m_hGrabbedObjectToRelease.IsInvalidated() && !m_hGrabbedActor.IsInvalidated())
  {
    ezComponent* pComponent;
    if (GetOwner()->GetWorld()->TryGetComponent(m_hGrabbedActor, pComponent))
    {
      if (pComponent->GetOwner()->GetHandle() == msg.m_hGrabbedObjectToRelease)
      {
        DropGrabbedObject();
      }
    }
  }
}

void ezJoltGrabObjectComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  ezGameObject* pObj = GetOwner();

  while (pObj)
  {
    ezJoltCharacterControllerComponent* pController;
    if (pObj->TryGetComponentOfBaseType(pController))
    {
      m_hCharacterControllerComponent = pController->GetHandle();
    }

    pObj = pObj->GetParent();
  }
}

void ezJoltGrabObjectComponent::OnDeactivated()
{
  ReleaseGrabbedObject(0.0f);

  SUPER::OnDeactivated();
}

void ezJoltGrabObjectComponent::Update()
{
  if (m_pConstraint == nullptr)
    return;

  ezJoltDynamicActorComponent* pGrabbedActor;
  if (!GetWorld()->TryGetComponent(m_hGrabbedActor, pGrabbedActor))
  {
    BreakObjectGrab();
    return;
  }

  DetectDistanceViolation(pGrabbedActor);

  if (IsCharacterStandingOnObject(pGrabbedActor->GetOwner()->GetHandle()))
  {
    // disallow grabbing something again until that time
    // to prevent grabbing an object in air that we just jumped off of
    m_LastValidTime = GetWorld()->GetClock().GetAccumulatedTime() + ezTime::MakeFromMilliseconds(400);
    BreakObjectGrab();
  }
}


EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_JoltGrabObjectComponent);
