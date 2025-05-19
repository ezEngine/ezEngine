#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
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
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <JoltPlugin/Utilities/JoltUserData.h>
#include <Physics/Collision/Shape/CompoundShape.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonPoseComponent.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Debug/DebugRenderer.h>

/* TODO

 * prevent crashes with zero bodies
 * import sphere/box/capsule shapes

  * external constraints
 * max force clamping / point vs area impulse ?
 * communication with anim controller
 * drive to pose
 */

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezJoltRagdollStartMode, 1)
  EZ_ENUM_CONSTANTS(ezJoltRagdollStartMode::WithBindPose, ezJoltRagdollStartMode::WithNextAnimPose, ezJoltRagdollStartMode::WithCurrentMeshPose)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltRagdollComponent, 2, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("SelfCollision", m_bSelfCollision),
    EZ_ENUM_ACCESSOR_PROPERTY("StartMode", ezJoltRagdollStartMode, GetStartMode, SetStartMode),
    EZ_ACCESSOR_PROPERTY("GravityFactor", GetGravityFactor, SetGravityFactor)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("Mass", m_fMass)->AddAttributes(new ezDefaultValueAttribute(50.0f)),
    EZ_MEMBER_PROPERTY("StiffnessFactor", m_fStiffnessFactor)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("OwnerVelocityScale", m_fOwnerVelocityScale)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("CenterPosition", m_vCenterPosition),
    EZ_MEMBER_PROPERTY("CenterVelocity", m_fCenterVelocity)->AddAttributes(new ezDefaultValueAttribute(0.0f)),
    EZ_MEMBER_PROPERTY("CenterAngularVelocity", m_fCenterAngularVelocity)->AddAttributes(new ezDefaultValueAttribute(0.0f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
    EZ_MESSAGE_HANDLER(ezMsgRetrieveBoneState, OnRetrieveBoneState),
    EZ_MESSAGE_HANDLER(ezMsgPhysicsAddImpulse, OnMsgPhysicsAddImpulse),
    EZ_MESSAGE_HANDLER(ezMsgPhysicsAddForce, OnMsgPhysicsAddForce),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Jolt/Animation"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(GetObjectFilterID),
    EZ_SCRIPT_FUNCTION_PROPERTY(SetInitialImpulse, In, "vWorldPosition", In, "vWorldDirectionAndStrength"),
    EZ_SCRIPT_FUNCTION_PROPERTY(AddInitialImpulse, In, "vWorldPosition", In, "vWorldDirectionAndStrength"),
    EZ_SCRIPT_FUNCTION_PROPERTY(SetJointTypeOverride, In, "JointName", In, "OverrideType"),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

EZ_DEFINE_AS_POD_TYPE(JPH::Vec3);

//////////////////////////////////////////////////////////////////////////

ezJoltRagdollComponentManager::ezJoltRagdollComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezJoltRagdollComponent, ezBlockStorageType::FreeList>(pWorld)
{
}

ezJoltRagdollComponentManager::~ezJoltRagdollComponentManager() = default;

void ezJoltRagdollComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezJoltRagdollComponentManager::Update, this);
    desc.m_Phase = ezWorldUpdatePhase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = false;

    this->RegisterUpdateFunction(desc);
  }
}

void ezJoltRagdollComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  EZ_PROFILE_SCOPE("UpdateRagdolls");

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();

  for (auto it : pModule->GetActiveRagdolls())
  {
    ezJoltRagdollComponent* pComponent = it.Key();

    pComponent->Update(false);
  }

  for (ezJoltRagdollComponent* pComponent : pModule->GetRagdollsPutToSleep())
  {
    pComponent->Update(true);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezJoltRagdollComponent::ezJoltRagdollComponent() = default;
ezJoltRagdollComponent::~ezJoltRagdollComponent() = default;

void ezJoltRagdollComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_StartMode;
  s << m_fGravityFactor;
  s << m_bSelfCollision;
  s << m_fOwnerVelocityScale;
  s << m_fCenterVelocity;
  s << m_fCenterAngularVelocity;
  s << m_vCenterPosition;
  s << m_fMass;
  s << m_fStiffnessFactor;
}

void ezJoltRagdollComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  if (uiVersion < 2)
    return;

  s >> m_StartMode;
  s >> m_fGravityFactor;
  s >> m_bSelfCollision;
  s >> m_fOwnerVelocityScale;
  s >> m_fCenterVelocity;
  s >> m_fCenterAngularVelocity;
  s >> m_vCenterPosition;
  s >> m_fMass;
  s >> m_fStiffnessFactor;
}

void ezJoltRagdollComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_StartMode == ezJoltRagdollStartMode::WithBindPose)
  {
    CreateLimbsFromBindPose();
  }
  if (m_StartMode == ezJoltRagdollStartMode::WithCurrentMeshPose)
  {
    CreateLimbsFromCurrentMeshPose();
  }
}

void ezJoltRagdollComponent::OnDeactivated()
{
  DestroyAllLimbs();

  SUPER::OnDeactivated();
}

void ezJoltRagdollComponent::Update(bool bForce)
{
  if (!HasCreatedLimbs())
    return;

  UpdateOwnerPosition();

  const ezVisibilityState::Enum visState = GetOwner()->GetVisibilityState();
  if (!bForce && visState != ezVisibilityState::Direct)
  {
    m_ElapsedTimeSinceUpdate += ezClock::GetGlobalClock()->GetTimeDiff();

    if (visState == ezVisibilityState::Indirect && m_ElapsedTimeSinceUpdate < ezTime::MakeFromMilliseconds(200))
    {
      // when the ragdoll is only visible by shadows or reflections, update it infrequently
      return;
    }

    if (visState == ezVisibilityState::Invisible && m_ElapsedTimeSinceUpdate < ezTime::MakeFromMilliseconds(500))
    {
      // when the ragdoll is entirely invisible, update it very rarely
      return;
    }
  }

  RetrieveRagdollPose();
  SendAnimationPoseMsg();

  m_ElapsedTimeSinceUpdate = ezTime::MakeZero();
}

ezResult ezJoltRagdollComponent::EnsureSkeletonIsKnown()
{
  if (!m_hSkeleton.IsValid())
  {
    ezMsgQueryAnimationSkeleton msg;
    GetOwner()->SendMessage(msg);
    m_hSkeleton = msg.m_hSkeleton;
  }

  if (!m_hSkeleton.IsValid())
  {
    ezLog::Error("No skeleton available for ragdoll on object '{}'.", GetOwner()->GetName());
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

bool ezJoltRagdollComponent::HasCreatedLimbs() const
{
  return m_pRagdoll != nullptr;
}

void ezJoltRagdollComponent::CreateLimbsFromBindPose()
{
  DestroyAllLimbs();

  if (EnsureSkeletonIsKnown().Failed())
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);
  const auto& desc = pSkeleton->GetDescriptor();

  m_CurrentLimbTransforms.SetCountUninitialized(desc.m_Skeleton.GetJointCount());

  auto ComputeFullJointTransform = [&](ezUInt32 uiJointIdx, auto self) -> ezMat4
  {
    const auto& joint = desc.m_Skeleton.GetJointByIndex(uiJointIdx);
    const ezMat4 jointTransform = joint.GetRestPoseLocalTransform().GetAsMat4();

    if (joint.GetParentIndex() != ezInvalidJointIndex)
    {
      const ezMat4 parentTransform = self(joint.GetParentIndex(), self);

      return parentTransform * jointTransform;
    }

    return jointTransform;
  };

  for (ezUInt32 i = 0; i < m_CurrentLimbTransforms.GetCount(); ++i)
  {
    m_CurrentLimbTransforms[i] = ComputeFullJointTransform(i, ComputeFullJointTransform);
  }

  ezMsgAnimationPoseUpdated msg;
  msg.m_pRootTransform = &desc.m_RootTransform;
  msg.m_pSkeleton = &desc.m_Skeleton;
  msg.m_ModelTransforms = m_CurrentLimbTransforms;

  CreateLimbsFromPose(msg);
}

void ezJoltRagdollComponent::CreateLimbsFromCurrentMeshPose()
{
  DestroyAllLimbs();

  if (EnsureSkeletonIsKnown().Failed())
    return;

  ezAnimatedMeshComponent* pMesh = nullptr;
  if (!GetOwner()->TryGetComponentOfBaseType<ezAnimatedMeshComponent>(pMesh))
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded_NeverFail);

  ezTransform tRoot;
  pMesh->RetrievePose(m_CurrentLimbTransforms, tRoot, pSkeleton->GetDescriptor().m_Skeleton);

  ezMsgAnimationPoseUpdated msg;
  msg.m_pRootTransform = &tRoot;
  msg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;
  msg.m_ModelTransforms = m_CurrentLimbTransforms;

  CreateLimbsFromPose(msg);
}

void ezJoltRagdollComponent::DestroyAllLimbs()
{
  if (m_pRagdoll)
  {
    m_pRagdoll->RemoveFromPhysicsSystem();
    m_pRagdoll->Release();
    m_pRagdoll = nullptr;
  }

  if (ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>())
  {
    pModule->DeallocateUserData(m_uiJoltUserDataIndex);
    pModule->DeleteObjectFilterID(m_uiObjectFilterID);
    m_pJoltUserData = nullptr;
  }

  m_CurrentLimbTransforms.Clear();
  m_Limbs.Clear();
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

void ezJoltRagdollComponent::SetStartMode(ezEnum<ezJoltRagdollStartMode> mode)
{
  if (m_StartMode == mode)
    return;

  m_StartMode = mode;
}

void ezJoltRagdollComponent::OnMsgPhysicsAddImpulse(ezMsgPhysicsAddImpulse& ref_msg)
{
  if (!HasCreatedLimbs())
  {
    m_vInitialImpulsePosition += ref_msg.m_vGlobalPosition;
    m_vInitialImpulseDirection += ref_msg.m_vImpulse;
    m_uiNumInitialImpulses++;
    return;
  }

  // TODO: normalize by number of limbs
  const ezUInt32 uiBodyId = reinterpret_cast<size_t>(ref_msg.m_pInternalPhysicsActor) & 0xFFFFFFFF;
  GetWorld()->GetModule<ezJoltWorldModule>()->AddImpulse(uiBodyId, ref_msg.m_vImpulse, ref_msg.m_vGlobalPosition);
}

void ezJoltRagdollComponent::OnMsgPhysicsAddForce(ezMsgPhysicsAddForce& ref_msg)
{
  if (!HasCreatedLimbs())
    return;

  JPH::BodyID bodyId = JPH::BodyID(reinterpret_cast<size_t>(ref_msg.m_pInternalPhysicsActor) & 0xFFFFFFFF);
  if (!bodyId.IsInvalid())
  {
    auto pBodies = &GetWorld()->GetModule<ezJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
    pBodies->AddForce(bodyId, ezJoltConversionUtils::ToVec3(ref_msg.m_vForce), ezJoltConversionUtils::ToVec3(ref_msg.m_vGlobalPosition));
  }
}

void ezJoltRagdollComponent::SetInitialImpulse(const ezVec3& vPosition, const ezVec3& vDirectionAndStrength)
{
  if (vDirectionAndStrength.IsZero())
  {
    m_vInitialImpulsePosition.SetZero();
    m_vInitialImpulseDirection.SetZero();
    m_uiNumInitialImpulses = 0;
  }
  else
  {
    m_vInitialImpulsePosition = vPosition;
    m_vInitialImpulseDirection = vDirectionAndStrength;
    m_uiNumInitialImpulses = 1;
  }
}

void ezJoltRagdollComponent::AddInitialImpulse(const ezVec3& vPosition, const ezVec3& vDirectionAndStrength)
{
  m_vInitialImpulsePosition += vPosition;
  m_vInitialImpulseDirection += vDirectionAndStrength;
  m_uiNumInitialImpulses++;
}

void ezJoltRagdollComponent::SetJointTypeOverride(ezStringView sJointName, ezEnum<ezSkeletonJointType> type)
{
  const ezTempHashedString sJointNameHashed(sJointName);

  for (ezUInt32 i = 0; i < m_JointOverrides.GetCount(); ++i)
  {
    if (m_JointOverrides[i].m_sJointName == sJointNameHashed)
    {
      m_JointOverrides[i].m_JointType = type;
      m_JointOverrides[i].m_bOverrideType = true;
      return;
    }
  }

  auto& jo = m_JointOverrides.ExpandAndGetRef();
  jo.m_sJointName = sJointNameHashed;
  jo.m_JointType = type;
  jo.m_bOverrideType = true;
}

void ezJoltRagdollComponent::OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& ref_poseMsg)
{
  if (!IsActiveAndSimulating())
    return;

  if (HasCreatedLimbs())
  {
    ref_poseMsg.m_bContinueAnimating = false; // TODO: change this

    // TODO: if at some point we can layer ragdolls with detail animations, we should
    // take poses for all bones for which there are no shapes (link == null) -> to animate leafs (fingers and such)
    return;
  }

  if (m_StartMode != ezJoltRagdollStartMode::WithNextAnimPose)
    return;

  m_CurrentLimbTransforms = ref_poseMsg.m_ModelTransforms;

  CreateLimbsFromPose(ref_poseMsg);
}

void ezJoltRagdollComponent::OnRetrieveBoneState(ezMsgRetrieveBoneState& ref_msg) const
{
  if (!HasCreatedLimbs())
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);
  const auto& skeleton = pSkeleton->GetDescriptor().m_Skeleton;

  for (ezUInt32 uiJointIdx = 0; uiJointIdx < skeleton.GetJointCount(); ++uiJointIdx)
  {
    ezMat4 mJoint = m_CurrentLimbTransforms[uiJointIdx];

    const auto& joint = skeleton.GetJointByIndex(uiJointIdx);
    const ezUInt16 uiParentIdx = joint.GetParentIndex();
    if (uiParentIdx != ezInvalidJointIndex)
    {
      // remove the parent transform to get the pure local transform
      const ezMat4 mParent = m_CurrentLimbTransforms[uiParentIdx].GetInverse();

      mJoint = mParent * mJoint;
    }

    auto& t = ref_msg.m_BoneTransforms[joint.GetName().GetString()];
    t.m_vPosition = mJoint.GetTranslationVector();
    t.m_qRotation.ReconstructFromMat4(mJoint);
    t.m_vScale.Set(1.0f);
  }
}

void ezJoltRagdollComponent::SendAnimationPoseMsg()
{
  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);
  const ezTransform rootTransform = pSkeleton->GetDescriptor().m_RootTransform;

  ezMsgAnimationPoseUpdated poseMsg;
  poseMsg.m_ModelTransforms = m_CurrentLimbTransforms;
  poseMsg.m_pRootTransform = &rootTransform;
  poseMsg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;

  GetOwner()->SendMessage(poseMsg);
}

ezTransform ezJoltRagdollComponent::GetRagdollRootTransform() const
{
  JPH::Vec3 joltRootPos;
  JPH::Quat joltRootRot;
  m_pRagdoll->GetRootTransform(joltRootPos, joltRootRot);

  ezTransform res = ezJoltConversionUtils::ToTransform(joltRootPos, joltRootRot);
  res.m_vScale = GetOwner()->GetGlobalScaling();

  return res;
}

void ezJoltRagdollComponent::UpdateOwnerPosition()
{
  GetOwner()->SetGlobalTransform(GetRagdollRootTransform() * m_RootBodyLocalTransform.GetInverse());
}

void ezJoltRagdollComponent::RetrieveRagdollPose()
{
  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);
  const ezSkeleton& skeleton = pSkeleton->GetDescriptor().m_Skeleton;
  const ezTransform rootTransform = pSkeleton->GetDescriptor().m_RootTransform;
  const ezMat4 invRootTransform = rootTransform.GetAsMat4().GetInverse();
  const ezMat4 mInv = invRootTransform * m_RootBodyLocalTransform.GetAsMat4() * GetRagdollRootTransform().GetInverse().GetAsMat4();

  const ezVec3 vObjectScale = GetOwner()->GetGlobalScaling();
  const float fObjectScale = ezMath::Max(vObjectScale.x, vObjectScale.y, vObjectScale.z);

  ezMat4 scale = ezMat4::MakeScaling(rootTransform.m_vScale * fObjectScale);

  ezHybridArray<ezMat4, 64> relativeTransforms;

  {
    // m_CurrentLimbTransforms is stored in model space
    // for bones that don't have their own shape in the ragdoll,
    // we don't get a new transform from the ragdoll, but we still must update them,
    // if there is a parent bone in the ragdoll, otherwise they don't move along as expected
    // therefore we compute their relative transform here
    // and then later we take their new parent transform (which may come from the ragdoll)
    // to set their final new transform

    for (ezUInt32 uiLimbIdx = 0; uiLimbIdx < m_Limbs.GetCount(); ++uiLimbIdx)
    {
      if (m_Limbs[uiLimbIdx].m_uiPartIndex != ezInvalidJointIndex)
        continue;

      const auto& joint = skeleton.GetJointByIndex(uiLimbIdx);
      const ezUInt16 uiParentIdx = joint.GetParentIndex();

      if (uiParentIdx == ezInvalidJointIndex)
        continue;

      const ezMat4 mJoint = m_CurrentLimbTransforms[uiLimbIdx];

      // remove the parent transform to get the pure local transform
      const ezMat4 mParentInv = m_CurrentLimbTransforms[uiParentIdx].GetInverse();

      relativeTransforms.PushBack(mParentInv * mJoint);
    }
  }

  ezUInt32 uiNextRelativeIdx = 0;
  for (ezUInt32 uiLimbIdx = 0; uiLimbIdx < m_Limbs.GetCount(); ++uiLimbIdx)
  {
    if (m_Limbs[uiLimbIdx].m_uiPartIndex == ezInvalidJointIndex)
    {
      const auto& joint = skeleton.GetJointByIndex(uiLimbIdx);
      const ezUInt16 uiParentIdx = joint.GetParentIndex();

      if (uiParentIdx != ezInvalidJointIndex)
      {
        m_CurrentLimbTransforms[uiLimbIdx] = m_CurrentLimbTransforms[uiParentIdx] * relativeTransforms[uiNextRelativeIdx];
        ++uiNextRelativeIdx;
      }
    }
    else
    {
      const JPH::BodyID bodyId = m_pRagdoll->GetBodyID(m_Limbs[uiLimbIdx].m_uiPartIndex);
      EZ_ASSERT_DEBUG(!bodyId.IsInvalid(), "Invalid limb -> body mapping");
      JPH::BodyLockRead bodyRead(pModule->GetJoltSystem()->GetBodyLockInterface(), bodyId);

      const ezTransform limbGlobalPose = ezJoltConversionUtils::ToTransform(bodyRead.GetBody().GetPosition(), bodyRead.GetBody().GetRotation());

      m_CurrentLimbTransforms[uiLimbIdx] = (mInv * limbGlobalPose.GetAsMat4()) * scale;
    }
  }
}

void ezJoltRagdollComponent::CreateLimbsFromPose(const ezMsgAnimationPoseUpdated& pose)
{
  EZ_ASSERT_DEBUG(!HasCreatedLimbs(), "Limbs are already created.");

  if (EnsureSkeletonIsKnown().Failed())
    return;

  const ezVec3 vObjectScale = GetOwner()->GetGlobalScaling();
  const float fObjectScale = ezMath::Max(vObjectScale.x, vObjectScale.y, vObjectScale.z);

  ezJoltWorldModule& worldModule = *GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
  m_uiObjectFilterID = worldModule.CreateObjectFilterID();
  m_uiJoltUserDataIndex = worldModule.AllocateUserData(m_pJoltUserData);
  m_pJoltUserData->Init(this);

  ezResourceLock<ezSkeletonResource> pSkeletonResource(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);

  // allocate the limbs array
  m_Limbs.SetCount(pose.m_ModelTransforms.GetCount());

  JPH::Ref<JPH::RagdollSettings> ragdollSettings = new JPH::RagdollSettings();
  EZ_SCOPE_EXIT(m_pRagdollSettings = nullptr);

  m_pRagdollSettings = ragdollSettings.GetPtr();
  m_pRagdollSettings->mParts.reserve(pSkeletonResource->GetDescriptor().m_Skeleton.GetJointCount());
  m_pRagdollSettings->mSkeleton = new JPH::Skeleton(); // TODO: share this in the resource
  m_pRagdollSettings->mSkeleton->GetJoints().reserve(m_pRagdollSettings->mParts.size());

  CreateAllLimbs(*pSkeletonResource.GetPointer(), pose, worldModule, fObjectScale);
  ApplyBodyMass();
  SetupLimbJoints(pSkeletonResource.GetPointer());
  ApplyPartInitialVelocity();

  if (m_bSelfCollision)
  {
    // enables collisions between all bodies except the ones that are directly connected to each other
    m_pRagdollSettings->DisableParentChildCollisions();
  }

  m_pRagdollSettings->Stabilize();

  m_pRagdoll = m_pRagdollSettings->CreateRagdoll(m_uiObjectFilterID, reinterpret_cast<ezUInt64>(m_pJoltUserData), worldModule.GetJoltSystem());

  m_pRagdoll->AddRef();
  m_pRagdoll->AddToPhysicsSystem(JPH::EActivation::Activate);

  ApplyInitialImpulse(worldModule, pSkeletonResource->GetDescriptor().m_fMaxImpulse);
}

void ezJoltRagdollComponent::ConfigureRagdollPart(void* pRagdollSettingsPart, const ezTransform& globalTransform, ezUInt8 uiCollisionLayer, ezJoltWorldModule& worldModule)
{
  JPH::RagdollSettings::Part* pPart = reinterpret_cast<JPH::RagdollSettings::Part*>(pRagdollSettingsPart);

  pPart->mPosition = ezJoltConversionUtils::ToVec3(globalTransform.m_vPosition);
  pPart->mRotation = ezJoltConversionUtils::ToQuat(globalTransform.m_qRotation).Normalized();
  pPart->mMotionQuality = JPH::EMotionQuality::LinearCast;
  pPart->mGravityFactor = m_fGravityFactor;
  pPart->mUserData = reinterpret_cast<ezUInt64>(m_pJoltUserData);
  pPart->mObjectLayer = ezJoltCollisionFiltering::ConstructObjectLayer(uiCollisionLayer, ezJoltBroadphaseLayer::Ragdoll);
  pPart->mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  pPart->mCollisionGroup.SetGroupFilter(worldModule.GetGroupFilterIgnoreSame()); // this is used if m_bSelfCollision is off, otherwise it gets overridden below
}

void ezJoltRagdollComponent::ApplyPartInitialVelocity()
{
  JPH::Vec3 vCommonVelocity = ezJoltConversionUtils::ToVec3(GetOwner()->GetLinearVelocity() * m_fOwnerVelocityScale);
  const JPH::Vec3 vCenterPos = ezJoltConversionUtils::ToVec3(GetOwner()->GetGlobalTransform() * m_vCenterPosition);

  ezCoordinateSystem coord;
  GetWorld()->GetCoordinateSystem(GetOwner()->GetGlobalPosition(), coord);
  ezRandom& rng = GetOwner()->GetWorld()->GetRandomNumberGenerator();

  for (JPH::RagdollSettings::Part& part : m_pRagdollSettings->mParts)
  {
    part.mLinearVelocity = vCommonVelocity;

    if (m_fCenterVelocity != 0.0f)
    {
      const JPH::Vec3 vVelocityDir = (part.mPosition - vCenterPos).NormalizedOr(JPH::Vec3::sZero());
      part.mLinearVelocity += vVelocityDir * ezMath::Min(part.mMaxLinearVelocity, m_fCenterVelocity);
    }

    if (m_fCenterAngularVelocity != 0.0f)
    {
      const ezVec3 vVelocityDir = ezJoltConversionUtils::ToVec3(part.mPosition - vCenterPos);
      ezVec3 vRotationDir = vVelocityDir.CrossRH(coord.m_vUpDir);
      vRotationDir.NormalizeIfNotZero(coord.m_vUpDir).IgnoreResult();

      ezVec3 vRotationAxis = ezVec3::MakeRandomDeviation(rng, ezAngle::MakeFromDegree(30.0f), vRotationDir);
      vRotationAxis *= rng.Bool() ? 1.0f : -1.0f;

      float fSpeed = rng.FloatVariance(m_fCenterAngularVelocity, 0.5f);
      fSpeed = ezMath::Min(fSpeed, part.mMaxAngularVelocity * 0.95f);

      part.mAngularVelocity = ezJoltConversionUtils::ToVec3(vRotationAxis) * fSpeed;
    }
  }
}

void ezJoltRagdollComponent::ApplyInitialImpulse(ezJoltWorldModule& worldModule, float fMaxImpulse)
{
  if (m_uiNumInitialImpulses == 0)
    return;

  if (m_uiNumInitialImpulses > 1)
  {
    ezLog::Info("Impulses: {} - {}", m_uiNumInitialImpulses, m_vInitialImpulseDirection.GetLength());
  }

  auto pJoltSystem = worldModule.GetJoltSystem();

  m_vInitialImpulsePosition /= m_uiNumInitialImpulses;

  float fImpulse = m_vInitialImpulseDirection.GetLength();

  if (fImpulse > fMaxImpulse)
  {
    fImpulse = fMaxImpulse;
    m_vInitialImpulseDirection.SetLength(fImpulse).AssertSuccess();
  }

  const JPH::Vec3 vImpulsePosition = ezJoltConversionUtils::ToVec3(m_vInitialImpulsePosition);
  float fLowestDistanceSqr = 100000;

  JPH::BodyID closestBody;

  for (ezUInt32 uiBodyIdx = 0; uiBodyIdx < m_pRagdoll->GetBodyCount(); ++uiBodyIdx)
  {
    const JPH::BodyID bodyId = m_pRagdoll->GetBodyID(uiBodyIdx);
    JPH::BodyLockRead bodyRead(pJoltSystem->GetBodyLockInterface(), bodyId);

    const float fDistanceToImpulseSqr = (bodyRead.GetBody().GetPosition() - vImpulsePosition).LengthSq();

    if (fDistanceToImpulseSqr < fLowestDistanceSqr)
    {
      fLowestDistanceSqr = fDistanceToImpulseSqr;
      closestBody = bodyId;
    }
  }

  if (pJoltSystem->GetBodyInterface().IsAdded(closestBody))
  {
    pJoltSystem->GetBodyInterface().AddImpulse(closestBody, ezJoltConversionUtils::ToVec3(m_vInitialImpulseDirection), vImpulsePosition);
  }
}


void ezJoltRagdollComponent::ApplyBodyMass()
{
  if (m_fMass <= 0.0f)
    return;

  float fPartMass = m_fMass / m_pRagdollSettings->mParts.size();

  for (auto& part : m_pRagdollSettings->mParts)
  {
    part.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    part.mMassPropertiesOverride.mMass = fPartMass;
  }
}

void ezJoltRagdollComponent::ComputeLimbModelSpaceTransform(ezTransform& transform, const ezMsgAnimationPoseUpdated& pose, ezUInt32 uiPoseJointIndex)
{
  ezMat4 mFullTransform;
  pose.ComputeFullBoneTransform(uiPoseJointIndex, mFullTransform, transform.m_qRotation);

  transform.m_vScale.Set(1);
  transform.m_vPosition = mFullTransform.GetTranslationVector();
}


void ezJoltRagdollComponent::ComputeLimbGlobalTransform(ezTransform& transform, const ezMsgAnimationPoseUpdated& pose, ezUInt32 uiPoseJointIndex)
{
  ezTransform local;
  ComputeLimbModelSpaceTransform(local, pose, uiPoseJointIndex);
  transform = ezTransform::MakeGlobalTransform(GetOwner()->GetGlobalTransform(), local);
}

void ezJoltRagdollComponent::CreateAllLimbs(const ezSkeletonResource& skeletonResource, const ezMsgAnimationPoseUpdated& pose, ezJoltWorldModule& worldModule, float fObjectScale)
{
  ezMap<ezUInt16, LimbConstructionInfo> limbConstructionInfos(ezFrameAllocator::GetCurrentAllocator());
  limbConstructionInfos.FindOrAdd(ezInvalidJointIndex); // dummy root link

  ezUInt16 uiLastLimbIdx = ezInvalidJointIndex;
  ezHybridArray<const ezSkeletonResourceGeometry*, 8> geometries;

  for (const auto& geo : skeletonResource.GetDescriptor().m_Geometry)
  {
    if (geo.m_Type == ezSkeletonJointGeometryType::None)
      continue;

    if (geo.m_uiAttachedToJoint != uiLastLimbIdx)
    {
      CreateLimb(skeletonResource, limbConstructionInfos, geometries, pose, worldModule, fObjectScale);
      geometries.Clear();
      uiLastLimbIdx = geo.m_uiAttachedToJoint;
    }

    geometries.PushBack(&geo);
  }

  CreateLimb(skeletonResource, limbConstructionInfos, geometries, pose, worldModule, fObjectScale);

  // get the limb with the lowest index (ie. the first one added) as the root joint
  // and use it's transform to compute m_RootBodyLocalTransform
  m_RootBodyLocalTransform = ezTransform::MakeLocalTransform(GetOwner()->GetGlobalTransform(), limbConstructionInfos.GetIterator().Value().m_GlobalTransform);
}

void ezJoltRagdollComponent::CreateLimb(const ezSkeletonResource& skeletonResource, ezMap<ezUInt16, LimbConstructionInfo>& limbConstructionInfos, ezArrayPtr<const ezSkeletonResourceGeometry*> geometries, const ezMsgAnimationPoseUpdated& pose, ezJoltWorldModule& worldModule, float fObjectScale)
{
  if (geometries.IsEmpty())
    return;

  const ezSkeleton& skeleton = skeletonResource.GetDescriptor().m_Skeleton;

  const ezUInt16 uiThisJointIdx = geometries[0]->m_uiAttachedToJoint;
  const ezSkeletonJoint& thisLimbJoint = skeleton.GetJointByIndex(uiThisJointIdx);
  ezUInt16 uiParentJointIdx = thisLimbJoint.GetParentIndex();

  // find the parent joint that is also part of the ragdoll
  while (!limbConstructionInfos.Contains(uiParentJointIdx))
  {
    uiParentJointIdx = skeleton.GetJointByIndex(uiParentJointIdx).GetParentIndex();
  }
  // now uiParentJointIdx is either the index of a limb that has been created before, or ezInvalidJointIndex

  LimbConstructionInfo& thisLimbInfo = limbConstructionInfos[uiThisJointIdx];
  const LimbConstructionInfo& parentLimbInfo = limbConstructionInfos[uiParentJointIdx];

  thisLimbInfo.m_uiJoltPartIndex = (ezUInt16)m_pRagdollSettings->mParts.size();
  m_pRagdollSettings->mParts.resize(m_pRagdollSettings->mParts.size() + 1);

  m_Limbs[uiThisJointIdx].m_uiPartIndex = thisLimbInfo.m_uiJoltPartIndex;

  m_pRagdollSettings->mSkeleton->AddJoint(thisLimbJoint.GetName().GetData(), parentLimbInfo.m_uiJoltPartIndex != ezInvalidJointIndex ? parentLimbInfo.m_uiJoltPartIndex : -1);

  ComputeLimbGlobalTransform(thisLimbInfo.m_GlobalTransform, pose, uiThisJointIdx);
  ConfigureRagdollPart(&m_pRagdollSettings->mParts[thisLimbInfo.m_uiJoltPartIndex], thisLimbInfo.m_GlobalTransform, thisLimbJoint.GetCollisionLayer(), worldModule);
  CreateAllLimbGeoShapes(thisLimbInfo, geometries, thisLimbJoint, skeletonResource, fObjectScale);
}

JPH::Shape* ezJoltRagdollComponent::CreateLimbGeoShape(const LimbConstructionInfo& limbConstructionInfo, const ezSkeletonResourceGeometry& geo, const ezJoltMaterial* pJoltMaterial, const ezQuat& qBoneDirAdjustment, const ezTransform& skeletonRootTransform, ezTransform& out_shapeTransform, float fObjectScale)
{
  out_shapeTransform.SetIdentity();
  out_shapeTransform.m_vPosition = qBoneDirAdjustment * geo.m_Transform.m_vPosition * fObjectScale;
  out_shapeTransform.m_qRotation = qBoneDirAdjustment * geo.m_Transform.m_qRotation;

  JPH::Ref<JPH::Shape> pShape;

  switch (geo.m_Type)
  {
    case ezSkeletonJointGeometryType::Sphere:
    {
      JPH::SphereShapeSettings shape;
      shape.mUserData = reinterpret_cast<ezUInt64>(m_pJoltUserData);
      shape.mMaterial = pJoltMaterial;
      shape.mRadius = geo.m_Transform.m_vScale.z * fObjectScale;

      pShape = shape.Create().Get();
    }
    break;

    case ezSkeletonJointGeometryType::Box:
    {
      JPH::BoxShapeSettings shape;
      shape.mUserData = reinterpret_cast<ezUInt64>(m_pJoltUserData);
      shape.mMaterial = pJoltMaterial;
      shape.mHalfExtent = ezJoltConversionUtils::ToVec3(geo.m_Transform.m_vScale * 0.5f) * fObjectScale;

      out_shapeTransform.m_vPosition += qBoneDirAdjustment * ezVec3(geo.m_Transform.m_vScale.x * 0.5f * fObjectScale, 0, 0);

      pShape = shape.Create().Get();
    }
    break;

    case ezSkeletonJointGeometryType::Capsule:
    {
      JPH::CapsuleShapeSettings shape;
      shape.mUserData = reinterpret_cast<ezUInt64>(m_pJoltUserData);
      shape.mMaterial = pJoltMaterial;
      shape.mHalfHeightOfCylinder = geo.m_Transform.m_vScale.x * 0.5f * fObjectScale;
      shape.mRadius = geo.m_Transform.m_vScale.z * fObjectScale;

      ezQuat qRot = ezQuat::MakeFromAxisAndAngle(ezVec3::MakeAxisZ(), ezAngle::MakeFromDegree(-90));
      out_shapeTransform.m_qRotation = out_shapeTransform.m_qRotation * qRot;
      out_shapeTransform.m_vPosition += qBoneDirAdjustment * ezVec3(geo.m_Transform.m_vScale.x * 0.5f * fObjectScale, 0, 0);

      pShape = shape.Create().Get();
    }
    break;

    case ezSkeletonJointGeometryType::ConvexMesh:
    {
      // convex mesh vertices are in "global space" of the mesh file format
      // so first move them into global space of the EZ convention (skeletonRootTransform)
      // then move them to the global position of the ragdoll object
      // then apply the inverse global transform of the limb, to move everything into local space of the limb

      out_shapeTransform = limbConstructionInfo.m_GlobalTransform.GetInverse() * GetOwner()->GetGlobalTransform() * skeletonRootTransform;
      out_shapeTransform.m_vPosition *= fObjectScale;

      ezHybridArray<JPH::Vec3, 256> verts;
      verts.SetCountUninitialized(geo.m_VertexPositions.GetCount());

      for (ezUInt32 i = 0; i < verts.GetCount(); ++i)
      {
        verts[i] = ezJoltConversionUtils::ToVec3(geo.m_VertexPositions[i] * fObjectScale);
      }

      JPH::ConvexHullShapeSettings shape(verts.GetData(), (int)verts.GetCount());
      shape.mUserData = reinterpret_cast<ezUInt64>(m_pJoltUserData);
      shape.mMaterial = pJoltMaterial;

      const auto shapeRes = shape.Create();

      if (shapeRes.HasError())
      {
        ezLog::Error("Cooking convex ragdoll piece failed: {}", shapeRes.GetError().c_str());
        return nullptr;
      }

      pShape = shapeRes.Get();
    }
    break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  pShape->AddRef();
  return pShape;
}

void ezJoltRagdollComponent::CreateAllLimbGeoShapes(const LimbConstructionInfo& limbConstructionInfo, ezArrayPtr<const ezSkeletonResourceGeometry*> geometries, const ezSkeletonJoint& thisLimbJoint, const ezSkeletonResource& skeletonResource, float fObjectScale)
{
  const ezJoltMaterial* pJoltMaterial = ezJoltCore::GetDefaultMaterial();

  if (thisLimbJoint.GetSurface().IsValid())
  {
    ezResourceLock<ezSurfaceResource> pSurface(thisLimbJoint.GetSurface(), ezResourceAcquireMode::BlockTillLoaded);

    if (pSurface->m_pPhysicsMaterialJolt != nullptr)
    {
      pJoltMaterial = static_cast<ezJoltMaterial*>(pSurface->m_pPhysicsMaterialJolt);
    }
  }

  const ezTransform& skeletonRootTransform = skeletonResource.GetDescriptor().m_RootTransform;

  const auto srcBoneDir = skeletonResource.GetDescriptor().m_Skeleton.m_BoneDirection;
  const ezQuat qBoneDirAdjustment = ezBasisAxis::GetBasisRotation(ezBasisAxis::PositiveX, srcBoneDir);

  JPH::RagdollSettings::Part* pBodyDesc = &m_pRagdollSettings->mParts[limbConstructionInfo.m_uiJoltPartIndex];

  if (geometries.GetCount() > 1)
  {
    JPH::StaticCompoundShapeSettings compound;

    for (const ezSkeletonResourceGeometry* pGeo : geometries)
    {
      ezTransform shapeTransform;
      if (JPH::Shape* pSubShape = CreateLimbGeoShape(limbConstructionInfo, *pGeo, pJoltMaterial, qBoneDirAdjustment, skeletonRootTransform, shapeTransform, fObjectScale))
      {
        compound.AddShape(ezJoltConversionUtils::ToVec3(shapeTransform.m_vPosition), ezJoltConversionUtils::ToQuat(shapeTransform.m_qRotation), pSubShape);
        pSubShape->Release(); // had to manual AddRef once
      }
    }

    const auto compoundRes = compound.Create();
    if (!compoundRes.IsValid())
    {
      ezLog::Error("Creating a compound shape for a ragdoll failed: {}", compoundRes.GetError().c_str());
      return;
    }

    pBodyDesc->SetShape(compoundRes.Get());
  }
  else
  {
    ezTransform shapeTransform;
    JPH::Shape* pSubShape = CreateLimbGeoShape(limbConstructionInfo, *geometries[0], pJoltMaterial, qBoneDirAdjustment, skeletonRootTransform, shapeTransform, fObjectScale);

    if (!shapeTransform.IsEqual(ezTransform::MakeIdentity(), 0.001f))
    {
      JPH::RotatedTranslatedShapeSettings outerShape;
      outerShape.mInnerShapePtr = pSubShape;
      outerShape.mPosition = ezJoltConversionUtils::ToVec3(shapeTransform.m_vPosition);
      outerShape.mRotation = ezJoltConversionUtils::ToQuat(shapeTransform.m_qRotation);
      outerShape.mUserData = reinterpret_cast<ezUInt64>(m_pJoltUserData);

      pBodyDesc->SetShape(outerShape.Create().Get());
    }
    else
    {
      pBodyDesc->SetShape(pSubShape);
    }

    pSubShape->Release(); // had to manual AddRef once
  }
}



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


void ezJoltRagdollComponent::SetupLimbJoints(const ezSkeletonResource* pSkeleton)
{
  // TODO: still needed ? (it should be)
  // the main direction of Jolt bones is +X (for bone limits and such)
  // therefore the main direction of the source bones has to be adjusted
  // const auto srcBoneDir = pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection;
  // const ezQuat qBoneDirAdjustment = -ezBasisAxis::GetBasisRotation(srcBoneDir, ezBasisAxis::PositiveX);

  const auto& skeleton = pSkeleton->GetDescriptor().m_Skeleton;

  for (ezUInt32 uiLimbIdx = 0; uiLimbIdx < m_Limbs.GetCount(); ++uiLimbIdx)
  {
    const auto& thisLimb = m_Limbs[uiLimbIdx];

    if (thisLimb.m_uiPartIndex == ezInvalidJointIndex)
      continue;

    const ezSkeletonJoint& thisJoint = skeleton.GetJointByIndex(uiLimbIdx);
    ezUInt16 uiParentLimb = thisJoint.GetParentIndex();
    while (uiParentLimb != ezInvalidJointIndex && m_Limbs[uiParentLimb].m_uiPartIndex == ezInvalidJointIndex)
    {
      uiParentLimb = skeleton.GetJointByIndex(uiParentLimb).GetParentIndex();
    }

    if (uiParentLimb == ezInvalidJointIndex)
      continue;

    const auto& parentLimb = m_Limbs[uiParentLimb];

    CreateLimbJoint(thisJoint, &m_pRagdollSettings->mParts[parentLimb.m_uiPartIndex], &m_pRagdollSettings->mParts[thisLimb.m_uiPartIndex]);
  }
}

void ezJoltRagdollComponent::CreateLimbJoint(const ezSkeletonJoint& thisJoint, void* pParentBodyDesc, void* pThisBodyDesc)
{
  ezEnum<ezSkeletonJointType> jointType = thisJoint.GetJointType();

  for (ezUInt32 i = 0; i < m_JointOverrides.GetCount(); ++i)
  {
    if (m_JointOverrides[i].m_sJointName == thisJoint.GetName())
    {
      if (m_JointOverrides[i].m_bOverrideType)
      {
        jointType = m_JointOverrides[i].m_JointType;
      }

      break;
    }
  }

  if (jointType == ezSkeletonJointType::None)
    return;

  JPH::RagdollSettings::Part* pLink = reinterpret_cast<JPH::RagdollSettings::Part*>(pThisBodyDesc);
  JPH::RagdollSettings::Part* pParentLink = reinterpret_cast<JPH::RagdollSettings::Part*>(pParentBodyDesc);

  ezTransform tParent = ezJoltConversionUtils::ToTransform(pParentLink->mPosition, pParentLink->mRotation);
  ezTransform tThis = ezJoltConversionUtils::ToTransform(pLink->mPosition, pLink->mRotation);

  if (jointType == ezSkeletonJointType::Fixed)
  {
    JPH::FixedConstraintSettings* pJoint = new JPH::FixedConstraintSettings();
    pLink->mToParent = pJoint;

    pJoint->mDrawConstraintSize = 0.1f;
    pJoint->mPoint1 = pLink->mPosition;
    pJoint->mPoint2 = pLink->mPosition;
  }

  if (jointType == ezSkeletonJointType::SwingTwist)
  {
    JPH::SwingTwistConstraintSettings* pJoint = new JPH::SwingTwistConstraintSettings();
    pLink->mToParent = pJoint;

    const ezQuat offsetRot = thisJoint.GetLocalOrientation();

    ezQuat qTwist = ezQuat::MakeFromAxisAndAngle(ezVec3::MakeAxisY(), thisJoint.GetTwistLimitCenterAngle());

    pJoint->mDrawConstraintSize = 0.1f;
    pJoint->mPosition1 = pLink->mPosition;
    pJoint->mPosition2 = pLink->mPosition;
    pJoint->mNormalHalfConeAngle = thisJoint.GetHalfSwingLimitZ().GetRadian();
    pJoint->mPlaneHalfConeAngle = thisJoint.GetHalfSwingLimitY().GetRadian();
    pJoint->mTwistMinAngle = -thisJoint.GetTwistLimitHalfAngle().GetRadian();
    pJoint->mTwistMaxAngle = thisJoint.GetTwistLimitHalfAngle().GetRadian();
    pJoint->mMaxFrictionTorque = m_fStiffnessFactor * thisJoint.GetStiffness();
    pJoint->mPlaneAxis1 = ezJoltConversionUtils::ToVec3(tParent.m_qRotation * offsetRot * qTwist * ezVec3::MakeAxisZ()).Normalized();
    pJoint->mPlaneAxis2 = ezJoltConversionUtils::ToVec3(tThis.m_qRotation * qTwist * ezVec3::MakeAxisZ()).Normalized();
    pJoint->mTwistAxis1 = ezJoltConversionUtils::ToVec3(tParent.m_qRotation * offsetRot * ezVec3::MakeAxisY()).Normalized();
    pJoint->mTwistAxis2 = ezJoltConversionUtils::ToVec3(tThis.m_qRotation * ezVec3::MakeAxisY()).Normalized();
  }
}

EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltRagdollComponent);
