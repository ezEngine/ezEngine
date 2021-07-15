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
      EZ_ACCESSOR_PROPERTY("AnchorA", DummyGetter, SetAnchorAReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
      EZ_ACCESSOR_PROPERTY("AnchorB", DummyGetter, SetAnchorBReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
      EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
      EZ_ACCESSOR_PROPERTY("Surface", GetSurfaceFile, SetSurfaceFile)->AddAttributes(new ezAssetBrowserAttribute("Surface")),
      EZ_ACCESSOR_PROPERTY("DisableGravity", GetDisableGravity, SetDisableGravity),
      EZ_MEMBER_PROPERTY("SelfCollision", m_bSelfCollision),
      EZ_MEMBER_PROPERTY("Pieces", m_uiPieces)->AddAttributes(new ezDefaultValueAttribute(16), new ezClampValueAttribute(4, 200)),
      EZ_MEMBER_PROPERTY("Thickness", m_fThickness)->AddAttributes(new ezDefaultValueAttribute(0.05f), new ezClampValueAttribute(0.01f, 0.5f)),
      EZ_MEMBER_PROPERTY("FixAtStart", m_bFixAtStart)->AddAttributes(new ezDefaultValueAttribute(true)),
      EZ_MEMBER_PROPERTY("FixAtEnd", m_bFixAtEnd)->AddAttributes(new ezDefaultValueAttribute(false)),
      EZ_ACCESSOR_PROPERTY("Slack", GetSlack, SetSlack)->AddAttributes(new ezDefaultValueAttribute(0.1f), new ezClampValueAttribute(0.0f, 10.0f)),
      EZ_MEMBER_PROPERTY("MaxBend", m_MaxBend)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(30)), new ezClampValueAttribute(ezAngle::Degree(5), ezAngle::Degree(90))),
      EZ_MEMBER_PROPERTY("MaxTwist", m_MaxTwist)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(15)), new ezClampValueAttribute(ezAngle::Degree(1), ezAngle::Degree(90))),
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

/* TODO:
* initial curve shape
* expose stiffness
* joint options (fixed / spherical / damping? / limit cone?)
* fix owner rotation issue
* property cleanup
*/

ezPxRopeComponent::ezPxRopeComponent() = default;
ezPxRopeComponent::~ezPxRopeComponent() = default;

void ezPxRopeComponent::SetSlack(float val)
{
  if (m_fSlack == val)
    return;

  m_fSlack = val;

  SendPreviewPose();
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
  s << m_fThickness;
  s << m_fSlack;
  s << m_bFixAtStart;
  s << m_bFixAtEnd;
  s << m_bSelfCollision;
  s << m_bDisableGravity;
  s << m_hSurface;
  s << m_MaxBend;
  s << m_MaxTwist;

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
  s >> m_fSlack;
  s >> m_bFixAtStart;
  s >> m_bFixAtEnd;
  s >> m_bSelfCollision;
  s >> m_bDisableGravity;
  s >> m_hSurface;
  s >> m_MaxBend;
  s >> m_MaxTwist;

  m_hAnchorA = stream.ReadGameObjectHandle();
  m_hAnchorB = stream.ReadGameObjectHandle();
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

ezVec3 ezPxRopeComponent::GetAnchorPositionA() const
{
  const ezGameObject* pObj;
  if (GetWorld()->TryGetObject(m_hAnchorA, pObj))
  {
    return pObj->GetGlobalPosition() - GetOwner()->GetGlobalPosition();
  }

  return ezVec3::ZeroVector();
}

ezVec3 ezPxRopeComponent::GetAnchorPositionB() const
{
  const ezGameObject* pObj;
  if (GetWorld()->TryGetObject(m_hAnchorB, pObj))
  {
    return pObj->GetGlobalPosition() - GetOwner()->GetGlobalPosition();
  }

  return ezVec3::ZeroVector();
}

void ezPxRopeComponent::CreateRope()
{
  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
  m_uiShapeID = pModule->CreateShapeId();

  const PxMaterial* pPxMaterial = GetPxMaterial();
  const PxFilterData filter = CreateFilterData();
  PxFilterData filterNone = filter;
  filterNone.word1 = 0; // disable collisions with everything else

  ezPxUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  m_pArticulation = ezPhysX::GetSingleton()->GetPhysXAPI()->createArticulation();
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
      parentFrameJoint.m_qRotation.SetIdentity();

      PxArticulationJoint* inb = reinterpret_cast<PxArticulationJoint*>(pLink->getInboundJoint());

      inb->setParentPose(ezPxConversionUtils::ToTransform(parentFrameJoint));
      inb->setSwingLimitEnabled(true);
      inb->setTwistLimitEnabled(true);
      inb->setSwingLimit(m_MaxBend.GetRadian(), m_MaxBend.GetRadian());
      inb->setTwistLimit(-m_MaxTwist.GetRadian(), m_MaxTwist.GetRadian());

      // magic values that seem to work well
      inb->setTangentialDamping(25.0f); // probably prevents changes to the rope twist
      inb->setDamping(25.0f);           // prevents changes to the rope bending
      //inb->setStiffness(25.0f); // makes the rope try to get straight
      //inb->setTangentialStiffness(25.0f); // probably makes the rope try to not have any twist
    }

    PxCapsuleGeometry shape(m_fThickness * 0.5f, fPieceLength * 0.5f);
    PxShape* pShape = PxRigidActorExt::createExclusiveShape(*pLink, shape, *pPxMaterial);

    ezVec3 vOffsetPos(fPieceLength * 0.5f, 0, 0);

    pShape->setLocalPose(ezPxConversionUtils::ToTransform(ezTransform(vOffsetPos)));
    pShape->setSimulationFilterData(filter);
    pShape->setQueryFilterData(filter);
    pShape->userData = pUserData;

    if ((m_bFixAtStart && idx == 0) || (m_bFixAtEnd && idx + 1 == pieces.GetCount()))
    {
      // disable all collisions for the first and last rope segment
      // this prevents colliding with walls that the rope is attached to
      pShape->setSimulationFilterData(filterNone);
    }

    float fMass = 0.2f;
    PxRigidBodyExt::setMassAndUpdateInertia(*pLink, fMass);
  }

  EZ_PX_WRITE_LOCK(*pModule->GetPxScene());

  m_pAggregate = ezPhysX::GetSingleton()->GetPhysXAPI()->createAggregate(m_uiPieces, m_bSelfCollision);
  m_pAggregate->addArticulation(*m_pArticulation);
  pModule->GetPxScene()->addAggregate(*m_pAggregate);

  if (m_bFixAtStart)
  {
    m_pJointA = CreateJoint(m_hAnchorA, pieces[0], m_ArticulationLinks[0], ezTransform::IdentityTransform());
  }

  if (m_bFixAtEnd)
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

    parentLocal.SetLocalTransform(pObject->GetGlobalTransform(), location);
  }
  else
  {
    parentLocal = location;
  }

  auto pPxApi = ezPhysX::GetSingleton()->GetPhysXAPI();
  return PxSphericalJointCreate(*pPxApi, pPxActor, ezPxConversionUtils::ToTransform(parentLocal), pLink, ezPxConversionUtils::ToTransform(linkOffset));
}

void ezPxRopeComponent::CreateSegmentTransforms(const ezTransform& rootTransform, ezDynamicArray<ezTransform>& transforms, float& out_fPieceLength) const
{
  out_fPieceLength = 0.0f;

  const ezVec3 vAnchorA = GetAnchorPositionA();
  const ezVec3 vAnchorB = GetAnchorPositionB();

  const float fLength = (vAnchorB - vAnchorA).GetLength();
  if (ezMath::IsZero(fLength, 0.001f))
    return;

  const ezVec3 vMainDir = (vAnchorB - vAnchorA).GetNormalized();

  // check out https://www.britannica.com/science/catenary for a more natural formula
  const ezVec3 centerPos = ezMath::Lerp(vAnchorA, vAnchorB, 0.5f) - ezVec3(0, 0, fLength * 0.5f * m_fSlack);
  float fLengthWithSlack = (centerPos - vAnchorA).GetLength() * 2;
  m_fRopeLength = fLengthWithSlack;

  ezTransform prevTransform;

  const ezUInt32 uiPieces = m_fSlack > 0 ? ezMath::RoundUp(m_uiPieces, 2) : m_uiPieces;
  transforms.SetCountUninitialized(uiPieces);
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

void ezPxRopeComponent::Update()
{
  if (m_ArticulationLinks.IsEmpty())
    return;

  if (m_pArticulation->isSleeping())
    return;

  ezHybridArray<ezTransform, 32> poses(ezFrameAllocator::GetCurrentAllocator());
  poses.SetCountUninitialized(m_ArticulationLinks.GetCount());

  ezMsgRopePoseUpdated poseMsg;
  poseMsg.m_fSegmentLength = m_fRopeLength / poses.GetCount();
  poseMsg.m_LinkTransforms = poses;

  // we can assume that the link in the middle of the array is also more or less in the middle of the rope
  const ezUInt32 uiCenterLink = m_ArticulationLinks.GetCount() / 2;

  ezTransform rootTransform = GetOwner()->GetGlobalTransform();
  rootTransform.m_vPosition = ezPxConversionUtils::ToVec3(m_ArticulationLinks[uiCenterLink]->getGlobalPose().p);
  GetOwner()->SetGlobalPosition(rootTransform.m_vPosition);

  for (ezUInt32 i = 0; i < m_ArticulationLinks.GetCount(); ++i)
  {
    poses[i].SetLocalTransform(rootTransform, ezPxConversionUtils::ToTransform(m_ArticulationLinks[i]->getGlobalPose()));
  }

  GetOwner()->SendMessage(poseMsg);
}

void ezPxRopeComponent::SendPreviewPose()
{
  if (!IsActiveAndInitialized() || IsActiveAndSimulating())
    return;

  ezDynamicArray<ezTransform> pieces(ezFrameAllocator::GetCurrentAllocator());

  ezMsgRopePoseUpdated poseMsg;
  CreateSegmentTransforms(ezTransform::IdentityTransform(), pieces, poseMsg.m_fSegmentLength);

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
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }
}

void ezPxRopeComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
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
