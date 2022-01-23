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
#include <RendererCore/Debug/DebugRenderer.h>
#include <foundation/Px.h>

using namespace physx;

/* TODO
* max force clamping ?
* mass distribution
* communication with anim controller
* drive to pose
* shape scale
*/

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezPxRagdollStart, 1)
  EZ_ENUM_CONSTANTS(ezPxRagdollStart::BindPose, ezPxRagdollStart::WaitForPose, ezPxRagdollStart::Wait)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPxRagdollConstraint, 1, ezRTTIDefaultAllocator<ezPxRagdollConstraint>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Bone", m_sBone),
    EZ_MEMBER_PROPERTY("Position", m_vRelativePosition)
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezTransformManipulatorAttribute("Position")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_COMPONENT_TYPE(ezPxRagdollComponent, 2, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Start", ezPxRagdollStart, m_Start),
    EZ_ACCESSOR_PROPERTY("DisableGravity", GetDisableGravity, SetDisableGravity),
    EZ_MEMBER_PROPERTY("SelfCollision", m_bSelfCollision),
    EZ_ARRAY_MEMBER_PROPERTY("Constraints", m_Constraints),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
    EZ_MESSAGE_HANDLER(ezMsgAnimationPoseProposal, OnAnimationPoseProposal),
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
  s << m_bSelfCollision;
  // TODO m_Constraints
}

void ezPxRagdollComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_Start;
  s >> m_bDisableGravity;
  s >> m_bSelfCollision;
  // TODO m_Constraints
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


void ezPxRagdollComponent::OnAnimationPoseProposal(ezMsgAnimationPoseProposal& msg)
{
  if (!m_bShapesCreated)
    return;

  msg.m_bContinueAnimating = false;

  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
  EZ_PX_WRITE_LOCK(*pModule->GetPxScene());

  for (ezUInt32 i = 0; i < m_ArticulationLinks.GetCount(); ++i)
  {
    if (m_ArticulationLinks[i].m_pLink == nullptr)
    {
      // no need to do anything, just pass the original pose through
    }
    else
    {
      if (PxArticulationJoint* pJoint = (PxArticulationJoint*)m_ArticulationLinks[i].m_pLink->getInboundJoint())
      {
        ezQuat rot;
        rot.SetIdentity();

        pJoint->setDriveType(PxArticulationJointDriveType::eTARGET);
        pJoint->setTargetOrientation(ezPxConversionUtils::ToQuat(rot));
        pJoint->setStiffness(100);
        //pJoint->setTargetVelocity(PxVec3(1, 1, 1));
      }
    }
  }
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

void ezPxRagdollComponent::CreatePhysicsShapes(const ezSkeletonResourceHandle& hSkeleton, ezMsgAnimationPoseUpdated& poseMsg)
{
  m_bShapesCreated = true;

  ezResourceLock<ezSkeletonResource> pSkelRes(hSkeleton, ezResourceAcquireMode::BlockTillLoaded);
  const auto& skelDesc = pSkelRes->GetDescriptor();
  const auto srcBoneDir = skelDesc.m_Skeleton.m_BoneDirection;

  ezPxUserData* pPxUserData = nullptr;

  {
    ezPhysXWorldModule* pPhysModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
    m_uiShapeID = pPhysModule->CreateShapeId();
    m_uiUserDataIndex = pPhysModule->AllocateUserData(pPxUserData);
    pPxUserData->Init(this);
  }

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
      CreateBoneLink(geo.m_uiAttachedToJoint, joint, srcBoneDir, pPxUserData, thisLink, parentLink, poseMsg);
    }

    CreateBoneShape(*poseMsg.m_pRootTransform, srcBoneDir, *thisLink.m_pLink, geo, pPxUserData);

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

  CreateConstraints();
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

static void AddLine(ezHybridArray<ezDebugRenderer::Line, 32>& lines, const ezTransform& transform, const ezVec3& dir, const ezColor& color)
{
  auto& l = lines.ExpandAndGetRef();
  l.m_start = transform.m_vPosition;
  l.m_end = transform.TransformPosition(dir);
  l.m_startColor = color;
  l.m_endColor = color;
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

  const ezMat4 mInv = invRootTransform * m_RootLinkLocalTransform.GetAsMat4() * newRootTransform.GetInverse().GetAsMat4();

#if JOINT_DEBUG_DRAW
  ezHybridArray<ezDebugRenderer::Line, 32> lines;
#endif

  for (ezUInt32 i = 0; i < m_ArticulationLinks.GetCount(); ++i)
  {
    if (m_ArticulationLinks[i].m_pLink == nullptr)
    {
      // no need to do anything, just pass the original pose through
    }
    else
    {
      const ezTransform linkGlobalPose = ezPxConversionUtils::ToTransform(m_ArticulationLinks[i].m_pLink->getGlobalPose());

      ezTransform pose = linkGlobalPose;
      pose.m_qRotation = pose.m_qRotation;

      m_JointPoses[i] = mInv * pose.GetAsMat4() * scale;

#if JOINT_DEBUG_DRAW
      if (auto joint = m_ArticulationLinks[i]->getInboundJoint())
      {
        // joint in parent frame
        {
          const ezTransform jointParentPose = ezPxConversionUtils::ToTransform(joint->getParentArticulationLink().getGlobalPose());
          const ezTransform jointLocalPose = ezPxConversionUtils::ToTransform(joint->getParentPose());

          ezTransform jointGlobalPose;
          jointGlobalPose.SetGlobalTransform(jointParentPose, jointLocalPose);

          const float s = 0.1f;

          AddLine(lines, jointGlobalPose, ezVec3(s, 0, 0), ezColor::Red);
          AddLine(lines, jointGlobalPose, ezVec3(-s, 0, 0), ezColor::DarkRed);

          AddLine(lines, jointGlobalPose, ezVec3(0, s, 0), ezColor::Lime);
          AddLine(lines, jointGlobalPose, ezVec3(0, -s, 0), ezColor::DarkGreen);

          AddLine(lines, jointGlobalPose, ezVec3(0, 0, s), ezColor::Blue);
          AddLine(lines, jointGlobalPose, ezVec3(0, 0, -s), ezColor::DarkBlue);
        }

        // joint in child frame
        {
          const ezTransform jointChildPose = ezPxConversionUtils::ToTransform(m_ArticulationLinks[i]->getGlobalPose());
          const ezTransform jointLocalPose = ezPxConversionUtils::ToTransform(joint->getChildPose());

          ezTransform jointGlobalPose;
          jointGlobalPose.SetGlobalTransform(jointChildPose, jointLocalPose);

          const float s = 0.05f;

          AddLine(lines, jointGlobalPose, ezVec3(s, 0, 0), ezColor::Red);
          //AddLine(lines, jointGlobalPose, ezVec3(-s, 0, 0), ezColor::DarkRed);

          AddLine(lines, jointGlobalPose, ezVec3(0, s, 0), ezColor::Lime);
          //AddLine(lines, jointGlobalPose, ezVec3(0, -s, 0), ezColor::DarkGreen);

          AddLine(lines, jointGlobalPose, ezVec3(0, 0, s), ezColor::Blue);
          //AddLine(lines, jointGlobalPose, ezVec3(0, 0, -s), ezColor::DarkBlue);
        }

        //ezDebugRenderer::DrawCross(GetWorld(), ezVec3::ZeroVector(), 0.05f, ezColor::Azure, linkGlobalPose);
      }
#endif
    }
  }

#if JOINT_DEBUG_DRAW
  ezDebugRenderer::DrawLines(GetWorld(), lines, ezColor::White);
#endif

  GetOwner()->SendMessage(poseMsg);
}

void ezPxRagdollComponent::Update()
{
  if (!m_bShapesCreated)
    return;

  UpdatePose();

  if (!m_NextImpulse.m_vImpulse.IsZero())
  {
    ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
    EZ_PX_WRITE_LOCK(*pModule->GetPxScene());

    if (m_NextImpulse.m_pTargetLink == nullptr)
    {
      float fBestDist = ezMath::HighValue<float>();
      m_NextImpulse.m_pTargetLink = m_pRootLink;

      // search for the best link to apply the impulse to
      for (const auto& link : m_ArticulationLinks)
      {
        if (link.m_pLink == nullptr)
          continue;

        const float fDistSqr = (ezPxConversionUtils::ToVec3(link.m_pLink->getGlobalPose().p) - m_NextImpulse.m_vPos).GetLengthSquared();

        if (fDistSqr < fBestDist)
        {
          fBestDist = fDistSqr;
          m_NextImpulse.m_pTargetLink = link.m_pLink;
        }
      }
    }

    PxRigidBodyExt::addForceAtPos(*m_NextImpulse.m_pTargetLink, ezPxConversionUtils::ToVec3(m_NextImpulse.m_vImpulse), ezPxConversionUtils::ToVec3(m_NextImpulse.m_vPos), PxForceMode::eIMPULSE);

    m_NextImpulse.m_pTargetLink = nullptr;
    m_NextImpulse.m_vImpulse.SetZero();
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

  auto getBone = [&](ezUInt32 i, auto f) -> ezMat4 {
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
}

void ezPxRagdollComponent::CreateBoneShape(const ezTransform& rootTransform, ezBasisAxis::Enum srcBoneDir, physx::PxRigidActor& actor, const ezSkeletonResourceGeometry& geo, ezPxUserData* pPxUserData)
{
  PxFilterData pxFilterData = ezPhysX::CreateFilterData(geo.m_uiCollisionLayer, m_uiShapeID);

  physx::PxMaterial* pxMaterial = nullptr;
  if (geo.m_hSurface.IsValid())
  {
    ezResourceLock<ezSurfaceResource> pSurface(geo.m_hSurface, ezResourceAcquireMode::BlockTillLoaded);

    if (pSurface->m_pPhysicsMaterial != nullptr)
    {
      pxMaterial = static_cast<physx::PxMaterial*>(pSurface->m_pPhysicsMaterial);
    }
  }
  else
    pxMaterial = ezPhysX::GetSingleton()->GetDefaultMaterial();

  PxShape* pShape = nullptr;

  const ezQuat qBoneDirAdjustment = ezBasisAxis::GetBasisRotation(ezBasisAxis::PositiveX, srcBoneDir);

  const ezQuat qFinalBoneRot = /*boneRot **/ qBoneDirAdjustment;

  ezTransform st;
  st.SetIdentity();
  st.m_vPosition = /*boneTrans.GetTranslationVector() +*/ qFinalBoneRot * geo.m_Transform.m_vPosition;
  st.m_qRotation = qFinalBoneRot * geo.m_Transform.m_qRotation;

  if (geo.m_Type == ezSkeletonJointGeometryType::Sphere)
  {
    PxSphereGeometry shape(geo.m_Transform.m_vScale.z);
    pShape = PxRigidActorExt::createExclusiveShape(actor, shape, *pxMaterial);
  }
  else if (geo.m_Type == ezSkeletonJointGeometryType::Box)
  {
    ezVec3 ext;
    ext.x = geo.m_Transform.m_vScale.x * 0.5f;
    ext.y = geo.m_Transform.m_vScale.y * 0.5f;
    ext.z = geo.m_Transform.m_vScale.z * 0.5f;

    // TODO: if offset desired
    st.m_vPosition += qFinalBoneRot * ezVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);

    PxBoxGeometry shape(ext.x, ext.y, ext.z);
    pShape = PxRigidActorExt::createExclusiveShape(actor, shape, *pxMaterial);
  }
  else if (geo.m_Type == ezSkeletonJointGeometryType::Capsule)
  {
    PxCapsuleGeometry shape(geo.m_Transform.m_vScale.z, geo.m_Transform.m_vScale.x * 0.5f);
    pShape = PxRigidActorExt::createExclusiveShape(actor, shape, *pxMaterial);

    // TODO: if offset desired
    st.m_vPosition += qFinalBoneRot * ezVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);
  }
  else
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  pShape->setLocalPose(ezPxConversionUtils::ToTransform(st));
  pShape->setSimulationFilterData(pxFilterData);
  pShape->setQueryFilterData(pxFilterData);
  pShape->userData = pPxUserData;
}

void ezPxRagdollComponent::CreateBoneLink(ezUInt16 uiBoneIdx, const ezSkeletonJoint& joint, ezBasisAxis::Enum srcBoneDir, ezPxUserData* pPxUserData, LinkData& thisLink, const LinkData& parentLink, ezMsgAnimationPoseUpdated& poseMsg)
{
  ezTransform linkTransform;
  {
    ezMat4 mFullTransform;
    poseMsg.ComputeFullBoneTransform(uiBoneIdx, mFullTransform, linkTransform.m_qRotation);

    linkTransform.m_vScale.Set(1);
    linkTransform.m_vPosition = mFullTransform.GetTranslationVector();
  }

  thisLink.m_GlobalTransform.SetGlobalTransform(GetOwner()->GetGlobalTransform(), linkTransform);

  thisLink.m_pLink = m_pArticulation->createLink(parentLink.m_pLink, ezPxConversionUtils::ToTransform(thisLink.m_GlobalTransform));
  EZ_ASSERT_DEV(thisLink.m_pLink != nullptr, "Ragdoll shape creation failed. Too many bones? (max 64)");

  m_ArticulationLinks[uiBoneIdx].m_sBoneName = joint.GetName();
  m_ArticulationLinks[uiBoneIdx].m_pLink = thisLink.m_pLink;
  thisLink.m_pLink->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, m_bDisableGravity);
  thisLink.m_pLink->setLinearVelocity(ezPxConversionUtils::ToVec3(GetOwner()->GetVelocity()), false);

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

  // joint setup
  {
    // the main direction of PhysX bones is +X (for bone limits and such)
    // therefore the main direction of the source bones has to be adjusted
    ezQuat qBoneDirAdjustment = -ezBasisAxis::GetBasisRotation(srcBoneDir, ezBasisAxis::PositiveX);

    ezTransform parentFrameJoint;
    parentFrameJoint.SetLocalTransform(parentLink.m_GlobalTransform, thisLink.m_GlobalTransform);
    parentFrameJoint.m_qRotation = joint.GetLocalOrientation() * qBoneDirAdjustment;

    ezTransform childFrameJoint;
    childFrameJoint.SetIdentity();
    childFrameJoint.m_qRotation = qBoneDirAdjustment;

    PxArticulationJoint* inb = reinterpret_cast<PxArticulationJoint*>(thisLink.m_pLink->getInboundJoint());
    inb->setInternalCompliance(0.7f);
    inb->setExternalCompliance(1.0f);

    inb->setParentPose(ezPxConversionUtils::ToTransform(parentFrameJoint));
    inb->setChildPose(ezPxConversionUtils::ToTransform(childFrameJoint));

    if (joint.GetHalfSwingLimitZ() >= ezAngle::Degree(1) || joint.GetHalfSwingLimitY() >= ezAngle::Degree(1))
    {
      inb->setSwingLimitEnabled(true);
      inb->setSwingLimit(ezMath::Max(ezAngle::Degree(1), joint.GetHalfSwingLimitZ()).GetRadian(), ezMath::Max(ezAngle::Degree(1), joint.GetHalfSwingLimitY()).GetRadian());
      inb->setSwingLimitContactDistance(ezAngle::Degree(10).GetRadian()); // ??
    }
    else
    {
      inb->setSwingLimitEnabled(false);
    }

    if (joint.GetTwistLimitLow() >= ezAngle::Degree(1) || joint.GetTwistLimitHigh() >= ezAngle::Degree(1))
    {
      ezAngle low = -joint.GetTwistLimitLow();
      ezAngle high = joint.GetTwistLimitHigh();

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
}

void ezPxRagdollComponent::CreateConstraints()
{
  if (m_Constraints.IsEmpty())
    return;

  ezPhysXWorldModule* pPhysModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();

  EZ_PX_WRITE_LOCK(*pPhysModule->GetPxScene());

  for (auto& constraint : m_Constraints)
  {
    for (const auto& link : m_ArticulationLinks)
    {
      if (link.m_sBoneName == constraint.m_sBone)
      {
        const ezTransform pos(GetOwner()->GetGlobalTransform().TransformPosition(constraint.m_vRelativePosition));

        auto pJoint = PxSphericalJointCreate(*(ezPhysX::GetSingleton()->GetPhysXAPI()), nullptr, ezPxConversionUtils::ToTransform(pos), link.m_pLink, ezPxConversionUtils::ToTransform(ezTransform::IdentityTransform()));

        break;
      }
    }
  }
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
  EZ_ASSERT_DEV(!msg.m_vImpulse.IsNaN() && !msg.m_vGlobalPosition.IsNaN(), "ezMsgPhysicsAddImpulse contains invalid (NaN) impulse or position");

  if (msg.m_vImpulse.GetLengthSquared() > m_NextImpulse.m_vImpulse.GetLengthSquared())
  {
    m_NextImpulse.m_vPos = msg.m_vGlobalPosition;
    m_NextImpulse.m_vImpulse = msg.m_vImpulse;
    m_NextImpulse.m_pTargetLink = static_cast<PxArticulationLink*>(msg.m_pInternalPhysicsActor);

    if (m_NextImpulse.m_pTargetLink)
    {
      EZ_ASSERT_DEBUG(ezStringUtils::IsEqual(m_NextImpulse.m_pTargetLink->getConcreteTypeName(), "PxArticulationLink"), "Expected PxArticulationLink, got {}", m_NextImpulse.m_pTargetLink->getConcreteTypeName());
    }
  }
}

//////////////////////////////////////////////////////////////////////////

ezPxRagdollComponentManager::ezPxRagdollComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezPxRagdollComponent, ezBlockStorageType::FreeList>(pWorld)
{
}

ezPxRagdollComponentManager::~ezPxRagdollComponentManager() = default;

void ezPxRagdollComponentManager::Initialize()
{
  // configure this system to update all components multi-threaded (async phase)

  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezPxRagdollComponentManager::UpdateRagdolls, this);
  desc.m_bOnlyUpdateWhenSimulating = true;
  desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PreAsync; // TODO: not clear yet when ragdolls need to be updated
  // desc.m_uiGranularity = 8;

  RegisterUpdateFunction(desc);
}

void ezPxRagdollComponentManager::UpdateRagdolls(const ezWorldModule::UpdateContext& context)
{
  for (auto it = m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndSimulating())
    {
      it->Update();
    }
  }
}
