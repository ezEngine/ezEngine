#include <PhysXPluginPCH.h>

#include <Components/PxDynamicActorComponent.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Components/PxRopeComponent.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <Utilities/PxConversionUtils.h>
#include <Utilities/PxUserData.h>
#include <WorldModule/Implementation/PhysX.h>
#include <WorldModule/PhysXWorldModule.h>
#include <extensions/PxD6Joint.h>
#include <foundation/Px.h>

using namespace physx;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxRopeComponent, 1, ezComponentMode::Dynamic)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_ACCESSOR_PROPERTY("AnchorA", DummyGetter, SetAnchorAReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
      EZ_ACCESSOR_PROPERTY("AnchorB", DummyGetter, SetAnchorBReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
      EZ_MEMBER_PROPERTY("AttachToA", m_bAttachToA)->AddAttributes(new ezDefaultValueAttribute(true)),
      EZ_MEMBER_PROPERTY("AttachToB", m_bAttachToB)->AddAttributes(new ezDefaultValueAttribute(true)),
      EZ_MEMBER_PROPERTY("Mass", m_fTotalMass)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.1f, 1000.0f)),
      EZ_MEMBER_PROPERTY("Pieces", m_uiPieces)->AddAttributes(new ezDefaultValueAttribute(16), new ezClampValueAttribute(2, 64)),
      EZ_MEMBER_PROPERTY("Thickness", m_fThickness)->AddAttributes(new ezDefaultValueAttribute(0.05f), new ezClampValueAttribute(0.01f, 0.5f)),
      EZ_MEMBER_PROPERTY("BendStiffness", m_fBendStiffness)->AddAttributes(new ezClampValueAttribute(0.0f,   ezVariant())),
      EZ_MEMBER_PROPERTY("TwistStiffness", m_fTwistStiffness)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
      EZ_MEMBER_PROPERTY("BendDamping", m_fBendDamping)->AddAttributes(new ezDefaultValueAttribute(300.0f), new ezClampValueAttribute(0.0f,   ezVariant())),
      EZ_MEMBER_PROPERTY("TwistDamping", m_fTwistDamping)->AddAttributes(new ezDefaultValueAttribute(1000.0f), new ezClampValueAttribute(0.0f,   ezVariant())),
      EZ_MEMBER_PROPERTY("MaxBend", m_MaxBend)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(30)), new ezClampValueAttribute(ezAngle::Degree(5), ezAngle::Degree(90))),
      EZ_MEMBER_PROPERTY("MaxTwist", m_MaxTwist)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(15)), new ezClampValueAttribute(ezAngle::Degree(0.01f), ezAngle::Degree(90))),
      EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
      EZ_ACCESSOR_PROPERTY("Surface", GetSurfaceFile, SetSurfaceFile)->AddAttributes(new ezAssetBrowserAttribute("Surface")),
      EZ_ACCESSOR_PROPERTY("DisableGravity", GetDisableGravity, SetDisableGravity),
      //EZ_MEMBER_PROPERTY("SelfCollision", m_bSelfCollision), // works, but not exposed for now
    }
    EZ_END_PROPERTIES;
    EZ_BEGIN_MESSAGEHANDLERS
    {
      EZ_MESSAGE_HANDLER(ezMsgPhysicsAddForce, AddForceAtPos),
      EZ_MESSAGE_HANDLER(ezMsgPhysicsAddImpulse, AddImpulseAtPos),
    }
    EZ_END_MESSAGEHANDLERS;
    EZ_BEGIN_ATTRIBUTES
    {
      new ezCategoryAttribute("Effects"),
    }
    EZ_END_ATTRIBUTES;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPxRopeComponent::ezPxRopeComponent() = default;
ezPxRopeComponent::~ezPxRopeComponent() = default;

void ezPxRopeComponent::SetSurfaceFile(const char* szFile)
{
  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hSurface = ezResourceManager::LoadResource<ezSurfaceResource>(szFile);
  }

  if (m_hSurface.IsValid())
    ezResourceManager::PreloadResource(m_hSurface);
}

const char* ezPxRopeComponent::GetSurfaceFile() const
{
  if (!m_hSurface.IsValid())
    return "";

  return m_hSurface.GetResourceID();
}

void ezPxRopeComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_uiCollisionLayer;
  s << m_uiPieces;
  s << m_fThickness;
  s << m_bAttachToA;
  s << m_bAttachToB;
  s << m_bSelfCollision;
  s << m_bDisableGravity;
  s << m_hSurface;
  s << m_MaxBend;
  s << m_MaxTwist;
  s << m_fBendStiffness;
  s << m_fTwistStiffness;
  s << m_fTotalMass;
  s << m_fBendDamping;
  s << m_fTwistDamping;

  stream.WriteGameObjectHandle(m_hAnchorA);
  stream.WriteGameObjectHandle(m_hAnchorB);
}

void ezPxRopeComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_uiCollisionLayer;
  s >> m_uiPieces;
  s >> m_fThickness;
  s >> m_bAttachToA;
  s >> m_bAttachToB;
  s >> m_bSelfCollision;
  s >> m_bDisableGravity;
  s >> m_hSurface;
  s >> m_MaxBend;
  s >> m_MaxTwist;
  s >> m_fBendStiffness;
  s >> m_fTwistStiffness;
  s >> m_fTotalMass;
  s >> m_fBendDamping;
  s >> m_fTwistDamping;

  m_hAnchorA = stream.ReadGameObjectHandle();
  m_hAnchorB = stream.ReadGameObjectHandle();
}

void ezPxRopeComponent::CreateFilterData(PxFilterData& filter)
{
  filter = ezPhysX::CreateFilterData(m_uiCollisionLayer, m_uiShapeID);
}

void ezPxRopeComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  CreateRope();
}

void ezPxRopeComponent::OnActivated()
{
  SendPreviewPose();
}

void ezPxRopeComponent::OnDeactivated()
{
  DestroyPhysicsShapes();

  // tell the render components, that the rope is gone
  ezMsgRopePoseUpdated poseMsg;
  GetOwner()->SendMessage(poseMsg);

  SUPER::OnDeactivated();
}

PxMaterial* ezPxRopeComponent::GetPxMaterial()
{
  if (m_hSurface.IsValid())
  {
    ezResourceLock<ezSurfaceResource> pSurface(m_hSurface, ezResourceAcquireMode::BlockTillLoaded);

    if (pSurface->m_pPhysicsMaterial != nullptr)
    {
      return static_cast<PxMaterial*>(pSurface->m_pPhysicsMaterial);
    }
  }

  return ezPhysX::GetSingleton()->GetDefaultMaterial();
}

ezVec3 ezPxRopeComponent::GetAnchorPosition(const ezGameObjectHandle& hTarget) const
{
  const ezGameObject* pObj;
  if (GetWorld()->TryGetObject(hTarget, pObj))
  {
    ezTransform t = GetOwner()->GetGlobalTransform();
    t.Invert();
    return t.TransformPosition(pObj->GetGlobalPosition());
  }

  return ezVec3::ZeroVector();
}

void ezPxRopeComponent::CreateRope()
{
  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
  m_uiShapeID = pModule->CreateShapeId();

  const PxMaterial* pPxMaterial = GetPxMaterial();
  PxFilterData filter;
  CreateFilterData(filter);

  PxFilterData filterNone = filter;
  filterNone.word1 = 0; // disable collisions with everything else

  ezPxUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  m_pArticulation = ezPhysX::GetSingleton()->GetPhysXAPI()->createArticulation();
  m_pArticulation->userData = pUserData;
  m_pArticulation->setSleepThreshold(0.8f);

  //m_pArticulation->setSolverIterationCounts(16, 4);
  //m_pArticulation->setMaxProjectionIterations(8);

  const ezTransform tRoot = GetOwner()->GetGlobalTransform();

  ezHybridArray<ezTransform, 65> pieces;
  float fPieceLength;
  CreateSegmentTransforms(tRoot, pieces, fPieceLength);
  pieces.PopBack(); // don't need the last transform

  m_ArticulationLinks.SetCountUninitialized(pieces.GetCount());

  const float fMass = m_fTotalMass / pieces.GetCount();

  for (ezUInt32 idx = 0; idx < pieces.GetCount(); ++idx)
  {
    PxArticulationLink* pPrevLink = idx == 0 ? nullptr : m_ArticulationLinks[idx - 1];

    auto pLink = m_pArticulation->createLink(pPrevLink, ezPxConversionUtils::ToTransform(pieces[idx]));
    EZ_ASSERT_DEV(pLink != nullptr, "Rope shape creation failed. Too many bones? (max 64)");

    m_ArticulationLinks[idx] = pLink;

    pLink->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, m_bDisableGravity);
    pLink->userData = pUserData;

    if (pPrevLink != nullptr)
    {
      ezTransform parentFrameJoint;
      parentFrameJoint.SetLocalTransform(pieces[idx - 1], pieces[idx]);
      parentFrameJoint.m_qRotation.SetIdentity();

      PxArticulationJoint* inb = reinterpret_cast<PxArticulationJoint*>(pLink->getInboundJoint());

      inb->setParentPose(ezPxConversionUtils::ToTransform(parentFrameJoint));
      inb->setSwingLimitEnabled(true);
      inb->setTwistLimitEnabled(true);
      inb->setSwingLimit(m_MaxBend.GetRadian(), m_MaxBend.GetRadian());
      inb->setTwistLimit(-m_MaxTwist.GetRadian(), m_MaxTwist.GetRadian());

      inb->setStiffness(m_fBendStiffness);            // makes the rope try to get straight
      inb->setTangentialStiffness(m_fTwistStiffness); // makes the rope try to not have any twist
      inb->setDamping(m_fBendDamping);                // prevents changes to the rope bending
      inb->setTangentialDamping(m_fTwistDamping);     // probably prevents changes to the rope twist
    }

    PxCapsuleGeometry shape(m_fThickness * 0.5f, fPieceLength * 0.5f);
    PxShape* pShape = PxRigidActorExt::createExclusiveShape(*pLink, shape, *pPxMaterial);

    ezVec3 vOffsetPos(fPieceLength * 0.5f, 0, 0);

    pShape->setLocalPose(ezPxConversionUtils::ToTransform(ezTransform(vOffsetPos)));
    pShape->setSimulationFilterData(filter);
    pShape->setQueryFilterData(filter);
    pShape->userData = pUserData;

    if ((m_bAttachToA && idx == 0) || (m_bAttachToB && idx + 1 == pieces.GetCount()))
    {
      // disable all collisions for the first and last rope segment
      // this prevents colliding with walls that the rope is attached to
      pShape->setSimulationFilterData(filterNone);
    }


    PxRigidBodyExt::setMassAndUpdateInertia(*pLink, fMass);
  }

  EZ_PX_WRITE_LOCK(*pModule->GetPxScene());

  m_pAggregate = ezPhysX::GetSingleton()->GetPhysXAPI()->createAggregate(m_uiPieces, m_bSelfCollision);
  m_pAggregate->addArticulation(*m_pArticulation);
  pModule->GetPxScene()->addAggregate(*m_pAggregate);

  if (m_bAttachToA)
  {
    m_pJointA = CreateJoint(m_hAnchorA, pieces[0], m_ArticulationLinks[0], ezTransform::IdentityTransform());
  }

  if (m_bAttachToB)
  {
    ezTransform localTransform;
    localTransform.SetIdentity();
    localTransform.m_vPosition.x = fPieceLength;

    ezTransform lastPiece;
    lastPiece.SetGlobalTransform(pieces.PeekBack(), localTransform);

    m_pJointB = CreateJoint(m_hAnchorB, lastPiece, m_ArticulationLinks.PeekBack(), localTransform);
  }
}

PxJoint* ezPxRopeComponent::CreateJoint(const ezGameObjectHandle& hTarget, const ezTransform& location, PxRigidBody* pLink, const ezTransform& linkOffset)
{
  ezPxDynamicActorComponent* pActor = nullptr;
  PxRigidActor* pPxActor = nullptr;
  ezTransform parentLocal;

  ezGameObject* pObject;
  if (GetWorld()->TryGetObject(hTarget, pObject))
  {
    if (!pObject->TryGetComponentOfBaseType(pActor))
    {
      pObject = pObject->GetParent();

      if (pObject)
      {
        pObject->TryGetComponentOfBaseType(pActor);
      }
    }
  }

  if (pActor)
  {
    pActor->EnsureSimulationStarted();
    pPxActor = pActor->GetPxActor();

    ezTransform tParent = pObject->GetGlobalTransform();
    tParent.m_vScale.Set(1.0f);

    parentLocal.SetLocalTransform(tParent, location);
  }
  else
  {
    parentLocal = location;
  }

  auto pPxApi = ezPhysX::GetSingleton()->GetPhysXAPI();

  PxD6Joint* pJoint = PxD6JointCreate(*pPxApi, pPxActor, ezPxConversionUtils::ToTransform(parentLocal), pLink, ezPxConversionUtils::ToTransform(linkOffset));

  pJoint->setMotion(PxD6Axis::eX, PxD6Motion::eLOCKED);
  pJoint->setMotion(PxD6Axis::eY, PxD6Motion::eLOCKED);
  pJoint->setMotion(PxD6Axis::eY, PxD6Motion::eLOCKED);
  pJoint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLIMITED);
  pJoint->setTwistLimit(PxJointAngularLimitPair(-m_MaxTwist.GetRadian(), m_MaxTwist.GetRadian(), PxSpring(m_fTwistStiffness, m_fTwistDamping)));
  pJoint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLIMITED);
  pJoint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLIMITED);
  pJoint->setSwingLimit(PxJointLimitCone(m_MaxBend.GetRadian(), m_MaxBend.GetRadian(), PxSpring(m_fBendStiffness, m_fBendDamping)));

  return pJoint;
}

void ezPxRopeComponent::UpdatePreview()
{
  ezVec3 vNewPreviewRefPos = GetOwner()->GetGlobalPosition();

  ezGameObject* pObj;
  if (GetWorld()->TryGetObject(m_hAnchorA, pObj))
    vNewPreviewRefPos += pObj->GetGlobalPosition();
  if (GetWorld()->TryGetObject(m_hAnchorB, pObj))
    vNewPreviewRefPos += pObj->GetGlobalPosition();

  if (vNewPreviewRefPos != m_vPreviewRefPos)
  {
    m_vPreviewRefPos = vNewPreviewRefPos;
    SendPreviewPose();
  }
}

void ezPxRopeComponent::CreateSegmentTransforms(const ezTransform& rootTransform, ezDynamicArray<ezTransform>& transforms, float& out_fPieceLength) const
{
  // check out https://www.britannica.com/science/catenary for a natural rope hanging formula

  out_fPieceLength = 0.0f;

  if (m_hAnchorA.IsInvalidated() && m_hAnchorB.IsInvalidated())
    return;

  const ezVec3 vAnchorA = GetAnchorPosition(m_hAnchorA);
  const ezVec3 vAnchorB = GetAnchorPosition(m_hAnchorB);

  const float fLength = (vAnchorB - vAnchorA).GetLength();
  if (ezMath::IsZero(fLength, 0.001f))
    return;

  ezVec3 centerPos = ezMath::Lerp(vAnchorA, vAnchorB, 0.5f);

  if (!m_hAnchorA.IsInvalidated() && !m_hAnchorB.IsInvalidated())
  {
    ezVec3 vDownDir = -centerPos;
    vDownDir.MakeOrthogonalTo((vAnchorB - vAnchorA).GetNormalized());
    vDownDir.NormalizeIfNotZero(ezVec3(0, 0, -1)).IgnoreResult();

    centerPos += vDownDir * centerPos.GetLength();
  }

  float fLengthWithSlack = (centerPos - vAnchorA).GetLength() * 2;
  m_fRopeLength = fLengthWithSlack;

  const ezUInt32 uiPieces = ezMath::RoundUp(m_uiPieces, 2);
  transforms.SetCountUninitialized(uiPieces + 1);
  out_fPieceLength = fLengthWithSlack / uiPieces;

  ezQuat qRotDown, qRotUp;
  qRotDown.SetShortestRotation(ezVec3::UnitXAxis(), (centerPos - vAnchorA).GetNormalized());
  qRotUp.SetShortestRotation(ezVec3::UnitXAxis(), (vAnchorB - centerPos).GetNormalized());

  ezVec3 vNextPos = vAnchorA;
  ezQuat qRot = qRotDown;


  for (ezUInt32 idx = 0; idx < uiPieces; ++idx)
  {
    if (idx * 2 == uiPieces)
    {
      qRot = qRotUp;
    }

    ezTransform localTransform;
    localTransform.SetIdentity();
    localTransform.m_qRotation = qRot;
    localTransform.m_vPosition = vNextPos;
    vNextPos += qRot * ezVec3(out_fPieceLength, 0, 0);

    transforms[idx].SetGlobalTransform(rootTransform, localTransform);
  }

  {
    ezTransform localTransform;
    localTransform.SetIdentity();
    localTransform.m_qRotation = qRot;
    localTransform.m_vPosition = vNextPos;

    transforms.PeekBack().SetGlobalTransform(rootTransform, localTransform);
  }
}

void ezPxRopeComponent::DestroyPhysicsShapes()
{
  if (m_pArticulation)
  {
    ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
    EZ_PX_WRITE_LOCK(*pModule->GetPxScene());

    if (m_pJointA)
    {
      m_pJointA->release();
      m_pJointA = nullptr;
    }

    if (m_pJointB)
    {
      m_pJointB->release();
      m_pJointB = nullptr;
    }

    m_pArticulation->release();
    m_pArticulation = nullptr;

    m_pAggregate->release();
    m_pAggregate = nullptr;

    pModule->DeallocateUserData(m_uiUserDataIndex);
    m_uiUserDataIndex = ezInvalidIndex;
  }
}

// this is a hard-coded constant that seems to prevent the most severe issues well enough
//
// applying forces to an articulation is a problem,
// because articulations tend to end up with bad (NaN)
// data real quickly if those forces are too large
// linear and angular damping already helps a lot, but it's not sufficient
// the only reliable solution seems to be to prevent too large incoming forces
// since ropes have many links, things like explosions tend to apply the same force to each link
// thus multiplying the effect
// therefore a 'frame budget' is used to only apply a certain amount of force during a single frame
// this effectively ignores most forces that are applied to multiple links and just moves one or two links
constexpr float g_fMaxForce = 1.5f;

void ezPxRopeComponent::Update()
{
  m_fMaxForcePerFrame = g_fMaxForce * 2.0f;

  if (m_ArticulationLinks.IsEmpty())
    return;

  if (m_pArticulation->isSleeping())
    return;

  ezHybridArray<ezTransform, 32> poses(ezFrameAllocator::GetCurrentAllocator());
  poses.SetCountUninitialized(m_ArticulationLinks.GetCount() + 1);

  ezMsgRopePoseUpdated poseMsg;
  poseMsg.m_LinkTransforms = poses;

  // we can assume that the link in the middle of the array is also more or less in the middle of the rope
  const ezUInt32 uiCenterLink = m_ArticulationLinks.GetCount() / 2;

  ezTransform rootTransform = GetOwner()->GetGlobalTransform();
  rootTransform.m_vPosition = ezPxConversionUtils::ToVec3(m_ArticulationLinks[uiCenterLink]->getGlobalPose().p);

  // apparently this can happen when strong forces are applied to the actor
  if (!rootTransform.m_vPosition.IsValid())
    return;

  GetOwner()->SetGlobalPosition(rootTransform.m_vPosition);

  for (ezUInt32 i = 0; i < m_ArticulationLinks.GetCount(); ++i)
  {
    poses[i].SetLocalTransform(rootTransform, ezPxConversionUtils::ToTransform(m_ArticulationLinks[i]->getGlobalPose()));
  }

  // last pose
  {
    const ezUInt32 uiLastIdx = m_ArticulationLinks.GetCount();

    ezTransform tLocal;
    tLocal.SetIdentity();
    tLocal.m_vPosition.x = (poses[uiLastIdx-1].m_vPosition - poses[uiLastIdx-2].m_vPosition).GetLength();

    poses.PeekBack().SetGlobalTransform(poses[uiLastIdx-1], tLocal);
  }

  GetOwner()->SendMessage(poseMsg);
}

void ezPxRopeComponent::SendPreviewPose()
{
  if (!IsActiveAndInitialized() || IsActiveAndSimulating())
    return;

  ezDynamicArray<ezTransform> pieces(ezFrameAllocator::GetCurrentAllocator());

  ezMsgRopePoseUpdated poseMsg;
  float fPieceLength;
  CreateSegmentTransforms(ezTransform::IdentityTransform(), pieces, fPieceLength);

  if (pieces.IsEmpty())
    return;

  poseMsg.m_LinkTransforms = pieces;
  GetOwner()->PostMessage(poseMsg, ezTime::Zero(), ezObjectMsgQueueType::AfterInitialized);
}

void ezPxRopeComponent::SetDisableGravity(bool b)
{
  if (m_bDisableGravity == b)
    return;

  m_bDisableGravity = b;

  if (m_pArticulation)
  {
    EZ_PX_WRITE_LOCK(*(m_pArticulation->getScene()));

    PxArticulationLink* pLinks[12] = {};
    const ezUInt32 uiNumLinks = m_pArticulation->getNbLinks();

    for (ezUInt32 uiFirstLink = 0; uiFirstLink < uiNumLinks; uiFirstLink += EZ_ARRAY_SIZE(pLinks))
    {
      const ezUInt32 numGotten = m_pArticulation->getLinks(pLinks, EZ_ARRAY_SIZE(pLinks), uiFirstLink);

      for (ezUInt32 i = 0; i < numGotten; ++i)
      {
        pLinks[i]->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, m_bDisableGravity);
      }
    }

    m_pArticulation->wakeUp();
  }
}

void ezPxRopeComponent::SetAnchorAReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetAnchorA(resolver(szReference, GetHandle(), "AnchorA"));

  SendPreviewPose();
}

void ezPxRopeComponent::SetAnchorBReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetAnchorB(resolver(szReference, GetHandle(), "AnchorB"));

  SendPreviewPose();
}

void ezPxRopeComponent::SetAnchorA(ezGameObjectHandle hActor)
{
  m_hAnchorA = hActor;
}

void ezPxRopeComponent::SetAnchorB(ezGameObjectHandle hActor)
{
  m_hAnchorB = hActor;
}

void ezPxRopeComponent::AddForceAtPos(ezMsgPhysicsAddForce& msg)
{
  if (m_pArticulation == nullptr || m_fMaxForcePerFrame <= 0.0f)
    return;

  EZ_PX_WRITE_LOCK(*m_pArticulation->getScene());

  PxArticulationLink* pLink[1] = {};

  if (msg.m_pInternalPhysicsActor != nullptr)
    pLink[0] = reinterpret_cast<PxArticulationLink*>(msg.m_pInternalPhysicsActor);
  else
    m_pArticulation->getLinks(pLink, 1);

  ezVec3 vImp = msg.m_vForce;
  const float fOrgImp = vImp.GetLength();

  if (fOrgImp > g_fMaxForce)
  {
    vImp.SetLength(g_fMaxForce).IgnoreResult();
    m_fMaxForcePerFrame -= g_fMaxForce;
  }
  else
  {
    m_fMaxForcePerFrame -= fOrgImp;
  }

  PxRigidBodyExt::addForceAtPos(*pLink[0], ezPxConversionUtils::ToVec3(vImp), ezPxConversionUtils::ToVec3(msg.m_vGlobalPosition), PxForceMode::eFORCE);
}

void ezPxRopeComponent::AddImpulseAtPos(ezMsgPhysicsAddImpulse& msg)
{
  if (m_pArticulation == nullptr || m_fMaxForcePerFrame <= 0.0f)
    return;

  EZ_PX_WRITE_LOCK(*m_pArticulation->getScene());

  PxArticulationLink* pLink[1] = {};

  if (msg.m_pInternalPhysicsActor != nullptr)
    pLink[0] = reinterpret_cast<PxArticulationLink*>(msg.m_pInternalPhysicsActor);
  else
    m_pArticulation->getLinks(pLink, 1);

  ezVec3 vImp = msg.m_vImpulse;
  const float fOrgImp = vImp.GetLength();

  if (fOrgImp > g_fMaxForce)
  {
    vImp.SetLength(g_fMaxForce).IgnoreResult();
    m_fMaxForcePerFrame -= g_fMaxForce;
  }
  else
  {
    m_fMaxForcePerFrame -= fOrgImp;
  }

  PxRigidBodyExt::addForceAtPos(*pLink[0], ezPxConversionUtils::ToVec3(vImp), ezPxConversionUtils::ToVec3(msg.m_vGlobalPosition), PxForceMode::eIMPULSE);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezPxRopeComponentManager::ezPxRopeComponentManager(ezWorld* pWorld)
  : ezComponentManager(pWorld)
{
}

ezPxRopeComponentManager::~ezPxRopeComponentManager() = default;

void ezPxRopeComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezPxRopeComponentManager::Update, this);
    desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = false;

    this->RegisterUpdateFunction(desc);
  }
}

void ezPxRopeComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  if (!GetWorld()->GetWorldSimulationEnabled())
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      if (it->IsActiveAndInitialized())
      {
        it->UpdatePreview();
      }
    }

    return;
  }

  ezPhysXWorldModule* pModule = GetWorld()->GetModule<ezPhysXWorldModule>();
  if (pModule == nullptr)
    return;

  EZ_PX_READ_LOCK(*pModule->GetPxScene());

  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndSimulating())
    {
      it->Update();
    }
  }
}
