#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Constraints/ConeConstraint.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
#include <Jolt/Physics/Ragdoll/Ragdoll.h>
#include <Jolt/Skeleton/Skeleton.h>
#include <JoltPlugin/Components/JoltRagdollComponent.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <JoltPlugin/Utilities/JoltUserData.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonPoseComponent.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Debug/DebugRenderer.h>

/* TODO
 * max force clamping ?
 * mass distribution
 * communication with anim controller
 * drive to pose
 * shape scale
 */

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezJoltRagdollStart, 1)
  EZ_ENUM_CONSTANTS(ezJoltRagdollStart::BindPose, ezJoltRagdollStart::WaitForPose, ezJoltRagdollStart::Wait)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezJoltRagdollConstraint, 1, ezRTTIDefaultAllocator<ezJoltRagdollConstraint>)
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

EZ_BEGIN_COMPONENT_TYPE(ezJoltRagdollComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("SelfCollision", m_bSelfCollision),
    EZ_ENUM_MEMBER_PROPERTY("Start", ezJoltRagdollStart, m_Start),
    EZ_ACCESSOR_PROPERTY("GravityFactor", GetGravityFactor, SetGravityFactor)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("Stiffness", m_fStiffness)->AddAttributes(new ezDefaultValueAttribute(10.0f)),
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
    new ezCategoryAttribute("Physics/Jolt/Animation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

ezResult ezJoltRagdollConstraint::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream << m_sBone;
  inout_stream << m_vRelativePosition;
  return EZ_SUCCESS;
}

ezResult ezJoltRagdollConstraint::Deserialize(ezStreamReader& inout_stream)
{
  inout_stream >> m_sBone;
  inout_stream >> m_vRelativePosition;
  return EZ_SUCCESS;
}

ezJoltRagdollComponent::ezJoltRagdollComponent() = default;
ezJoltRagdollComponent::~ezJoltRagdollComponent() = default;

void ezJoltRagdollComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_Start;
  s << m_fGravityFactor;
  s << m_bSelfCollision;
  s << m_uiCollisionLayer;
  s.WriteArray(m_Constraints).AssertSuccess();
}

void ezJoltRagdollComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_Start;
  s >> m_fGravityFactor;
  s >> m_bSelfCollision;
  s >> m_uiCollisionLayer;
  s.ReadArray(m_Constraints).AssertSuccess();
}

void ezJoltRagdollComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_Start == ezJoltRagdollStart::BindPose)
  {
    SetupLimbsFromBindPose();
  }
}

void ezJoltRagdollComponent::OnDeactivated()
{
  ClearPhysicsObjects();

  SUPER::OnDeactivated();
}

void ezJoltRagdollComponent::Update()
{
  if (!m_bLimbsSetup)
    return;

  RetrievePhysicsPose();

  ApplyImpulse();
}

bool ezJoltRagdollComponent::EnsureSkeletonIsKnown()
{
  if (!m_hSkeleton.IsValid())
  {
    ezMsgQueryAnimationSkeleton msg;
    GetOwner()->SendMessage(msg);
    m_hSkeleton = msg.m_hSkeleton;
  }

  return m_hSkeleton.IsValid();
}

void ezJoltRagdollComponent::ClearPhysicsObjects()
{
  if (m_pRagdoll)
  {
    m_pRagdoll->RemoveFromPhysicsSystem();
    m_pRagdoll->Release();
    m_pRagdoll = nullptr;
  }

  if (m_pRagdollSettings)
  {
    m_pRagdollSettings->Release();
    m_pRagdollSettings = nullptr;
  }

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
  pModule->DeallocateUserData(m_uiJoltUserDataIndex);
  pModule->DeleteObjectFilterID(m_uiObjectFilterID);

  m_pJoltUserData = nullptr;

  m_Limbs.Clear();
  m_LimbPoses.Clear();
  m_bLimbsSetup = false;

  m_NextImpulse = {};
}

void ezJoltRagdollComponent::SetGravityFactor(float fFactor)
{
  if (m_fGravityFactor == fFactor)
    return;

  m_fGravityFactor = fFactor;

  if (!m_pRagdoll)
    return;

  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();

  for (ezUInt32 i = 0; i < m_pRagdoll->GetBodyCount(); ++i)
  {
    pModule->GetJoltSystem()->GetBodyInterface().SetGravityFactor(m_pRagdoll->GetBodyID(i), m_fGravityFactor);
  }

  m_pRagdoll->Activate();
}

void ezJoltRagdollComponent::AddForceAtPos(ezMsgPhysicsAddForce& ref_msg)
{
  // if (m_pPxAggregate != nullptr)
  //{
  //   EZ_PX_WRITE_LOCK(*m_pPxAggregate->getScene());

  //  PxRigidBody* pBody = m_pPxRootBody;

  //  if (msg.m_pInternalPhysicsActor != nullptr)
  //    pBody = reinterpret_cast<PxRigidBody*>(msg.m_pInternalPhysicsActor);

  //  PxRigidBodyExt::addForceAtPos(*pBody, ezJoltConversionUtils::ToVec3(msg.m_vForce), ezJoltConversionUtils::ToVec3(msg.m_vGlobalPosition), PxForceMode::eFORCE);
  //}
}

void ezJoltRagdollComponent::AddImpulseAtPos(ezMsgPhysicsAddImpulse& ref_msg)
{
  // EZ_ASSERT_DEV(!msg.m_vImpulse.IsNaN() && !msg.m_vGlobalPosition.IsNaN(), "ezMsgPhysicsAddImpulse contains invalid (NaN) impulse or position");

  // if (msg.m_vImpulse.GetLengthSquared() > m_NextImpulse.m_vImpulse.GetLengthSquared())
  //{
  //   m_NextImpulse.m_vPos = msg.m_vGlobalPosition;
  //   m_NextImpulse.m_vImpulse = msg.m_vImpulse;
  //   m_NextImpulse.m_pRigidBody = static_cast<PxRigidDynamic*>(msg.m_pInternalPhysicsActor);

  //  //if (m_NextImpulse.m_pRigidBody)
  //  //{
  //  //  EZ_ASSERT_DEBUG(ezStringUtils::IsEqual(m_NextImpulse.m_pRigidBody->getConcreteTypeName(), "PxRigidDynamic"), "Expected PxRigidDynamic, got {}", m_NextImpulse.m_pRigidBody->getConcreteTypeName());
  //  //}
  //}
}

void ezJoltRagdollComponent::ApplyImpulse()
{
  // if (m_NextImpulse.m_vImpulse.IsZero())
  //   return;

  // ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
  // EZ_PX_WRITE_LOCK(*pModule->GetPxScene());

  // if (m_NextImpulse.m_pRigidBody == nullptr)
  //{
  //   float fBestDist = ezMath::HighValue<float>();
  //   m_NextImpulse.m_pRigidBody = m_pPxRootBody;

  //  // search for the best limb to apply the impulse to
  //  for (const auto& limb : m_Limbs)
  //  {
  //    if (limb.m_pPxBody == nullptr)
  //      continue;

  //    const float fDistSqr = (ezJoltConversionUtils::ToVec3(limb.m_pPxBody->getGlobalPose().p) - m_NextImpulse.m_vPos).GetLengthSquared();

  //    if (fDistSqr < fBestDist)
  //    {
  //      fBestDist = fDistSqr;
  //      m_NextImpulse.m_pRigidBody = limb.m_pPxBody;
  //    }
  //  }
  //}

  // JoltRigidBodyExt::addForceAtPos(*m_NextImpulse.m_pRigidBody, ezJoltConversionUtils::ToVec3(m_NextImpulse.m_vImpulse), ezJoltConversionUtils::ToVec3(m_NextImpulse.m_vPos), PxForceMode::eIMPULSE);

  // m_NextImpulse = {};
}

void ezJoltRagdollComponent::OnAnimationPoseProposal(ezMsgAnimationPoseProposal& ref_msg)
{
  // if (!m_bShapesCreated)
  //   return;

  // msg.m_bContinueAnimating = false;

  // ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
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
  //      pJoint->setTargetOrientation(ezJoltConversionUtils::ToQuat(rot));
  //      pJoint->setStiffness(100);
  //      //pJoint->setTargetVelocity(PxVec3(1, 1, 1));
  //    }
  //  }
  //}
}

void ezJoltRagdollComponent::OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& ref_poseMsg)
{
  if (!IsActiveAndSimulating())
    return;

  if (m_Start == ezJoltRagdollStart::Wait)
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

void ezJoltRagdollComponent::OnRetrieveBoneState(ezMsgRetrieveBoneState& ref_msg) const
{
  if (!m_bLimbsSetup)
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);
  const auto& skeleton = pSkeleton->GetDescriptor().m_Skeleton;

  for (ezUInt32 uiJointIdx = 0; uiJointIdx < skeleton.GetJointCount(); ++uiJointIdx)
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

void ezJoltRagdollComponent::RetrievePhysicsPose()
{
  if (!m_bLimbsSetup)
    return;

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();

  if (IsSleeping())
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);

  const ezTransform rootTransform = pSkeleton->GetDescriptor().m_RootTransform;
  const ezMat4 invRootTransform = rootTransform.GetAsMat4().GetInverse();

  ezMat4 scale;
  scale.SetScalingMatrix(rootTransform.m_vScale);

  ezMsgAnimationPoseUpdated poseMsg;
  poseMsg.m_ModelTransforms = m_LimbPoses;
  poseMsg.m_pRootTransform = &rootTransform;
  poseMsg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;

  JPH::Vec3 joltRootPos;
  JPH::Quat joltRootRot;
  m_pRagdoll->GetRootTransform(joltRootPos, joltRootRot);

  const ezTransform newRootTransform = ezJoltConversionUtils::ToTransform(joltRootPos, joltRootRot);
  GetOwner()->SetGlobalTransform(newRootTransform * m_RootBodyLocalTransform.GetInverse());

  const ezMat4 mInv = invRootTransform * m_RootBodyLocalTransform.GetAsMat4() * newRootTransform.GetInverse().GetAsMat4();

#if JOINT_DEBUG_DRAW
  ezHybridArray<ezDebugRenderer::Line, 32> lines;
#endif

  for (ezUInt32 uiLimbIdx = 0; uiLimbIdx < m_Limbs.GetCount(); ++uiLimbIdx)
  {
    if (m_Limbs[uiLimbIdx].m_pBodyDesc == nullptr)
    {
      // no need to do anything, just pass the original pose through
      continue;
    }

    const JPH::BodyID bodyId = m_pRagdoll->GetBodyID(m_Limbs[uiLimbIdx].m_uiPartIndex);
    JPH::BodyLockRead bodyRead(pModule->GetJoltSystem()->GetBodyLockInterface(), bodyId);

    const ezTransform limbGlobalPose = ezJoltConversionUtils::ToTransform(bodyRead.GetBody().GetPosition(), bodyRead.GetBody().GetRotation());

    m_LimbPoses[uiLimbIdx] = (mInv * limbGlobalPose.GetAsMat4()) * scale;

#if JOINT_DEBUG_DRAW
    if (auto joint = m_ArticulationLinks[uiLimbIdx]->getInboundJoint())
    {
      // joint in parent frame
      {
        const ezTransform jointParentPose = ezJoltConversionUtils::ToTransform(joint->getParentArticulationLink().getGlobalPose());
        const ezTransform jointLocalPose = ezJoltConversionUtils::ToTransform(joint->getParentPose());

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
        const ezTransform jointChildPose = ezJoltConversionUtils::ToTransform(m_ArticulationLinks[uiLimbIdx]->getGlobalPose());
        const ezTransform jointLocalPose = ezJoltConversionUtils::ToTransform(joint->getChildPose());

        ezTransform jointGlobalPose;
        jointGlobalPose.SetGlobalTransform(jointChildPose, jointLocalPose);

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

void ezJoltRagdollComponent::WakeUp()
{
  m_pRagdoll->Activate();
}

bool ezJoltRagdollComponent::IsSleeping() const
{
  const ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();

  JPH::BodyLockRead lock(pModule->GetJoltSystem()->GetBodyLockInterface(), m_pRagdoll->GetBodyID(0));

  if (!lock.Succeeded())
    return true;

  return !lock.GetBody().IsActive();
}

void ezJoltRagdollComponent::SetupLimbsFromBindPose()
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

  for (ezUInt32 i = 0; i < m_LimbPoses.GetCount(); ++i)
  {
    m_LimbPoses[i] = getBone(i, getBone);
  }

  ezMsgAnimationPoseUpdated msg;
  msg.m_pRootTransform = &desc.m_RootTransform;
  msg.m_pSkeleton = &desc.m_Skeleton;
  msg.m_ModelTransforms = m_LimbPoses;

  SetupLimbs(msg);
}

void ezJoltRagdollComponent::CreateConstraints()
{
  // if (m_Constraints.IsEmpty())
  //   return;

  // ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();

  // const ezTransform ownTransform = GetOwner()->GetGlobalTransform();

  // for (auto& constraint : m_Constraints)
  //{
  //   for (const auto& limb : m_Limbs)
  //   {
  //     if (limb.m_sName != constraint.m_sBone)
  //       continue;

  //    const ezTransform pos(ownTransform.TransformPosition(constraint.m_vRelativePosition));

  //    auto pJoint = PxSphericalJointCreate(*(ezJolt::GetSingleton()->GetJoltAPI()), nullptr, ezJoltConversionUtils::ToTransform(pos), limb.m_pPxBody, ezJoltConversionUtils::ToTransform(ezTransform::IdentityTransform()));

  //    pJoint->setConstraintFlag(physx::PxConstraintFlag::ePROJECTION, true);
  //    pJoint->setProjectionLinearTolerance(0.05f);

  //    break;
  //  }
  //}
}

void ezJoltRagdollComponent::SetupJoltBasics(/*physx::PxPhysics* pPxApi,*/ ezJoltWorldModule* pModule)
{
  EZ_ASSERT_DEBUG(m_uiJoltUserDataIndex == ezInvalidIndex, "Can't initialize twice.");

  m_uiObjectFilterID = pModule->CreateObjectFilterID();
  m_uiJoltUserDataIndex = pModule->AllocateUserData(m_pJoltUserData);
  m_pJoltUserData->Init(this);
}

void ezJoltRagdollComponent::FinishSetupLimbs()
{
  m_pRagdollSettings->Stabilize();

  // if (m_bSelfCollision)
  // TODO: use GetGroupFilterIgnoreSame when m_bSelfCollision is false (see ropes)
  {
    m_pRagdollSettings->DisableParentChildCollisions();
  }

  static JPH::CollisionGroup::GroupID s_iRagdollCounter = 0;
  ++s_iRagdollCounter;

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
  m_pRagdoll = m_pRagdollSettings->CreateRagdoll(s_iRagdollCounter, reinterpret_cast<ezUInt64>(m_pJoltUserData), pModule->GetJoltSystem());

  m_pRagdoll->AddRef();
  m_pRagdoll->AddToPhysicsSystem(JPH::EActivation::Activate);
}

void ezJoltRagdollComponent::SetupLimbs(const ezMsgAnimationPoseUpdated& pose)
{
  m_bLimbsSetup = true;

  if (!EnsureSkeletonIsKnown())
  {
    ezLog::Error("No skeleton available to ragdoll.");
    return;
  }

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();

  SetupJoltBasics(pModule);

  ezResourceLock<ezSkeletonResource> pSkeletonResource(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);

  // allocate the limbs array
  m_Limbs.SetCount(pose.m_ModelTransforms.GetCount());

  m_pRagdollSettings = new JPH::RagdollSettings();
  m_pRagdollSettings->AddRef();
  m_pRagdollSettings->mParts.reserve(pSkeletonResource->GetDescriptor().m_Skeleton.GetJointCount());
  m_pRagdollSettings->mSkeleton = new JPH::Skeleton(); // TODO: share this in the resource
  m_pRagdollSettings->mSkeleton->GetJoints().reserve(m_pRagdollSettings->mParts.size());

  SetupLimbBodiesAndGeometry(pSkeletonResource.GetPointer(), pose);

  SetupLimbJoints(pSkeletonResource.GetPointer());

  FinishSetupLimbs();

  CreateConstraints();
}

void ezJoltRagdollComponent::SetupLimbBodiesAndGeometry(const ezSkeletonResource* pSkeleton, const ezMsgAnimationPoseUpdated& pose)
{
  ezMap<ezUInt16, LimbConfig> limbStructure(ezFrameAllocator::GetCurrentAllocator());
  limbStructure.FindOrAdd(ezInvalidJointIndex); // dummy root link

  const auto& skeleton = pSkeleton->GetDescriptor().m_Skeleton;
  const auto srcBoneDir = pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection;
  // physx::PxVec3 vPxVelocity = ezJoltConversionUtils::ToVec3(GetOwner()->GetVelocity());

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

    if (thisLimb.m_pBodyDesc == nullptr)
    {
      thisLimb.m_uiPartIndex = (ezUInt16)m_pRagdollSettings->mParts.size();
      m_pRagdollSettings->mParts.resize(m_pRagdollSettings->mParts.size() + 1);
      m_pRagdollSettings->mSkeleton->GetJoints().resize(m_pRagdollSettings->mParts.size());
      m_pRagdollSettings->mSkeleton->GetJoints().back().mName = thisJoint.GetName().GetData();
      m_pRagdollSettings->mSkeleton->GetJoints().back().mParentJointIndex = parentLimb.m_uiPartIndex == ezInvalidJointIndex ? -1 : parentLimb.m_uiPartIndex;

      if (parentLimb.m_uiPartIndex != ezInvalidJointIndex)
      {
        const ezSkeletonJoint& parentJoint = skeleton.GetJointByIndex(thisJoint.GetParentIndex());
        m_pRagdollSettings->mSkeleton->GetJoints().back().mParentName = parentJoint.GetName().GetData();
      }

      auto* pBodyDesc = &m_pRagdollSettings->mParts.back();
      thisLimb.m_pBodyDesc = pBodyDesc;

      ComputeLimbGlobalTransform(thisLimb.m_GlobalTransform, pose, geo.m_uiAttachedToJoint);
      CreateLimbBody(parentLimb, thisLimb);

      m_Limbs[geo.m_uiAttachedToJoint].m_uiPartIndex = thisLimb.m_uiPartIndex;
      m_Limbs[geo.m_uiAttachedToJoint].m_pBodyDesc = thisLimb.m_pBodyDesc; // TODO keep ?
      m_Limbs[geo.m_uiAttachedToJoint].m_sName = thisJoint.GetName();

      if (m_pRagdollSettings->mParts.size() == 1) // first body that was added
      {
        // m_pPxRootBody = thisLimb.m_pPxBody;
        m_RootBodyLocalTransform.SetLocalTransform(GetOwner()->GetGlobalTransform(), thisLimb.m_GlobalTransform);
      }
    }

    AddLimbGeometry(srcBoneDir, thisLimb, geo);

    // TODO mass distribution
    {
      float fMass = 4.0f;

      // if (parentLimb.m_pPxBody)
      //{
      //   fMass = 0.9f * parentLimb.m_pPxBody->getMass();
      // }

      // JoltRigidBodyExt::setMassAndUpdateInertia(*thisLimb.m_pPxBody, fMass);
    }
  }
}

void ezJoltRagdollComponent::SetupLimbJoints(const ezSkeletonResource* pSkeleton)
{
  const auto srcBoneDir = pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection;

  // the main direction of Jolt bones is +X (for bone limits and such)
  // therefore the main direction of the source bones has to be adjusted
  const ezQuat qBoneDirAdjustment = -ezBasisAxis::GetBasisRotation(srcBoneDir, ezBasisAxis::PositiveX);

  const auto& skeleton = pSkeleton->GetDescriptor().m_Skeleton;

  for (ezUInt32 uiLimbIdx = 0; uiLimbIdx < m_Limbs.GetCount(); ++uiLimbIdx)
  {
    const auto& thisLimb = m_Limbs[uiLimbIdx];

    if (thisLimb.m_pBodyDesc == nullptr)
      continue;

    const ezSkeletonJoint& thisJoint = skeleton.GetJointByIndex(uiLimbIdx);
    ezUInt16 uiParentLimb = thisJoint.GetParentIndex();
    while (uiParentLimb != ezInvalidJointIndex && m_Limbs[uiParentLimb].m_pBodyDesc == nullptr)
    {
      uiParentLimb = skeleton.GetJointByIndex(uiParentLimb).GetParentIndex();
    }

    if (uiParentLimb == ezInvalidJointIndex)
      continue;

    const auto& parentLimb = m_Limbs[uiParentLimb];

    // TODO: all this stuff

    // const ezTransform parentTransform = ezJoltConversionUtils::ToTransform(parentLimb.m_pPxBody->getGlobalPose());
    // const ezTransform thisTransform = ezJoltConversionUtils::ToTransform(thisLimb.m_pPxBody->getGlobalPose());

    ezTransform parentJointFrame;
    // parentJointFrame.SetLocalTransform(parentTransform, thisTransform); // TODO this should just be the constant local offset from child to parent (rotation is overridden anyway, position should never differ)
    // parentJointFrame.m_qRotation = thisJoint.GetLocalOrientation() * qBoneDirAdjustment;

    ezTransform thisJointFrame;
    // thisJointFrame.SetIdentity();
    // thisJointFrame.m_qRotation = qBoneDirAdjustment;

    CreateLimbJoint(thisJoint, parentLimb.m_pBodyDesc, parentJointFrame, thisLimb.m_pBodyDesc, thisJointFrame);
  }
}

void ezJoltRagdollComponent::CreateLimbBody(const LimbConfig& parentLimb, LimbConfig& thisLimb)
{
  JPH::RagdollSettings::Part* pParentLink = reinterpret_cast<JPH::RagdollSettings::Part*>(parentLimb.m_pBodyDesc);
  JPH::RagdollSettings::Part* pLink = reinterpret_cast<JPH::RagdollSettings::Part*>(thisLimb.m_pBodyDesc);

  pLink->mPosition = ezJoltConversionUtils::ToVec3(thisLimb.m_GlobalTransform.m_vPosition);
  pLink->mRotation = ezJoltConversionUtils::ToQuat(thisLimb.m_GlobalTransform.m_qRotation).Normalized();
  pLink->mMotionQuality = JPH::EMotionQuality::LinearCast;
  pLink->mGravityFactor = m_fGravityFactor;
  pLink->mUserData = reinterpret_cast<ezUInt64>(m_pJoltUserData);
  pLink->mObjectLayer = ezJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, ezJoltBroadphaseLayer::Ragdoll);
  pLink->mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  // pLink->setLinearVelocity(vPxVelocity, false);

  // TODO: setup self-collision
}

void ezJoltRagdollComponent::ComputeLimbModelSpaceTransform(ezTransform& transform, const ezMsgAnimationPoseUpdated& pose, ezUInt32 uiIndex)
{
  ezMat4 mFullTransform;
  pose.ComputeFullBoneTransform(uiIndex, mFullTransform, transform.m_qRotation);

  transform.m_vScale.Set(1);
  transform.m_vPosition = mFullTransform.GetTranslationVector();
}


void ezJoltRagdollComponent::ComputeLimbGlobalTransform(ezTransform& transform, const ezMsgAnimationPoseUpdated& pose, ezUInt32 uiIndex)
{
  ezTransform local;
  ComputeLimbModelSpaceTransform(local, pose, uiIndex);
  transform.SetGlobalTransform(GetOwner()->GetGlobalTransform(), local);
}

void ezJoltRagdollComponent::AddLimbGeometry(ezBasisAxis::Enum srcBoneDir, LimbConfig& limb, const ezSkeletonResourceGeometry& geo)
{
  // TODO: compound shapes

  JPH::RagdollSettings::Part* pBodyDesc = reinterpret_cast<JPH::RagdollSettings::Part*>(limb.m_pBodyDesc);

  // physx::PxMaterial* pxMaterial = nullptr;
  // if (geo.m_hSurface.IsValid())
  //{
  //   ezResourceLock<ezSurfaceResource> pSurface(geo.m_hSurface, ezResourceAcquireMode::BlockTillLoaded);

  //  if (pSurface->m_pPhysicsMaterial != nullptr)
  //  {
  //    pxMaterial = static_cast<physx::PxMaterial*>(pSurface->m_pPhysicsMaterial);
  //  }
  //}
  // else
  //  pxMaterial = ezJolt::GetSingleton()->GetDefaultMaterial();

  // JoltShape* pShape = nullptr;

  const ezQuat qBoneDirAdjustment = ezBasisAxis::GetBasisRotation(ezBasisAxis::PositiveX, srcBoneDir);

  const ezQuat qFinalBoneRot = /*boneRot **/ qBoneDirAdjustment;

  ezTransform st;
  st.SetIdentity();
  st.m_vPosition = /*boneTrans.GetTranslationVector() +*/ qFinalBoneRot * geo.m_Transform.m_vPosition;
  st.m_qRotation = qFinalBoneRot * geo.m_Transform.m_qRotation;

  if (geo.m_Type == ezSkeletonJointGeometryType::Sphere)
  {
    JPH::SphereShapeSettings shape;
    shape.mRadius = geo.m_Transform.m_vScale.z;
    shape.mDensity = 100.0f;
    // TODO: shape.mUserData =
    // TODO: material

    pBodyDesc->SetShape(shape.Create().Get());
  }
  else if (geo.m_Type == ezSkeletonJointGeometryType::Box)
  {
    JPH::BoxShapeSettings shape;
    shape.mHalfExtent = ezJoltConversionUtils::ToVec3(geo.m_Transform.m_vScale * 0.5f);
    // TODO: shape.mMaterial = ...
    // TODO: shape.mUserData = ...

    // TODO: if offset desired
    st.m_vPosition += qFinalBoneRot * ezVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);

    pBodyDesc->SetShape(shape.Create().Get());
  }
  else if (geo.m_Type == ezSkeletonJointGeometryType::Capsule)
  {
    JPH::CapsuleShapeSettings shape;
    shape.mHalfHeightOfCylinder = geo.m_Transform.m_vScale.x * 0.5f;
    shape.mRadius = geo.m_Transform.m_vScale.z;
    // TODO: shape.mMaterial = ...
    // TODO: shape.mUserData = ...

    ezQuat qRot;
    qRot.SetFromAxisAndAngle(ezVec3::UnitZAxis(), ezAngle::Degree(-90));
    st.m_qRotation = st.m_qRotation * qRot;

    // TODO: if offset desired
    st.m_vPosition += qFinalBoneRot * ezVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);

    pBodyDesc->SetShape(shape.Create().Get());
  }
  else
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  if (!st.IsEqual(ezTransform::IdentityTransform(), 0.001f))
  {
    JPH::RotatedTranslatedShapeSettings dec;
    dec.mInnerShapePtr = pBodyDesc->GetShape();
    dec.mPosition = ezJoltConversionUtils::ToVec3(st.m_vPosition);
    dec.mRotation = ezJoltConversionUtils::ToQuat(st.m_qRotation);
    // TODO: dec.mUserData = ...

    pBodyDesc->SetShape(dec.Create().Get());
  }

  // pShape->setLocalPose(ezJoltConversionUtils::ToTransform(st));
  // pShape->setSimulationFilterData(pxFilterData);
  // pShape->setQueryFilterData(pxFilterData);
  // pShape->userData = m_pJoltUserData;
}

void ezJoltRagdollComponent::CreateLimbJoint(const ezSkeletonJoint& thisJoint, void* pParentBodyDesc, const ezTransform& parentFrame, void* pThisBodyDesc, const ezTransform& thisFrame)
{
  JPH::RagdollSettings::Part* pLink = reinterpret_cast<JPH::RagdollSettings::Part*>(pThisBodyDesc);
  JPH::RagdollSettings::Part* pParentLink = reinterpret_cast<JPH::RagdollSettings::Part*>(pParentBodyDesc);

  ezTransform tParent = ezJoltConversionUtils::ToTransform(pParentLink->mPosition, pParentLink->mRotation);
  ezTransform tThis = ezJoltConversionUtils::ToTransform(pLink->mPosition, pLink->mRotation);

  {
    JPH::SwingTwistConstraintSettings* pJoint = new JPH::SwingTwistConstraintSettings();
    pLink->mToParent = pJoint;

    const ezQuat offsetRot = thisJoint.GetLocalOrientation();

    ezQuat qTwist;
    qTwist.SetFromAxisAndAngle(ezVec3::UnitYAxis(), thisJoint.GetTwistLimitCenterAngle());

    pJoint->mDrawConstraintSize = 0.1f;
    pJoint->mPosition1 = pLink->mPosition;
    pJoint->mPosition2 = pLink->mPosition;
    pJoint->mNormalHalfConeAngle = thisJoint.GetHalfSwingLimitZ().GetRadian(); // TODO: disable ?
    pJoint->mPlaneHalfConeAngle = thisJoint.GetHalfSwingLimitY().GetRadian();  // TODO: disable ?
    pJoint->mTwistMinAngle = -thisJoint.GetTwistLimitHalfAngle().GetRadian();
    pJoint->mTwistMaxAngle = thisJoint.GetTwistLimitHalfAngle().GetRadian();
    pJoint->mMaxFrictionTorque = m_fStiffness;
    pJoint->mPlaneAxis1 = ezJoltConversionUtils::ToVec3(tParent.m_qRotation * offsetRot * qTwist * ezVec3::UnitZAxis());
    pJoint->mPlaneAxis2 = ezJoltConversionUtils::ToVec3(tThis.m_qRotation * qTwist * ezVec3::UnitZAxis());
    pJoint->mTwistAxis1 = ezJoltConversionUtils::ToVec3(tParent.m_qRotation * offsetRot * ezVec3::UnitYAxis());
    pJoint->mTwistAxis2 = ezJoltConversionUtils::ToVec3(tThis.m_qRotation * ezVec3::UnitYAxis());
  }
}


EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltRagdollComponent);

