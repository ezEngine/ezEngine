#include <PhysXPlugin/PhysXPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Components/PxRagdollComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <PhysXPlugin/Utilities/PxUserData.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <foundation/Px.h>

using namespace physx;

/* TODO
* angular momentum preservation
* joint limit config
* joint limit frame positioning
* materials
* force application
* mass distribution
* communication with anim controller
* drive to pose
* shape scale
*/

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezPxRagdollStart, 1)
  EZ_ENUM_CONSTANTS(ezPxRagdollStart::BindPose, ezPxRagdollStart::WaitForPose, ezPxRagdollStart::WaitForPoseAndVelocity, ezPxRagdollStart::Wait)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_COMPONENT_TYPE(ezPxRagdollComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Start", ezPxRagdollStart, m_Start),
    EZ_ACCESSOR_PROPERTY("Surface", GetSurfaceFile, SetSurfaceFile)->AddAttributes(new ezAssetBrowserAttribute("Surface")),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_ACCESSOR_PROPERTY("DisableGravity", GetDisableGravity, SetDisableGravity),
    EZ_MEMBER_PROPERTY("SelfCollision", m_bSelfCollision),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
    EZ_MESSAGE_HANDLER(ezMsgPhysicsAddForce, AddForceAtPos),
    EZ_MESSAGE_HANDLER(ezMsgPhysicsAddImpulse, AddImpulseAtPos),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Animation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPxRagdollComponent::ezPxRagdollComponent() = default;
ezPxRagdollComponent::~ezPxRagdollComponent() = default;

void ezPxRagdollComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_Start;
  s << m_bDisableGravity;
  s << m_uiCollisionLayer;
  s << m_bSelfCollision;
  s << m_hSurface;
}

void ezPxRagdollComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_Start;
  s >> m_bDisableGravity;
  s >> m_uiCollisionLayer;
  s >> m_bSelfCollision;
  s >> m_hSurface;
}

void ezPxRagdollComponent::SetSurfaceFile(const char* szFile)
{
  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hSurface = ezResourceManager::LoadResource<ezSurfaceResource>(szFile);
  }

  if (m_hSurface.IsValid())
    ezResourceManager::PreloadResource(m_hSurface);
}

const char* ezPxRagdollComponent::GetSurfaceFile() const
{
  if (!m_hSurface.IsValid())
    return "";

  return m_hSurface.GetResourceID();
}

PxMaterial* ezPxRagdollComponent::GetPxMaterial()
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

PxFilterData ezPxRagdollComponent::CreateFilterData()
{
  return ezPhysX::CreateFilterData(m_uiCollisionLayer, m_uiShapeID);
}

void ezPxRagdollComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_Start == ezPxRagdollStart::BindPose)
  {
    CreateShapesFromBindPose();
  }
}

void ezPxRagdollComponent::OnDeactivated()
{
  DestroyPhysicsShapes();

  SUPER::OnDeactivated();
}

void ezPxRagdollComponent::OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& poseMsg)
{
  if (!IsActiveAndSimulating())
    return;

  if (m_Start == ezPxRagdollStart::Wait || m_Start == ezPxRagdollStart::BindPose)
    return;

  if (m_bShapesCreated)
  {
    // TODO: if at some point we can layer ragdolls with detail animations, we should
    // take poses for all bones for which there are no shapes (link == null) -> to animate leafs (fingers and such)
    return;
  }

  m_JointPoses = poseMsg.m_ModelTransforms;

  if (!m_hSkeleton.IsValid())
  {
    ezMsgQueryAnimationSkeleton msg;
    GetOwner()->SendMessage(msg);

    if (!msg.m_hSkeleton.IsValid())
      return;

    m_hSkeleton = msg.m_hSkeleton;
  }

  if (m_bHasFirstState == false && m_Start == ezPxRagdollStart::WaitForPoseAndVelocity)
  {
    m_bHasFirstState = true;

    // TODO: positions are not enough since bones don't really change their position, but mostly their angular momentum
    // -> probably need to store bone rotations and compute the angular change instead

    //m_vLastPos.SetCountUninitialized(poseMsg.m_ModelTransforms.GetCount());

    //for (ezUInt32 i = 0; i < poseMsg.m_ModelTransforms.GetCount(); ++i)
    //{
    //  m_vLastPos[i] = poseMsg.m_ModelTransforms[i].GetTranslationVector();
    //}

    return;
  }

  CreatePhysicsShapes(m_hSkeleton, poseMsg);
}

void ezPxRagdollComponent::SetDisableGravity(bool b)
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

static void ComputeBoneRotation(const ezMat4& fullTransform, ezQuat& rotationOnly)
{
  // the bone might contain (non-uniform) scaling and mirroring, which the quaternion can't represent
  // so reconstruct a representable rotation matrix
  {
    const ezVec3 x = fullTransform.TransformDirection(ezVec3(1, 0, 0)).GetNormalized();
    const ezVec3 y = fullTransform.TransformDirection(ezVec3(0, 1, 0)).GetNormalized();
    const ezVec3 z = x.CrossRH(y);

    ezMat3 m;
    m.SetColumn(0, x);
    m.SetColumn(1, y);
    m.SetColumn(2, z);

    rotationOnly.SetFromMat3(m);
  }
}

void ezPxRagdollComponent::CreatePhysicsShapes(const ezSkeletonResourceHandle& hSkeleton, ezMsgAnimationPoseUpdated& poseMsg)
{
  m_bShapesCreated = true;

  ezResourceLock<ezSkeletonResource> pSkelRes(hSkeleton, ezResourceAcquireMode::BlockTillLoaded);
  const auto& skelDesc = pSkelRes->GetDescriptor();

  ezPxUserData* pPxUserData = nullptr;

  {
    ezPhysXWorldModule* pPhysModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
    m_uiShapeID = pPhysModule->CreateShapeId();
    m_uiUserDataIndex = pPhysModule->AllocateUserData(pPxUserData);
  }

  pPxUserData->Init(this);
  const PxMaterial& pxMaterial = *GetPxMaterial();
  const PxFilterData pxFilter = CreateFilterData();

  m_pArticulation = ezPhysX::GetSingleton()->GetPhysXAPI()->createArticulation();
  m_pArticulation->userData = pPxUserData;

  m_ArticulationLinks.SetCount(poseMsg.m_ModelTransforms.GetCount());

  ezMap<ezUInt16, LinkData> linkData(ezFrameAllocator::GetCurrentAllocator());
  linkData.FindOrAdd(ezInvalidJointIndex); // dummy root link

  for (const auto& geo : skelDesc.m_Geometry)
  {
    if (geo.m_Type == ezSkeletonJointGeometryType::None)
      continue;

    const ezSkeletonJoint& joint = skelDesc.m_Skeleton.GetJointByIndex(geo.m_uiAttachedToJoint);

    ezUInt16 uiParentJointIdx = joint.GetParentIndex();

    // find the parent joint that is also part of the ragdoll
    while (!linkData.Contains(uiParentJointIdx))
    {
      uiParentJointIdx = skelDesc.m_Skeleton.GetJointByIndex(uiParentJointIdx).GetParentIndex();
    }

    const auto& parentLink = linkData[uiParentJointIdx];
    auto& thisLink = linkData[geo.m_uiAttachedToJoint];

    if (thisLink.m_pLink == nullptr)
    {
      CreateBoneLink(geo.m_uiAttachedToJoint, joint, pPxUserData, thisLink, parentLink, poseMsg);
    }

    CreateBoneShape(*poseMsg.m_pRootTransform, *thisLink.m_pLink, geo, pxMaterial, pxFilter, pPxUserData);

    // TODO mass distribution
    {
      float fMass = 5.0f;

      if (parentLink.m_pLink)
      {
        fMass = 0.9f * parentLink.m_pLink->getMass();
      }

      PxRigidBodyExt::setMassAndUpdateInertia(*thisLink.m_pLink, fMass);
    }
  }

  // TODO: need a better way to communicate with other animation producers
  // probably just move all the ragdoll stuff into an animation graph node and then use different messages to input and forward poses
  poseMsg.m_bContinueAnimating = false;
  AddArticulationToScene();
}

void ezPxRagdollComponent::DestroyPhysicsShapes()
{
  if (m_pArticulation)
  {
    ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
    EZ_PX_WRITE_LOCK(*pModule->GetPxScene());

    m_pArticulation->release();
    m_pArticulation = nullptr;

    m_pAggregate->release();
    m_pAggregate = nullptr;

    pModule->DeallocateUserData(m_uiUserDataIndex);
    m_uiUserDataIndex = ezInvalidIndex;
  }
}

void ezPxRagdollComponent::UpdatePose()
{
  if (!m_bShapesCreated)
    return;

  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
  EZ_PX_READ_LOCK(*pModule->GetPxScene());

  if (m_pArticulation->isSleeping())
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);
  const auto& desc = pSkeleton->GetDescriptor();

  EZ_ASSERT_DEBUG(m_JointPoses.GetCount() == m_ArticulationLinks.GetCount(), "Invalid pose matrix size");

  const ezTransform rootTransform = desc.m_RootTransform;
  const ezMat4 invRootTransform = rootTransform.GetAsMat4().GetInverse();

  ezMat4 scale;
  scale.SetScalingMatrix(rootTransform.m_vScale);

  ezMsgAnimationPoseUpdated poseMsg;
  poseMsg.m_ModelTransforms = m_JointPoses;
  poseMsg.m_pRootTransform = &rootTransform;
  poseMsg.m_pSkeleton = &desc.m_Skeleton;

  const ezTransform newRootTransform = ezPxConversionUtils::ToTransform(m_pRootLink->getGlobalPose());
  GetOwner()->SetGlobalTransform(newRootTransform * m_RootLinkLocalTransform.GetInverse());

  ezMat4 mInv = invRootTransform * m_RootLinkLocalTransform.GetAsMat4() * newRootTransform.GetInverse().GetAsMat4();

  const ezQuat qTilt = ezBasisAxis::GetBasisRotation(ezBasisAxis::PositiveX, ezBasisAxis::PositiveY);

  for (ezUInt32 i = 0; i < m_ArticulationLinks.GetCount(); ++i)
  {
    if (m_ArticulationLinks[i] == nullptr)
    {
      // no need to do anything, just pass the original pose through
    }
    else
    {
      ezTransform pose = ezPxConversionUtils::ToTransform(m_ArticulationLinks[i]->getGlobalPose());
      pose.m_qRotation = pose.m_qRotation * qTilt;

      m_JointPoses[i] = mInv * pose.GetAsMat4() * scale;
    }
  }

  GetOwner()->SendMessage(poseMsg);
}

void ezPxRagdollComponent::Update()
{
  if (m_bShapesCreated)
  {
    UpdatePose();
  }
}

void ezPxRagdollComponent::CreateShapesFromBindPose()
{
  if (m_bShapesCreated)
    return;

  {
    ezMsgQueryAnimationSkeleton msg;
    GetOwner()->SendMessage(msg);

    if (!msg.m_hSkeleton.IsValid())
      return;

    m_hSkeleton = msg.m_hSkeleton;
  }

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);
  const auto& desc = pSkeleton->GetDescriptor();

  m_JointPoses.SetCountUninitialized(desc.m_Skeleton.GetJointCount());

  auto getBone = [&](ezUInt32 i, auto f) -> ezMat4
  {
    const auto& j = desc.m_Skeleton.GetJointByIndex(i);
    const ezMat4 bm = j.GetBindPoseLocalTransform().GetAsMat4();

    if (j.GetParentIndex() != ezInvalidJointIndex)
    {
      const ezMat4 pbm = f(j.GetParentIndex(), f);

      return pbm * bm;
    }

    return bm;
  };

  for (ezUInt32 i = 0; i < m_JointPoses.GetCount(); ++i)
  {
    m_JointPoses[i] = getBone(i, getBone);
  }

  ezMsgAnimationPoseUpdated msg;
  msg.m_pRootTransform = &desc.m_RootTransform;
  msg.m_pSkeleton = &desc.m_Skeleton;
  msg.m_ModelTransforms = m_JointPoses;

  CreatePhysicsShapes(m_hSkeleton, msg);
}

void ezPxRagdollComponent::AddArticulationToScene()
{
  PxPhysics* pPhysApi = ezPhysX::GetSingleton()->GetPhysXAPI();
  ezPhysXWorldModule* pPhysModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();

  EZ_PX_WRITE_LOCK(*pPhysModule->GetPxScene());

  m_pAggregate = pPhysApi->createAggregate(64, m_bSelfCollision);
  m_pAggregate->addArticulation(*m_pArticulation);
  pPhysModule->GetPxScene()->addAggregate(*m_pAggregate);

  {
    PxArticulationLink* pLink[1] = {};
    m_pArticulation->getLinks(pLink, 1);

    for (const auto& imp : m_Impulses)
    {
      PxRigidBodyExt::addForceAtPos(*pLink[0], ezPxConversionUtils::ToVec3(imp.m_vImpulse), ezPxConversionUtils::ToVec3(imp.m_vPos), PxForceMode::eIMPULSE);
      //pLink[0]->addForce(ezPxConversionUtils::ToVec3(imp.m_vImpulse), PxForceMode::eIMPULSE);
    }

    m_Impulses.Clear();
  }
}

void ezPxRagdollComponent::CreateBoneShape(const ezTransform& rootTransform, physx::PxRigidActor& actor, const ezSkeletonResourceGeometry& geo, const PxMaterial& pxMaterial, const PxFilterData& pxFilterData, ezPxUserData* pPxUserData)
{
  ezVec3 vOffsetPos = geo.m_Transform.m_qRotation * geo.m_Transform.m_vPosition;
  ezQuat qOffsetRot = geo.m_Transform.m_qRotation;

  const float fScale = 1.0f;

  PxShape* pShape = nullptr;

  // maybe can get rid of this and instead always add the offset to X (in Px space)
  const ezQuat qTilt = ezBasisAxis::GetBasisRotation(ezBasisAxis::PositiveX, ezBasisAxis::PositiveY);

  if (geo.m_Type == ezSkeletonJointGeometryType::Sphere)
  {
    PxSphereGeometry shape(fScale * geo.m_Transform.m_vScale.z);
    pShape = PxRigidActorExt::createExclusiveShape(actor, shape, pxMaterial);
  }
  else if (geo.m_Type == ezSkeletonJointGeometryType::Box)
  {
    ezVec3 ext;
    ext.x = geo.m_Transform.m_vScale.y * 0.5f;
    ext.y = geo.m_Transform.m_vScale.x * 0.5f;
    ext.z = geo.m_Transform.m_vScale.z * 0.5f;
    ext *= fScale;

    PxBoxGeometry shape(ext.x, ext.y, ext.z);
    pShape = PxRigidActorExt::createExclusiveShape(actor, shape, pxMaterial);

    vOffsetPos += geo.m_Transform.m_qRotation * qTilt * ezVec3(0, geo.m_Transform.m_vScale.x * 0.5f, 0);
  }
  else if (geo.m_Type == ezSkeletonJointGeometryType::Capsule)
  {
    ezQuat qRotCapsule;
    qRotCapsule.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(90));

    PxCapsuleGeometry shape(fScale * geo.m_Transform.m_vScale.z, fScale * geo.m_Transform.m_vScale.x * 0.5f);
    pShape = PxRigidActorExt::createExclusiveShape(actor, shape, pxMaterial);

    vOffsetPos += geo.m_Transform.m_qRotation * qTilt * ezVec3(0, geo.m_Transform.m_vScale.x * 0.5f, 0);
    qOffsetRot = qOffsetRot * qRotCapsule;
  }
  else
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  //pShape->SetSurfaceFile(geo.m_sSurface);

  vOffsetPos = vOffsetPos.CompMul(rootTransform.m_vScale);

  ezTransform shapeTransform(vOffsetPos, qOffsetRot);

  //const ezQuat qTilt = ezBasisAxis::GetBasisRotation(ezBasisAxis::PositiveY, ezBasisAxis::PositiveX);
  shapeTransform.m_qRotation = shapeTransform.m_qRotation * qTilt;

  pShape->setLocalPose(ezPxConversionUtils::ToTransform(shapeTransform));
  pShape->setSimulationFilterData(pxFilterData);
  pShape->setQueryFilterData(pxFilterData);
  pShape->userData = pPxUserData;
}

void ezPxRagdollComponent::CreateBoneLink(ezUInt16 uiBoneIdx, const ezSkeletonJoint& bone, ezPxUserData* pPxUserData, LinkData& thisLink, const LinkData& parentLink, ezMsgAnimationPoseUpdated& poseMsg)
{
  ezMat4 mFullTransform;
  ezQuat qFullRotation;
  poseMsg.ComputeFullBoneTransform(uiBoneIdx, mFullTransform, qFullRotation);

  ezTransform linkTransform;
  linkTransform.SetIdentity();

  linkTransform.m_vPosition = mFullTransform.GetTranslationVector();
  linkTransform.m_qRotation = qFullRotation;

  thisLink.m_GlobalTransform.SetGlobalTransform(GetOwner()->GetGlobalTransform(), linkTransform);

  const ezQuat qTilt = ezBasisAxis::GetBasisRotation(ezBasisAxis::PositiveY, ezBasisAxis::PositiveX);
  thisLink.m_GlobalTransform.m_qRotation = thisLink.m_GlobalTransform.m_qRotation * qTilt;

  thisLink.m_pLink = m_pArticulation->createLink(parentLink.m_pLink, ezPxConversionUtils::ToTransform(thisLink.m_GlobalTransform));
  EZ_ASSERT_DEV(thisLink.m_pLink != nullptr, "Ragdoll shape creation failed. Too many bones? (max 64)");

  m_ArticulationLinks[uiBoneIdx] = thisLink.m_pLink;
  thisLink.m_pLink->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, m_bDisableGravity);
  thisLink.m_pLink->setLinearVelocity(ezPxConversionUtils::ToVec3(GetOwner()->GetVelocity()));
  thisLink.m_pLink->userData = pPxUserData;

  if (parentLink.m_pLink == nullptr)
  {
    if (m_pRootLink == nullptr)
    {
      m_pRootLink = thisLink.m_pLink;
      m_RootLinkLocalTransform.SetLocalTransform(GetOwner()->GetGlobalTransform(), thisLink.m_GlobalTransform);
    }

    return;
  }

  ezTransform parentFrameJoint;
  parentFrameJoint.SetLocalTransform(parentLink.m_GlobalTransform, thisLink.m_GlobalTransform);

  PxArticulationJoint* inb = reinterpret_cast<PxArticulationJoint*>(thisLink.m_pLink->getInboundJoint());
  inb->setInternalCompliance(0.7f);
  inb->setExternalCompliance(1.0f);

  inb->setParentPose(ezPxConversionUtils::ToTransform(parentFrameJoint));

  if (bone.GetHalfSwingLimitX() >= ezAngle::Degree(1) || bone.GetHalfSwingLimitY() >= ezAngle::Degree(1))
  {
    inb->setSwingLimitEnabled(true);
    inb->setSwingLimit(ezMath::Max(ezAngle::Degree(1), bone.GetHalfSwingLimitX()).GetRadian(), ezMath::Max(ezAngle::Degree(1), bone.GetHalfSwingLimitY()).GetRadian());
    // inb->setSwingLimitContactDistance(ezAngle::Degree(10).GetRadian()); // ??
  }
  else
  {
    inb->setSwingLimitEnabled(false);
  }

  if (bone.GetTwistLimitLow() >= ezAngle::Degree(1) || bone.GetTwistLimitHigh() >= ezAngle::Degree(1))
  {
    ezAngle low = -bone.GetTwistLimitLow();
    ezAngle high = bone.GetTwistLimitHigh();

    low = ezMath::Max(ezAngle::Degree(-179), low);
    high = ezMath::Min(ezAngle::Degree(+179), high);

    inb->setTwistLimitEnabled(true);
    inb->setTwistLimit(low.GetRadian(), high.GetRadian());
    // inb->setTwistLimitContactDistance(); // ??
  }
  else
  {
    inb->setTwistLimitEnabled(false);
  }

  inb->setTangentialStiffness(1000.0f);
  inb->setTangentialDamping(25.0f);

  inb->setDamping(25.0f);
}

void ezPxRagdollComponent::AddForceAtPos(ezMsgPhysicsAddForce& msg)
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

void ezPxRagdollComponent::AddImpulseAtPos(ezMsgPhysicsAddImpulse& msg)
{
  if (!m_bShapesCreated)
  {
    auto& imp = m_Impulses.ExpandAndGetRef();
    imp.m_vPos = msg.m_vGlobalPosition;
    imp.m_vImpulse = msg.m_vImpulse;
    return;
  }

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
