#include <PhysXPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Components/PxRopeComponent.h>
#include <Utilities/PxConversionUtils.h>
#include <Utilities/PxUserData.h>
#include <WorldModule/Implementation/PhysX.h>
#include <WorldModule/PhysXWorldModule.h>
#include <foundation/Px.h>

using namespace physx;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxRopeComponent, 1, ezComponentMode::Dynamic)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
      EZ_ACCESSOR_PROPERTY("Surface", GetSurfaceFile, SetSurfaceFile)->AddAttributes(new ezAssetBrowserAttribute("Surface")),
      EZ_ACCESSOR_PROPERTY("DisableGravity", GetDisableGravity, SetDisableGravity),
      EZ_MEMBER_PROPERTY("SelfCollision", m_bSelfCollision),
      EZ_MEMBER_PROPERTY("Pieces", m_uiPieces)->AddAttributes(new ezDefaultValueAttribute(16), new ezClampValueAttribute(4, 200)),
      EZ_ACCESSOR_PROPERTY("Length", GetLength, SetLength)->AddAttributes(new ezDefaultValueAttribute(2.0f), new ezClampValueAttribute(0.3f, 20.0f)),
      EZ_MEMBER_PROPERTY("Thickness", m_fThickness)->AddAttributes(new ezDefaultValueAttribute(0.05f), new ezClampValueAttribute(0.01f, 0.5f)),
      EZ_MEMBER_PROPERTY("FixAtStart", m_bFixAtStart)->AddAttributes(new ezDefaultValueAttribute(true)),
      EZ_MEMBER_PROPERTY("FixAtEnd", m_bFixAtEnd)->AddAttributes(new ezDefaultValueAttribute(false)),
      EZ_ACCESSOR_PROPERTY("Slack", GetSlack, SetSlack)->AddAttributes(new ezDefaultValueAttribute(0.1f), new ezClampValueAttribute(0.0f, 10.0f)),
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
      //new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 1.0f, ezColor::SlateGrey, nullptr, "Length"),
    }
    EZ_END_ATTRIBUTES;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

/* TODO:
* attach to joints
* initial curve shape
* start/end reference objects ?
* swing limit / bendiness / drive ?
* twist limit
* rope manager for update
*/

ezPxRopeComponent::ezPxRopeComponent() = default;
ezPxRopeComponent::~ezPxRopeComponent() = default;

void ezPxRopeComponent::SetLength(float val)
{
  if (m_fLength == val)
    return;

  m_fLength = val;

  if (IsActiveAndInitialized() && !IsActiveAndSimulating())
  {
    SendPreviewPose();
  }
}

void ezPxRopeComponent::SetSlack(float val)
{
  if (m_fSlack == val)
    return;

  m_fSlack = val;

  if (IsActiveAndInitialized() && !IsActiveAndSimulating())
  {
    SendPreviewPose();
  }
}

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
  s << m_fLength;
  s << m_fThickness;
  s << m_fSlack;
  s << m_bFixAtStart;
  s << m_bFixAtEnd;
  s << m_bSelfCollision;
  s << m_bDisableGravity;
  s << m_hSurface;
}

void ezPxRopeComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_uiCollisionLayer;
  s >> m_uiPieces;
  s >> m_fLength;
  s >> m_fThickness;
  s >> m_fSlack;
  s >> m_bFixAtStart;
  s >> m_bFixAtEnd;
  s >> m_bSelfCollision;
  s >> m_bDisableGravity;
  s >> m_hSurface;
}

PxFilterData ezPxRopeComponent::CreateFilterData()
{
  return ezPhysX::CreateFilterData(m_uiCollisionLayer, m_uiShapeID);
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
  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
  m_uiShapeID = pModule->CreateShapeId();

  ezPhysX* pEzPhysX = ezPhysX::GetSingleton();
  PxPhysics* pPxPhysX = pEzPhysX->GetPhysXAPI();

  const PxMaterial* pPxMaterial = GetPxMaterial();
  const PxFilterData filter = CreateFilterData();

  ezPxUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  m_pArticulation = pPxPhysX->createArticulation();
  m_pArticulation->userData = pUserData;

  const ezTransform tRoot = GetOwner()->GetGlobalTransform();

  ezHybridArray<ezTransform, 64> pieces;
  float fPieceLength;
  CreateSegmentTransforms(tRoot, pieces, fPieceLength);

  m_ArticulationLinks.SetCountUninitialized(pieces.GetCount());

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

      PxArticulationJoint* inb = reinterpret_cast<PxArticulationJoint*>(pLink->getInboundJoint());

      inb->setParentPose(ezPxConversionUtils::ToTransform(parentFrameJoint));
      //inb->setSwingLimitEnabled(true);
      inb->setTwistLimitEnabled(true);
      inb->setSwingLimit(ezAngle::Degree(35).GetRadian(), ezAngle::Degree(35).GetRadian());
      inb->setTwistLimit(ezAngle::Degree(-25).GetRadian(), ezAngle::Degree(25).GetRadian());
      inb->setTangentialDamping(25.0f);
      inb->setDamping(25.0f);
      inb->setStiffness(25.0f);
      inb->setTangentialStiffness(25.0f);

      inb->setInternalCompliance(0.7f);
      inb->setExternalCompliance(1.0f);
    }

    PxCapsuleGeometry shape(m_fThickness * 0.5f, fPieceLength * 0.5f);
    PxShape* pShape = PxRigidActorExt::createExclusiveShape(*pLink, shape, *pPxMaterial);

    ezVec3 vOffsetPos(fPieceLength * 0.5f, 0, 0);

    pShape->setLocalPose(ezPxConversionUtils::ToTransform(ezTransform(vOffsetPos)));
    pShape->setSimulationFilterData(filter);
    pShape->setQueryFilterData(filter);
    pShape->userData = pUserData;

    float fMass = 0.2f;
    PxRigidBodyExt::setMassAndUpdateInertia(*pLink, fMass);
  }

  EZ_PX_WRITE_LOCK(*pModule->GetPxScene());

  m_pAggregate = pPxPhysX->createAggregate(m_uiPieces, m_bSelfCollision);
  m_pAggregate->addArticulation(*m_pArticulation);
  pModule->GetPxScene()->addAggregate(*m_pAggregate);

  if (m_bFixAtStart)
  {
    m_pJointStart = PxSphericalJointCreate(*pPxPhysX, nullptr, ezPxConversionUtils::ToTransform(ezTransform(tRoot)), m_ArticulationLinks[0], ezPxConversionUtils::ToTransform(ezTransform::IdentityTransform()));
  }

  if (m_bFixAtEnd)
  {
    ezTransform localTransform;
    localTransform.SetIdentity();
    localTransform.m_vPosition.x = m_fLength;

    ezTransform globalTransform;
    globalTransform.SetGlobalTransform(tRoot, localTransform);

    localTransform.m_vPosition.x = fPieceLength;

    m_pJointEnd = PxSphericalJointCreate(*pPxPhysX, nullptr, ezPxConversionUtils::ToTransform(ezTransform(globalTransform)), m_ArticulationLinks.PeekBack(), ezPxConversionUtils::ToTransform(localTransform));
  }
}

void ezPxRopeComponent::CreateSegmentTransforms(const ezTransform& rootTransform, ezDynamicArray<ezTransform>& transforms, float& out_fPieceLength) const
{
  // check out https://www.britannica.com/science/catenary for a more natural formula
  const ezVec3 centerPos(m_fLength * 0.5f, 0, -m_fLength * 0.5f * m_fSlack);
  float fLengthWithSlack = centerPos.GetLength() * 2;

  ezTransform prevTransform;

  ezUInt32 uiPieces = m_uiPieces;

  ezQuat qRot1, qRot2;
  if (m_fSlack > 0)
  {
    qRot1.SetShortestRotation(ezVec3::UnitXAxis(), centerPos.GetNormalized());
    qRot2 = -qRot1 * -qRot1;

    uiPieces = ezMath::RoundUp(uiPieces, 2);
  }

  out_fPieceLength = fLengthWithSlack / uiPieces;

  ezQuat qRot = qRot1;
  ezVec3 vNextPos(0);

  transforms.SetCountUninitialized(uiPieces);

  for (ezUInt32 idx = 0; idx < uiPieces; ++idx)
  {
    if (idx * 2 >= uiPieces)
    {
      // TODO: fix local space joint
      qRot = qRot2;
    }

    ezTransform localTransform;
    localTransform.SetIdentity();
    localTransform.m_qRotation = qRot;
    localTransform.m_vPosition = vNextPos;
    vNextPos += qRot * ezVec3(out_fPieceLength, 0, 0);

    transforms[idx].SetGlobalTransform(rootTransform, localTransform);
  }
}

void ezPxRopeComponent::DestroyPhysicsShapes()
{
  if (m_pArticulation)
  {
    ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
    EZ_PX_WRITE_LOCK(*pModule->GetPxScene());

    if (m_pJointStart)
    {
      m_pJointStart->release();
      m_pJointStart = nullptr;
    }

    if (m_pJointEnd)
    {
      m_pJointEnd->release();
      m_pJointEnd = nullptr;
    }

    m_pArticulation->release();
    m_pArticulation = nullptr;

    m_pAggregate->release();
    m_pAggregate = nullptr;

    pModule->DeallocateUserData(m_uiUserDataIndex);
    m_uiUserDataIndex = ezInvalidIndex;
  }
}

void ezPxRopeComponent::Update()
{
  if (m_ArticulationLinks.IsEmpty())
    return;

  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
  EZ_PX_READ_LOCK(*pModule->GetPxScene());

  if (m_pArticulation->isSleeping())
    return;

  ezHybridArray<ezTransform, 32> poses(ezFrameAllocator::GetCurrentAllocator());
  poses.SetCountUninitialized(m_ArticulationLinks.GetCount());

  ezMsgRopePoseUpdated poseMsg;
  poseMsg.m_LinkTransforms = poses;

  const ezTransform rootTransform = ezPxConversionUtils::ToTransform(m_ArticulationLinks[0]->getGlobalPose());
  GetOwner()->SetGlobalTransform(rootTransform);

  // TODO: use average position as center

  for (ezUInt32 i = 0; i < m_ArticulationLinks.GetCount(); ++i)
  {
    poses[i].SetLocalTransform(rootTransform, ezPxConversionUtils::ToTransform(m_ArticulationLinks[i]->getGlobalPose()));
  }

  GetOwner()->SendMessage(poseMsg);
}

void ezPxRopeComponent::SendPreviewPose()
{
  ezDynamicArray<ezTransform> pieces(ezFrameAllocator::GetCurrentAllocator());
  float fPieceLength;
  CreateSegmentTransforms(ezTransform::IdentityTransform(), pieces, fPieceLength);

  ezMsgRopePoseUpdated poseMsg;
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

void ezPxRopeComponent::AddForceAtPos(ezMsgPhysicsAddForce& msg)
{
  if (m_pArticulation != nullptr)
  {
    EZ_PX_WRITE_LOCK(*m_pArticulation->getScene());

    PxArticulationLink* pLink[1] = {};

    if (msg.m_pInternalPhysicsActor != nullptr)
      pLink[0] = reinterpret_cast<PxArticulationLink*>(msg.m_pInternalPhysicsActor);
    else
      m_pArticulation->getLinks(pLink, 1);

    PxRigidBodyExt::addForceAtPos(*pLink[0], ezPxConversionUtils::ToVec3(msg.m_vForce), ezPxConversionUtils::ToVec3(msg.m_vGlobalPosition), PxForceMode::eFORCE);
  }
}

void ezPxRopeComponent::AddImpulseAtPos(ezMsgPhysicsAddImpulse& msg)
{
  if (m_pArticulation != nullptr)
  {
    EZ_PX_WRITE_LOCK(*m_pArticulation->getScene());

    PxArticulationLink* pLink[1] = {};

    if (msg.m_pInternalPhysicsActor != nullptr)
      pLink[0] = reinterpret_cast<PxArticulationLink*>(msg.m_pInternalPhysicsActor);
    else
      m_pArticulation->getLinks(pLink, 1);

    PxRigidBodyExt::addForceAtPos(*pLink[0], ezPxConversionUtils::ToVec3(msg.m_vImpulse), ezPxConversionUtils::ToVec3(msg.m_vGlobalPosition), PxForceMode::eIMPULSE);
  }
}
