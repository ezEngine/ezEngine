#include <PhysXPlugin/PhysXPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Components/PxRagdollComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <PhysXPlugin/Utilities/PxUserData.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonPoseComponent.h>
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

EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezPxRagdollComponent, 4)
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
    EZ_MESSAGE_HANDLER(ezMsgRetrieveBoneState, OnRetrieveBoneState),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/PhysX/Animation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

ezResult ezPxRagdollConstraint::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream << m_sBone;
  inout_stream << m_vRelativePosition;
  return EZ_SUCCESS;
}

ezResult ezPxRagdollConstraint::Deserialize(ezStreamReader& inout_stream)
{
  inout_stream >> m_sBone;
  inout_stream >> m_vRelativePosition;
  return EZ_SUCCESS;
}

ezPxRagdollComponent::ezPxRagdollComponent() = default;
ezPxRagdollComponent::~ezPxRagdollComponent() = default;

void ezPxRagdollComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_Start;
  s << m_bDisableGravity;
  s << m_bSelfCollision;
  s.WriteArray(m_Constraints).AssertSuccess();
}

void ezPxRagdollComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  if (uiVersion < 4)
    return;

  s >> m_Start;
  s >> m_bDisableGravity;
  s >> m_bSelfCollision;
  s.ReadArray(m_Constraints).AssertSuccess();
}

void ezPxRagdollComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_Start == ezPxRagdollStart::BindPose)
  {
    SetupLimbsFromBindPose();
  }
}

void ezPxRagdollComponent::OnDeactivated()
{
  ClearPhysicsObjects();

  SUPER::OnDeactivated();
}

void ezPxRagdollComponent::Update()
{
  if (!m_bLimbsSetup)
    return;

  RetrievePhysicsPose();

  ApplyImpulse();
}

bool ezPxRagdollComponent::EnsureSkeletonIsKnown()
{
  if (!m_hSkeleton.IsValid())
  {
    ezMsgQueryAnimationSkeleton msg;
    GetOwner()->SendMessage(msg);
    m_hSkeleton = msg.m_hSkeleton;
  }

  return m_hSkeleton.IsValid();
}

void ezPxRagdollComponent::ClearPhysicsObjects()
{
  if (m_bLimbsSetup == false)
    return;

  ezPhysXWorldModule* pPxModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
  EZ_PX_WRITE_LOCK(*pPxModule->GetPxScene());

  if (m_pPxAggregate)
  {
    m_pPxAggregate->release();
    m_pPxAggregate = nullptr;
  }

  pPxModule->DeallocateUserData(m_uiPxUserDataIndex);

  m_uiPxUserDataIndex = ezInvalidIndex;
  m_pPxUserData = nullptr;
  m_pPxRootBody = nullptr;
  m_uiPxShapeID = ezInvalidIndex;

  m_Limbs.Clear();
  m_LimbPoses.Clear();
  m_bLimbsSetup = false;

  m_NextImpulse = {};
}

void ezPxRagdollComponent::SetDisableGravity(bool b)
{
  if (m_bDisableGravity == b)
    return;

  m_bDisableGravity = b;

  if (m_pPxRootBody != nullptr)
  {
    EZ_PX_WRITE_LOCK(*(m_pPxAggregate->getScene()));

    for (auto& limb : m_Limbs)
    {
      if (limb.m_pPxBody == nullptr)
        continue;

      limb.m_pPxBody->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, m_bDisableGravity);
    }

    if (!m_bDisableGravity)
    {
      WakeUp();
    }
  }
}

void ezPxRagdollComponent::AddForceAtPos(ezMsgPhysicsAddForce& ref_msg)
{
  if (m_pPxRootBody != nullptr)
  {
    EZ_PX_WRITE_LOCK(*m_pPxAggregate->getScene());

    PxRigidBody* pBody = m_pPxRootBody;

    if (ref_msg.m_pInternalPhysicsActor != nullptr)
      pBody = reinterpret_cast<PxRigidBody*>(ref_msg.m_pInternalPhysicsActor);

    PxRigidBodyExt::addForceAtPos(*pBody, ezPxConversionUtils::ToVec3(ref_msg.m_vForce), ezPxConversionUtils::ToVec3(ref_msg.m_vGlobalPosition), PxForceMode::eFORCE);
  }
}

void ezPxRagdollComponent::AddImpulseAtPos(ezMsgPhysicsAddImpulse& ref_msg)
{
  EZ_ASSERT_DEV(!ref_msg.m_vImpulse.IsNaN() && !ref_msg.m_vGlobalPosition.IsNaN(), "ezMsgPhysicsAddImpulse contains invalid (NaN) impulse or position");

  if (ref_msg.m_vImpulse.GetLengthSquared() > m_NextImpulse.m_vImpulse.GetLengthSquared())
  {
    m_NextImpulse.m_vPos = ref_msg.m_vGlobalPosition;
    m_NextImpulse.m_vImpulse = ref_msg.m_vImpulse;
    m_NextImpulse.m_pRigidBody = static_cast<PxRigidDynamic*>(ref_msg.m_pInternalPhysicsActor);

    // if (m_NextImpulse.m_pRigidBody)
    //{
    //   EZ_ASSERT_DEBUG(ezStringUtils::IsEqual(m_NextImpulse.m_pRigidBody->getConcreteTypeName(), "PxRigidDynamic"), "Expected PxRigidDynamic, got {}", m_NextImpulse.m_pRigidBody->getConcreteTypeName());
    // }
  }
}

void ezPxRagdollComponent::ApplyImpulse()
{
  if (m_NextImpulse.m_vImpulse.IsZero())
    return;

  ezPhysXWorldModule* pPxModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
  EZ_PX_WRITE_LOCK(*pPxModule->GetPxScene());

  if (m_NextImpulse.m_pRigidBody == nullptr)
  {
    float fBestDist = ezMath::HighValue<float>();
    m_NextImpulse.m_pRigidBody = m_pPxRootBody;

    // search for the best limb to apply the impulse to
    for (const auto& limb : m_Limbs)
    {
      if (limb.m_pPxBody == nullptr)
        continue;

      const float fDistSqr = (ezPxConversionUtils::ToVec3(limb.m_pPxBody->getGlobalPose().p) - m_NextImpulse.m_vPos).GetLengthSquared();

      if (fDistSqr < fBestDist)
      {
        fBestDist = fDistSqr;
        m_NextImpulse.m_pRigidBody = limb.m_pPxBody;
      }
    }
  }

  PxRigidBodyExt::addForceAtPos(*m_NextImpulse.m_pRigidBody, ezPxConversionUtils::ToVec3(m_NextImpulse.m_vImpulse), ezPxConversionUtils::ToVec3(m_NextImpulse.m_vPos), PxForceMode::eIMPULSE);

  m_NextImpulse = {};
}

void ezPxRagdollComponent::OnAnimationPoseProposal(ezMsgAnimationPoseProposal& ref_msg)
{
  // if (!m_bShapesCreated)
  //   return;

  // msg.m_bContinueAnimating = false;

  // ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
  // EZ_PX_WRITE_LOCK(*pModule->GetPxScene());

  // for (ezUInt32 i = 0; i < m_ArticulationLinks.GetCount(); ++i)
  //{
  //   if (m_ArticulationLinks[i].m_pLink == nullptr)
  //   {
  //     // no need to do anything, just pass the original pose through
  //   }
  //   else
  //   {
  //     if (PxArticulationJoint* pJoint = (PxArticulationJoint*)m_ArticulationLinks[i].m_pLink->getInboundJoint())
  //     {
  //       ezQuat rot;
  //       rot.SetIdentity();

  //      pJoint->setDriveType(PxArticulationJointDriveType::eTARGET);
  //      pJoint->setTargetOrientation(ezPxConversionUtils::ToQuat(rot));
  //      pJoint->setStiffness(100);
  //      //pJoint->setTargetVelocity(PxVec3(1, 1, 1));
  //    }
  //  }
  //}
}

void ezPxRagdollComponent::OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& ref_poseMsg)
{
  if (!IsActiveAndSimulating())
    return;

  if (m_Start == ezPxRagdollStart::Wait)
    return;

  ref_poseMsg.m_bContinueAnimating = false; // TODO: change this

  if (m_bLimbsSetup)
  {
    // TODO: if at some point we can layer ragdolls with detail animations, we should
    // take poses for all bones for which there are no shapes (link == null) -> to animate leafs (fingers and such)
    return;
  }

  m_LimbPoses = ref_poseMsg.m_ModelTransforms;

  SetupLimbs(ref_poseMsg);
}

void ezPxRagdollComponent::OnRetrieveBoneState(ezMsgRetrieveBoneState& ref_msg) const
{
  if (!m_bLimbsSetup)
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);
  const auto& skeleton = pSkeleton->GetDescriptor().m_Skeleton;

  for (ezUInt16 uiJointIdx = 0; uiJointIdx < skeleton.GetJointCount(); ++uiJointIdx)
  {
    ezMat4 mJoint = m_LimbPoses[uiJointIdx];

    const auto& joint = skeleton.GetJointByIndex(uiJointIdx);
    const ezUInt16 uiParentIdx = joint.GetParentIndex();
    if (uiParentIdx != ezInvalidJointIndex)
    {
      // remove the parent transform to get the pure local transform
      const ezMat4 mParent = m_LimbPoses[uiParentIdx].GetInverse();

      mJoint = mParent * mJoint;
    }

    auto& t = ref_msg.m_BoneTransforms[joint.GetName().GetString()];
    t.m_vPosition = mJoint.GetTranslationVector();
    t.m_qRotation.ReconstructFromMat4(mJoint);
    t.m_vScale.Set(1.0f);
  }
}

#if JOINT_DEBUG_DRAW
static void AddLine(ezHybridArray<ezDebugRenderer::Line, 32>& lines, const ezTransform& transform, const ezVec3& dir, const ezColor& color)
{
  auto& l = lines.ExpandAndGetRef();
  l.m_start = transform.m_vPosition;
  l.m_end = transform.TransformPosition(dir);
  l.m_startColor = color;
  l.m_endColor = color;
}
#endif

void ezPxRagdollComponent::RetrievePhysicsPose()
{
  if (m_pPxRootBody == nullptr)
    return;

  ezPhysXWorldModule* pPxModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
  EZ_PX_READ_LOCK(*pPxModule->GetPxScene());

  if (IsSleeping())
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);

  const ezTransform rootTransform = pSkeleton->GetDescriptor().m_RootTransform;
  const ezMat4 invRootTransform = rootTransform.GetAsMat4().GetInverse();

  ezMat4 scale = ezMat4::MakeScaling(rootTransform.m_vScale);

  ezMsgAnimationPoseUpdated poseMsg;
  poseMsg.m_ModelTransforms = m_LimbPoses;
  poseMsg.m_pRootTransform = &rootTransform;
  poseMsg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;

  const ezTransform newRootTransform = ezPxConversionUtils::ToTransform(m_pPxRootBody->getGlobalPose());
  GetOwner()->SetGlobalTransform(newRootTransform * m_RootBodyLocalTransform.GetInverse());

  const ezMat4 mInv = invRootTransform * m_RootBodyLocalTransform.GetAsMat4() * newRootTransform.GetInverse().GetAsMat4();

#if JOINT_DEBUG_DRAW
  ezHybridArray<ezDebugRenderer::Line, 32> lines;
#endif

  for (ezUInt32 uiLimbIdx = 0; uiLimbIdx < m_Limbs.GetCount(); ++uiLimbIdx)
  {
    if (m_Limbs[uiLimbIdx].m_pPxBody == nullptr)
    {
      // no need to do anything, just pass the original pose through
      continue;
    }

    const ezTransform limbGlobalPose = ezPxConversionUtils::ToTransform(m_Limbs[uiLimbIdx].m_pPxBody->getGlobalPose());

    m_LimbPoses[uiLimbIdx] = (mInv * limbGlobalPose.GetAsMat4()) * scale;

#if JOINT_DEBUG_DRAW
    if (auto joint = m_ArticulationLinks[uiLimbIdx]->getInboundJoint())
    {
      // joint in parent frame
      {
        const ezTransform jointParentPose = ezPxConversionUtils::ToTransform(joint->getParentArticulationLink().getGlobalPose());
        const ezTransform jointLocalPose = ezPxConversionUtils::ToTransform(joint->getParentPose());

        ezTransform jointGlobalPose;
        jointGlobalPose = ezTransform::MakeGlobalTransform(jointParentPose, jointLocalPose);

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
        const ezTransform jointChildPose = ezPxConversionUtils::ToTransform(m_ArticulationLinks[uiLimbIdx]->getGlobalPose());
        const ezTransform jointLocalPose = ezPxConversionUtils::ToTransform(joint->getChildPose());

        ezTransform jointGlobalPose;
        jointGlobalPose = ezTransform::MakeGlobalTransform(jointChildPose, jointLocalPose);

        const float s = 0.05f;

        AddLine(lines, jointGlobalPose, ezVec3(s, 0, 0), ezColor::Red);
        AddLine(lines, jointGlobalPose, ezVec3(0, s, 0), ezColor::Lime);
        AddLine(lines, jointGlobalPose, ezVec3(0, 0, s), ezColor::Blue);
      }
    }
#endif
  }

#if JOINT_DEBUG_DRAW
  ezDebugRenderer::DrawLines(GetWorld(), lines, ezColor::White);
#endif

  GetOwner()->SendMessage(poseMsg);
}


void ezPxRagdollComponent::SetupLimbsFromBindPose()
{
  if (m_bLimbsSetup)
    return;

  if (!EnsureSkeletonIsKnown())
  {
    ezLog::Error("No skeleton available to ragdoll.");
    return;
  }

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);
  const auto& desc = pSkeleton->GetDescriptor();

  m_LimbPoses.SetCountUninitialized(desc.m_Skeleton.GetJointCount());

  auto getBone = [&](ezUInt16 i, auto f) -> ezMat4
  {
    const auto& j = desc.m_Skeleton.GetJointByIndex(i);
    const ezMat4 bm = j.GetRestPoseLocalTransform().GetAsMat4();

    if (j.GetParentIndex() != ezInvalidJointIndex)
    {
      const ezMat4 pbm = f(j.GetParentIndex(), f);

      return pbm * bm;
    }

    return bm;
  };

  for (ezUInt16 i = 0; i < static_cast<ezUInt16>(m_LimbPoses.GetCount()); ++i)
  {
    m_LimbPoses[i] = getBone(i, getBone);
  }

  ezMsgAnimationPoseUpdated msg;
  msg.m_pRootTransform = &desc.m_RootTransform;
  msg.m_pSkeleton = &desc.m_Skeleton;
  msg.m_ModelTransforms = m_LimbPoses;

  SetupLimbs(msg);
}

void ezPxRagdollComponent::CreateConstraints()
{
  if (m_Constraints.IsEmpty())
    return;

  ezPhysXWorldModule* pPxModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
  EZ_PX_WRITE_LOCK(*pPxModule->GetPxScene());

  const ezTransform ownTransform = GetOwner()->GetGlobalTransform();

  for (auto& constraint : m_Constraints)
  {
    for (const auto& limb : m_Limbs)
    {
      if (limb.m_sName != constraint.m_sBone)
        continue;

      const ezTransform pos(ownTransform.TransformPosition(constraint.m_vRelativePosition));

      auto pJoint = PxSphericalJointCreate(*(ezPhysX::GetSingleton()->GetPhysXAPI()), nullptr, ezPxConversionUtils::ToTransform(pos), limb.m_pPxBody, ezPxConversionUtils::ToTransform(ezTransform::MakeIdentity()));

      pJoint->setConstraintFlag(physx::PxConstraintFlag::ePROJECTION, true);
      pJoint->setProjectionLinearTolerance(0.05f);

      break;
    }
  }
}

void ezPxRagdollComponent::SetupPxBasics(physx::PxPhysics* pPxApi, ezPhysXWorldModule* pPxModule)
{
  EZ_ASSERT_DEBUG(m_uiPxUserDataIndex == ezInvalidIndex, "Can't initialize twice.");

  m_uiPxShapeID = pPxModule->CreateShapeId();
  m_uiPxUserDataIndex = pPxModule->AllocateUserData(m_pPxUserData);
  m_pPxUserData->Init(this);
}

void ezPxRagdollComponent::SetupLimbs(const ezMsgAnimationPoseUpdated& pose)
{
  EZ_ASSERT_DEBUG(!m_bLimbsSetup, "Limbs already set up.");

  m_bLimbsSetup = true;

  if (!EnsureSkeletonIsKnown())
  {
    ezLog::Error("No skeleton available to ragdoll.");
    return;
  }

  PxPhysics* pPxApi = ezPhysX::GetSingleton()->GetPhysXAPI();
  ezPhysXWorldModule* pPxModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
  EZ_PX_WRITE_LOCK(*pPxModule->GetPxScene());

  SetupPxBasics(pPxApi, pPxModule);

  ezResourceLock<ezSkeletonResource> pSkeletonResource(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);

  // allocate the limbs array
  m_Limbs.SetCount(pose.m_ModelTransforms.GetCount());

  // we don't know yet how many limbs actually need rigid bodies (many could be skipped, like finger bones), but 64 is the hard upper limit supported by PhysX
  m_pPxAggregate = pPxApi->createAggregate(ezMath::Min(64u, m_Limbs.GetCount()), m_bSelfCollision);

  SetupLimbBodiesAndGeometry(pSkeletonResource.GetPointer(), pose);

  SetupLimbJoints(pSkeletonResource.GetPointer());

  FinishSetupLimbs();

  if (m_pPxRootBody == nullptr)
  {
    m_pPxAggregate->release();
    m_pPxAggregate = nullptr;
    return;
  }

  pPxModule->GetPxScene()->addAggregate(*m_pPxAggregate);

  CreateConstraints();
}

void ezPxRagdollComponent::SetupLimbBodiesAndGeometry(const ezSkeletonResource* pSkeleton, const ezMsgAnimationPoseUpdated& pose)
{
  PxPhysics* pPxApi = ezPhysX::GetSingleton()->GetPhysXAPI();

  ezMap<ezUInt16, LimbConfig> limbStructure(ezFrameAllocator::GetCurrentAllocator());
  limbStructure.FindOrAdd(ezInvalidJointIndex); // dummy root link

  const auto& skeleton = pSkeleton->GetDescriptor().m_Skeleton;
  const auto srcBoneDir = pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection;
  physx::PxVec3 vPxVelocity = ezPxConversionUtils::ToVec3(GetOwner()->GetLinearVelocity());

  for (const auto& geo : pSkeleton->GetDescriptor().m_Geometry)
  {
    if (geo.m_Type == ezSkeletonJointGeometryType::None)
      continue;

    const ezSkeletonJoint& thisJoint = skeleton.GetJointByIndex(geo.m_uiAttachedToJoint);

    ezUInt16 uiParentJointIdx = thisJoint.GetParentIndex();

    // find the parent joint that is also part of the ragdoll
    while (!limbStructure.Contains(uiParentJointIdx))
    {
      uiParentJointIdx = skeleton.GetJointByIndex(uiParentJointIdx).GetParentIndex();
    }

    const auto& parentLimb = limbStructure[uiParentJointIdx];
    auto& thisLimb = limbStructure[geo.m_uiAttachedToJoint];

    if (thisLimb.m_pPxBody == nullptr)
    {
      ComputeLimbGlobalTransform(thisLimb.m_GlobalTransform, pose, geo.m_uiAttachedToJoint);
      CreateLimbBody(pPxApi, parentLimb, thisLimb);

      EZ_ASSERT_DEBUG(thisLimb.m_pPxBody != nullptr, "Limb body wasn't set up.");
      thisLimb.m_pPxBody->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, m_bDisableGravity);
      thisLimb.m_pPxBody->setLinearVelocity(vPxVelocity, false);
      thisLimb.m_pPxBody->userData = m_pPxUserData;

      if (thisLimb.m_pPxBody->getType() == PxActorType::eRIGID_DYNAMIC)
      {
        m_pPxAggregate->addActor(*thisLimb.m_pPxBody);
      }

      m_Limbs[geo.m_uiAttachedToJoint].m_pPxBody = thisLimb.m_pPxBody;
      m_Limbs[geo.m_uiAttachedToJoint].m_sName = thisJoint.GetName();

      if (m_pPxRootBody == nullptr)
      {
        m_pPxRootBody = thisLimb.m_pPxBody;
        m_RootBodyLocalTransform = ezTransform::MakeLocalTransform(GetOwner()->GetGlobalTransform(), thisLimb.m_GlobalTransform);
      }
    }

    AddLimbGeometry(srcBoneDir, *thisLimb.m_pPxBody, geo);

    // TODO mass distribution
    {
      float fMass = 4.0f;

      if (parentLimb.m_pPxBody)
      {
        fMass = 0.9f * parentLimb.m_pPxBody->getMass();
      }

      PxRigidBodyExt::setMassAndUpdateInertia(*thisLimb.m_pPxBody, fMass);
    }
  }
}

void ezPxRagdollComponent::SetupLimbJoints(const ezSkeletonResource* pSkeleton)
{
  PxPhysics* pPxApi = ezPhysX::GetSingleton()->GetPhysXAPI();

  const auto srcBoneDir = pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection;

  // the main direction of PhysX bones is +X (for bone limits and such)
  // therefore the main direction of the source bones has to be adjusted
  const ezQuat qBoneDirAdjustment = -ezBasisAxis::GetBasisRotation(srcBoneDir, ezBasisAxis::PositiveX);

  const auto& skeleton = pSkeleton->GetDescriptor().m_Skeleton;

  for (ezUInt16 uiLimbIdx = 0; uiLimbIdx < static_cast<ezUInt16>(m_Limbs.GetCount()); ++uiLimbIdx)
  {
    const auto& thisLimb = m_Limbs[uiLimbIdx];

    if (thisLimb.m_pPxBody == nullptr)
      continue;

    const ezSkeletonJoint& thisJoint = skeleton.GetJointByIndex(uiLimbIdx);
    ezUInt16 uiParentLimb = thisJoint.GetParentIndex();
    while (uiParentLimb != ezInvalidJointIndex && m_Limbs[uiParentLimb].m_pPxBody == nullptr)
    {
      uiParentLimb = skeleton.GetJointByIndex(uiParentLimb).GetParentIndex();
    }

    if (uiParentLimb == ezInvalidJointIndex)
      continue;

    const auto& parentLimb = m_Limbs[uiParentLimb];

    const ezTransform parentTransform = ezPxConversionUtils::ToTransform(parentLimb.m_pPxBody->getGlobalPose());
    const ezTransform thisTransform = ezPxConversionUtils::ToTransform(thisLimb.m_pPxBody->getGlobalPose());

    ezTransform parentJointFrame;
    parentJointFrame = ezTransform::MakeLocalTransform(parentTransform, thisTransform); // TODO this should just be the constant local offset from child to parent (rotation is overridden anyway, position should never differ)
    parentJointFrame.m_qRotation = thisJoint.GetLocalOrientation() * qBoneDirAdjustment;

    ezTransform thisJointFrame;
    thisJointFrame.SetIdentity();
    thisJointFrame.m_qRotation = qBoneDirAdjustment;

    CreateLimbJoint(pPxApi, thisJoint, parentLimb.m_pPxBody, parentJointFrame, thisLimb.m_pPxBody, thisJointFrame);
  }
}

void ezPxRagdollComponent::ComputeLimbModelSpaceTransform(ezTransform& transform, const ezMsgAnimationPoseUpdated& pose, ezUInt32 uiIndex)
{
  ezMat4 mFullTransform;
  pose.ComputeFullBoneTransform(uiIndex, mFullTransform, transform.m_qRotation);

  transform.m_vScale.Set(1);
  transform.m_vPosition = mFullTransform.GetTranslationVector();
}


void ezPxRagdollComponent::ComputeLimbGlobalTransform(ezTransform& transform, const ezMsgAnimationPoseUpdated& pose, ezUInt32 uiIndex)
{
  ezTransform local;
  ComputeLimbModelSpaceTransform(local, pose, uiIndex);
  transform = ezTransform::MakeGlobalTransform(GetOwner()->GetGlobalTransform(), local);
}

void ezPxRagdollComponent::AddLimbGeometry(ezBasisAxis::Enum srcBoneDir, physx::PxRigidActor& actor, const ezSkeletonResourceGeometry& geo)
{
  EZ_ASSERT_NOT_IMPLEMENTED;

  // PxFilterData pxFilterData = ezPhysX::CreateFilterData(geo.m_uiCollisionLayer, m_uiPxShapeID);

  // physx::PxMaterial* pxMaterial = nullptr;
  // if (geo.m_hSurface.IsValid())
  //{
  //   ezResourceLock<ezSurfaceResource> pSurface(geo.m_hSurface, ezResourceAcquireMode::BlockTillLoaded);

  //  if (pSurface->m_pPhysicsMaterialPhysX != nullptr)
  //  {
  //    pxMaterial = static_cast<physx::PxMaterial*>(pSurface->m_pPhysicsMaterialPhysX);
  //  }
  //}
  // else
  //  pxMaterial = ezPhysX::GetSingleton()->GetDefaultMaterial();

  // PxShape* pShape = nullptr;

  // const ezQuat qBoneDirAdjustment = ezBasisAxis::GetBasisRotation(ezBasisAxis::PositiveX, srcBoneDir);

  // const ezQuat qFinalBoneRot = /*boneRot **/ qBoneDirAdjustment;

  // ezTransform st;
  // st.SetIdentity();
  // st.m_vPosition = /*boneTrans.GetTranslationVector() +*/ qFinalBoneRot * geo.m_Transform.m_vPosition;
  // st.m_qRotation = qFinalBoneRot * geo.m_Transform.m_qRotation;

  // if (geo.m_Type == ezSkeletonJointGeometryType::Sphere)
  //{
  //   PxSphereGeometry shape(geo.m_Transform.m_vScale.z);
  //   pShape = PxRigidActorExt::createExclusiveShape(actor, shape, *pxMaterial);
  // }
  // else if (geo.m_Type == ezSkeletonJointGeometryType::Box)
  //{
  //   ezVec3 ext;
  //   ext.x = geo.m_Transform.m_vScale.x * 0.5f;
  //   ext.y = geo.m_Transform.m_vScale.y * 0.5f;
  //   ext.z = geo.m_Transform.m_vScale.z * 0.5f;

  //  // TODO: if offset desired
  //  st.m_vPosition += qFinalBoneRot * ezVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);

  //  PxBoxGeometry shape(ext.x, ext.y, ext.z);
  //  pShape = PxRigidActorExt::createExclusiveShape(actor, shape, *pxMaterial);
  //}
  // else if (geo.m_Type == ezSkeletonJointGeometryType::Capsule)
  //{
  //  PxCapsuleGeometry shape(geo.m_Transform.m_vScale.z, geo.m_Transform.m_vScale.x * 0.5f);
  //  pShape = PxRigidActorExt::createExclusiveShape(actor, shape, *pxMaterial);

  //  // TODO: if offset desired
  //  st.m_vPosition += qFinalBoneRot * ezVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);
  //}
  // else
  //{
  //  EZ_ASSERT_NOT_IMPLEMENTED;
  //}

  // pShape->setLocalPose(ezPxConversionUtils::ToTransform(st));
  // pShape->setSimulationFilterData(pxFilterData);
  // pShape->setQueryFilterData(pxFilterData);
  // pShape->userData = m_pPxUserData;
}
