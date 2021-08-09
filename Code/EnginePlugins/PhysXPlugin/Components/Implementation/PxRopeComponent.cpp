#include <PhysXPlugin/PhysXPluginPCH.h>

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Physics/RopeSimulator.h>
#include <PhysXPlugin/Components/PxDynamicActorComponent.h>
#include <PhysXPlugin/Components/PxRopeComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <PhysXPlugin/Utilities/PxUserData.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <extensions/PxD6Joint.h>
#include <foundation/Px.h>

using namespace physx;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxRopeComponent, 2, ezComponentMode::Dynamic)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_ACCESSOR_PROPERTY("Anchor", DummyGetter, SetAnchorReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
      EZ_MEMBER_PROPERTY("AttachToOrigin", m_bAttachToOrigin)->AddAttributes(new ezDefaultValueAttribute(true)),
      EZ_MEMBER_PROPERTY("AttachToAnchor", m_bAttachToAnchor)->AddAttributes(new ezDefaultValueAttribute(true)),
      EZ_MEMBER_PROPERTY("Pieces", m_uiPieces)->AddAttributes(new ezDefaultValueAttribute(16), new ezClampValueAttribute(2, 64)),
      EZ_MEMBER_PROPERTY("Slack", m_fSlack)->AddAttributes(new ezDefaultValueAttribute(0.3f)),
      EZ_MEMBER_PROPERTY("Mass", m_fTotalMass)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.1f, 1000.0f)),
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
  s << m_bAttachToOrigin;
  s << m_bAttachToAnchor;
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
  s << m_fSlack;

  stream.WriteGameObjectHandle(m_hAnchor);
}

void ezPxRopeComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  if (uiVersion < 2)
  {
    ezLog::Error("ezPxRopeComponent of version 1 is not supported anymore, please reexport the scene.");
    return;
  }

  auto& s = stream.GetStream();

  s >> m_uiCollisionLayer;
  s >> m_uiPieces;
  s >> m_fThickness;
  s >> m_bAttachToOrigin;
  s >> m_bAttachToAnchor;
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
  s >> m_fSlack;

  m_hAnchor = stream.ReadGameObjectHandle();
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
  UpdatePreview();
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

void ezPxRopeComponent::CreateRope()
{
  const ezTransform tRoot = GetOwner()->GetGlobalTransform();

  ezHybridArray<ezTransform, 65> pieces;
  float fPieceLength;
  if (CreateSegmentTransforms(pieces, fPieceLength).Failed())
    return;

  pieces.PopBack(); // don't need the last transform

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
  m_pArticulation->setSleepThreshold(0.02f);

  //m_pArticulation->setSolverIterationCounts(16, 4);
  //m_pArticulation->setMaxProjectionIterations(8);

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

    if ((m_bAttachToOrigin && idx == 0) || (m_bAttachToAnchor && idx + 1 == pieces.GetCount()))
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

  if (m_bAttachToOrigin)
  {
    m_pJointOrigin = CreateJoint(GetOwner()->GetHandle(), pieces[0], m_ArticulationLinks[0], ezTransform::IdentityTransform());
  }

  if (m_bAttachToAnchor)
  {
    ezTransform localTransform;
    localTransform.SetIdentity();
    localTransform.m_vPosition.x = fPieceLength;

    ezTransform lastPiece;
    lastPiece.SetGlobalTransform(pieces.PeekBack(), localTransform);

    localTransform.m_qRotation = -localTransform.m_qRotation;

    m_pJointAnchor = CreateJoint(m_hAnchor, lastPiece, m_ArticulationLinks.PeekBack(), localTransform);
  }
}

PxJoint* ezPxRopeComponent::CreateJoint(const ezGameObjectHandle& hTarget, const ezTransform& location, PxRigidBody* pLink, const ezTransform& linkOffset)
{
  ezPxDynamicActorComponent* pActor = nullptr;
  PxRigidActor* pPxActor = nullptr;
  ezTransform parentLocal;

  ezGameObject* pObject = nullptr;
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
  if (GetWorld()->TryGetObject(m_hAnchor, pObj))
    vNewPreviewRefPos += pObj->GetGlobalPosition();

  // TODO: use a hash value instead
  vNewPreviewRefPos.x += m_fSlack;
  vNewPreviewRefPos.y += (float)m_uiPieces;

  if (vNewPreviewRefPos != m_vPreviewRefPos)
  {
    m_vPreviewRefPos = vNewPreviewRefPos;
    SendPreviewPose();
  }
}

ezResult ezPxRopeComponent::CreateSegmentTransforms(ezDynamicArray<ezTransform>& transforms, float& out_fPieceLength) const
{
  out_fPieceLength = 0.0f;

  if (m_uiPieces == 0)
    return EZ_FAILURE;

  const ezVec3 vAnchorA = GetOwner()->GetGlobalPosition();

  const ezGameObject* pAnchor = nullptr;
  if (!GetWorld()->TryGetObject(m_hAnchor, pAnchor))
    return EZ_FAILURE;

  const ezVec3 vAnchorB = pAnchor->GetGlobalPosition();

  const float fLength = (vAnchorB - vAnchorA).GetLength();
  if (ezMath::IsZero(fLength, 0.001f))
    return EZ_FAILURE;

  // the rope simulation always introduces some sag,
  // (m_fSlack - 0.1f) puts the rope under additional tension to counteract the imprecise simulation
  // we could also drastically ramp up the simulation steps, but that costs way too much performance
  const float fIntendedRopeLength = fLength + fLength * (ezMath::Abs(m_fSlack) - 0.1f);

  ezRopeSimulator rope;
  rope.m_bFirstNodeIsFixed = true;
  rope.m_bLastNodeIsFixed = true;
  rope.m_fDampingFactor = 0.97f;
  rope.m_fSegmentLength = fIntendedRopeLength / m_uiPieces;
  rope.m_Nodes.SetCount(m_uiPieces + 1);
  rope.m_vAcceleration.Set(0, 0, ezMath::Sign(m_fSlack) * -1);

  for (ezUInt16 i = 0; i < m_uiPieces + 1; ++i)
  {
    rope.m_Nodes[i].m_vPosition = ezMath::Lerp(vAnchorA, vAnchorB, (float)i / (float)m_uiPieces);
    rope.m_Nodes[i].m_vPreviousPosition = rope.m_Nodes[i].m_vPosition;
  }

  rope.SimulateTillEquilibrium(0.001f, 200);

  transforms.SetCountUninitialized(m_uiPieces + 1);

  out_fPieceLength = 0.0f;

  for (ezUInt16 idx = 0; idx < m_uiPieces; ++idx)
  {
    const ezVec3 p0 = rope.m_Nodes[idx].m_vPosition;
    const ezVec3 p1 = rope.m_Nodes[idx + 1].m_vPosition;
    ezVec3 dir = p1 - p0;

    const float len = dir.GetLength();
    out_fPieceLength += len;

    if (len <= 0.001f)
      dir = ezVec3::UnitXAxis();
    else
      dir /= len;

    transforms[idx].m_vScale.Set(1);
    transforms[idx].m_vPosition = p0;
    transforms[idx].m_qRotation.SetShortestRotation(ezVec3::UnitXAxis(), dir);
  }

  out_fPieceLength /= m_uiPieces;

  {
    ezUInt32 idx = m_uiPieces;
    transforms[idx].m_vScale.Set(1);
    transforms[idx].m_vPosition = rope.m_Nodes[idx].m_vPosition;
    transforms[idx].m_qRotation = transforms[idx - 1].m_qRotation;
  }

  return EZ_SUCCESS;
}

void ezPxRopeComponent::DestroyPhysicsShapes()
{
  if (m_pArticulation)
  {
    ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
    EZ_PX_WRITE_LOCK(*pModule->GetPxScene());

    if (m_pJointOrigin)
    {
      m_pJointOrigin->release();
      m_pJointOrigin = nullptr;
    }

    if (m_pJointAnchor)
    {
      m_pJointAnchor->release();
      m_pJointAnchor = nullptr;
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

  // at runtime, allow to disengage the connection
  {
    if (!m_bAttachToOrigin && m_pJointOrigin)
    {
      PxRigidActor* a0;
      PxRigidActor* a1;
      m_pJointOrigin->getActors(a0, a1);

      if (a0 && a0->is<PxRigidDynamic>())
      {
        static_cast<PxRigidDynamic*>(a0)->wakeUp();
      }
      if (a1 && a1->is<PxRigidDynamic>())
      {
        static_cast<PxRigidDynamic*>(a1)->wakeUp();
      }

      m_pJointOrigin->release();
      m_pJointOrigin = nullptr;
      m_pArticulation->wakeUp();
    }

    if (!m_bAttachToAnchor && m_pJointAnchor)
    {
      PxRigidActor* a0;
      PxRigidActor* a1;
      m_pJointAnchor->getActors(a0, a1);

      if (a0 && a0->is<PxRigidDynamic>())
      {
        static_cast<PxRigidDynamic*>(a0)->wakeUp();
      }
      if (a1 && a1->is<PxRigidDynamic>())
      {
        static_cast<PxRigidDynamic*>(a1)->wakeUp();
      }

      m_pJointAnchor->release();
      m_pJointAnchor = nullptr;
      m_pArticulation->wakeUp();
    }
  }

  //if (m_fWindInfluence > 0.0f)
  //{
  //  if (const ezWindWorldModuleInterface* pWind = GetWorld()->GetModuleReadOnly<ezWindWorldModuleInterface>())
  //  {
  //    ezVec3 ropeDir = m_RopeSim.m_Nodes.PeekBack().m_vPosition - m_RopeSim.m_Nodes[0].m_vPosition;

  //    const ezVec3 vWind = pWind->GetWindAt(m_RopeSim.m_Nodes.PeekBack().m_vPosition) * m_fWindInfluence;

  //    ezVec3 windForce = vWind;
  //    windForce += pWind->ComputeWindFlutter(vWind, ropeDir, 10.0f, GetOwner()->GetStableRandomSeed());

  //    if (!windForce.IsZero())
  //    {
  //      // apply force to all articulation links
  //    }
  //  }
  //}

  if (m_pArticulation->isSleeping())
    return;

  ezHybridArray<ezTransform, 32> poses(ezFrameAllocator::GetCurrentAllocator());
  poses.SetCountUninitialized(m_ArticulationLinks.GetCount() + 1);

  ezMsgRopePoseUpdated poseMsg;
  poseMsg.m_LinkTransforms = poses;

  ezTransform rootTransform = GetOwner()->GetGlobalTransform();
  rootTransform.m_vPosition = ezPxConversionUtils::ToVec3(m_ArticulationLinks[0]->getGlobalPose().p);

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
    tLocal.m_vPosition.x = (poses[uiLastIdx - 1].m_vPosition - poses[uiLastIdx - 2].m_vPosition).GetLength();

    poses.PeekBack().SetGlobalTransform(poses[uiLastIdx - 1], tLocal);
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
  if (CreateSegmentTransforms(pieces, fPieceLength).Succeeded())
  {
    poseMsg.m_LinkTransforms = pieces;

    const ezTransform tOwner = GetOwner()->GetGlobalTransform();

    for (auto& n : pieces)
    {
      n.SetLocalTransform(tOwner, n);
    }
  }

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

void ezPxRopeComponent::SetAnchorReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetAnchor(resolver(szReference, GetHandle(), "Anchor"));
}

void ezPxRopeComponent::SetAnchor(ezGameObjectHandle hActor)
{
  m_hAnchor = hActor;
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

  EZ_PX_WRITE_LOCK(*pModule->GetPxScene());

  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndSimulating())
    {
      it->Update();
    }
  }
}
