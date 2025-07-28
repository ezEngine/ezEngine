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
#include <Physics/Body/BodyLockMulti.h>
#include <Physics/Collision/Shape/CompoundShape.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonPoseComponent.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Debug/DebugRenderer.h>

/* TODO
 * prevent crashes with zero bodies
 * external constraints
 * max force clamping / point vs area impulse ?
 * integrate root motion into controlled playback (?)
 */

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezJoltRagdollStartMode, 1)
  EZ_ENUM_CONSTANTS(ezJoltRagdollStartMode::WithBindPose, ezJoltRagdollStartMode::WithNextAnimPose, ezJoltRagdollStartMode::WithCurrentMeshPose)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezJoltRagdollAnimMode, 1)
  EZ_ENUM_CONSTANTS(ezJoltRagdollAnimMode::Off, ezJoltRagdollAnimMode::Limp, ezJoltRagdollAnimMode::Powered, ezJoltRagdollAnimMode::Controlled)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltRagdollComponent, 5, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("SelfCollision", m_bSelfCollision),
    EZ_ENUM_ACCESSOR_PROPERTY("StartMode", ezJoltRagdollStartMode, GetStartMode, SetStartMode),
    EZ_ENUM_ACCESSOR_PROPERTY("AnimMode", ezJoltRagdollAnimMode, GetAnimMode, SetAnimMode),
    EZ_ACCESSOR_PROPERTY("GravityFactor", GetGravityFactor, SetGravityFactor)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("WeightCategory", m_uiWeightCategory)->AddAttributes(new ezDynamicEnumAttribute("PhysicsWeightCategory")),
    EZ_ACCESSOR_PROPERTY("WeightScale", GetWeight_Scale, SetWeight_Scale)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.1f, 10.0f)),
    EZ_ACCESSOR_PROPERTY("Mass", GetWeight_Mass, SetWeight_Mass)->AddAttributes(new ezSuffixAttribute(" kg"), new ezDefaultValueAttribute(50.0f), new ezClampValueAttribute(1.0f, 1000.0f)),
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
    EZ_MESSAGE_HANDLER(ezMsgAnimationPoseGeneration, OnMsgAnimationPoseGeneration),
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
    EZ_SCRIPT_FUNCTION_PROPERTY(SetJointTypeOverride, In, "sJointName", In, "overrideType"),
    EZ_SCRIPT_FUNCTION_PROPERTY(SetJointMotorStrength, In, "fStrength"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetJointMotorStrength),
    EZ_SCRIPT_FUNCTION_PROPERTY(FadeJointMotorStrength, In, "fTargetStrength", In, "tDuration"),
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
    desc.m_bOnlyUpdateWhenSimulating = true;

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

void ezJoltRagdollComponentManager::DriveAnimatedRagdolls(ezTime deltaTime)
{
  EZ_PROFILE_SCOPE("DriveAnimatedRagdolls");

  for (auto it = GetComponents(0); it.IsValid(); it.Next())
  {
    it->DriveAnimated(deltaTime);
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
  s << m_uiWeightCategory;
  s << (float)m_fWeightScale;
  s << (float)m_fWeightMass;
  s << m_fStiffnessFactor;
  s << m_AnimMode;
}

void ezJoltRagdollComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  EZ_ASSERT_DEBUG(uiVersion >= 4, "Outdated version, please re-transform asset.");
  if (uiVersion < 4)
    return;

  s >> m_StartMode;
  s >> m_fGravityFactor;
  s >> m_bSelfCollision;
  s >> m_fOwnerVelocityScale;
  s >> m_fCenterVelocity;
  s >> m_fCenterAngularVelocity;
  s >> m_vCenterPosition;
  s >> m_uiWeightCategory;

  {
    float f;

    s >> f;
    m_fWeightScale = f;

    s >> f;
    m_fWeightMass = f;
  }

  s >> m_fStiffnessFactor;

  if (uiVersion >= 5)
  {
    s >> m_AnimMode;
  }
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

  if (m_pSkeletonPose)
  {
    auto pMan = static_cast<ezJoltRagdollComponentManager*>(GetOwningManager());

    EZ_LOCK(pMan->m_SkeletonsMutex);
    pMan->m_FreeSkeletonPoses.PushBack(std::move(m_pSkeletonPose));
    m_pSkeletonPose.Clear();
  }

  SUPER::OnDeactivated();
}

void ezJoltRagdollComponent::Update(bool bForce)
{
  if (!HasCreatedLimbs())
    return;

  if (m_AnimMode != ezJoltRagdollAnimMode::Powered && m_bIsPowered)
  {
    ResetJointMotors();
  }

  if (m_MotorLerpDuration.IsPositive())
  {
    const ezTime tNow = GetWorld()->GetClock().GetAccumulatedTime();
    const ezTime tDiff = GetWorld()->GetClock().GetTimeDiff();
    const ezTime tStart = tNow - tDiff;
    const ezTime tFinish = tStart + m_MotorLerpDuration;

    const float fLerpFactor = ezMath::Saturate(ezMath::Unlerp(tStart.GetSeconds(), tFinish.GetSeconds(), tNow.GetSeconds()));
    const float fNewStrength = ezMath::Lerp(m_fMotorStrength, m_fMotorTargetStrength, fLerpFactor);

    if (m_fMotorStrength != fNewStrength)
    {
      m_fMotorStrength = fNewStrength;
      m_MotorLerpDuration -= tDiff;

      if (m_MotorLerpDuration.IsNegative())
        m_MotorLerpDuration = ezTime::MakeZero();

      ApplyJointMotorStrength(fNewStrength);
    }
  }

  if (m_pSkeletonPose && (m_AnimMode != ezJoltRagdollAnimMode::Controlled) && (m_AnimMode != ezJoltRagdollAnimMode::Powered || m_fMotorStrength <= 0.0f))
  {
    auto pMan = static_cast<ezJoltRagdollComponentManager*>(GetOwningManager());

    EZ_LOCK(pMan->m_SkeletonsMutex);
    pMan->m_FreeSkeletonPoses.PushBack(std::move(m_pSkeletonPose));
    m_pSkeletonPose.Clear();
  }

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

  const ezVec3 vRootPos = RetrieveRagdollPose();
  GetOwner()->SetGlobalPosition(vRootPos);

  SendAnimationPoseMsg();

  m_ElapsedTimeSinceUpdate = ezTime::MakeZero();
}

void ezJoltRagdollComponent::DriveAnimated(ezTime deltaTime)
{
  if (m_pSkeletonPose)
  {
    m_pSkeletonPose->CalculateJointStates();

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    for (auto& joint : m_pSkeletonPose->GetJoints())
    {
      joint.mRotation = joint.mRotation.Normalized();
    }
#endif

    if (m_AnimMode == ezJoltRagdollAnimMode::Controlled)
    {
      m_pRagdoll->DriveToPoseUsingKinematics(*m_pSkeletonPose, deltaTime.AsFloatInSeconds());
    }
    else if (m_AnimMode == ezJoltRagdollAnimMode::Powered && m_fMotorStrength > 0.0f)
    {
      m_bIsPowered = true;
      m_pRagdoll->DriveToPoseUsingMotors(*m_pSkeletonPose);
    }
    else
    {
      m_pSkeletonPose.Clear();
    }
  }
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

  for (ezUInt32 i = 0; i < m_pRagdoll->GetBodyCount(); ++i)
  {
    m_pJoltSystem->GetBodyInterface().SetGravityFactor(m_pRagdoll->GetBodyID(i), m_fGravityFactor);
  }

  m_pRagdoll->Activate();
}

void ezJoltRagdollComponent::SetStartMode(ezEnum<ezJoltRagdollStartMode> mode)
{
  if (m_StartMode == mode)
    return;

  m_StartMode = mode;
}

void ezJoltRagdollComponent::SetAnimMode(ezEnum<ezJoltRagdollAnimMode> mode)
{
  if (m_AnimMode == mode)
    return;

  m_AnimMode = mode;

  if (m_pRagdoll)
  {
    if (m_AnimMode == ezJoltRagdollAnimMode::Controlled)
    {
      for (ezUInt32 i = 0; i < m_pRagdoll->GetBodyCount(); ++i)
      {
        // in the 'Controlled' mode, disable gravity, so that it doesn't affect the pose
        m_pJoltSystem->GetBodyInterface().SetGravityFactor(m_pRagdoll->GetBodyID(i), 0.0f);
      }
    }
    else
    {
      for (ezUInt32 i = 0; i < m_pRagdoll->GetBodyCount(); ++i)
      {
        m_pJoltSystem->GetBodyInterface().SetGravityFactor(m_pRagdoll->GetBodyID(i), m_fGravityFactor);
      }
    }
  }
}

void ezJoltRagdollComponent::OnMsgPhysicsAddImpulse(ezMsgPhysicsAddImpulse& ref_msg)
{
  const float fImpulse = ezJoltCore::GetImpulseTypeConfig().GetImpulseForWeight(ref_msg.m_uiImpulseType, m_uiWeightCategory);

  if (!HasCreatedLimbs())
  {
    m_vInitialImpulsePosition += ref_msg.m_vGlobalPosition;
    m_vInitialImpulseDirection += ref_msg.m_vImpulse * fImpulse;
    m_uiNumInitialImpulses++;
    return;
  }

  // TODO: normalize by number of limbs
  const ezUInt32 uiBodyId = reinterpret_cast<size_t>(ref_msg.m_pInternalPhysicsActor) & 0xFFFFFFFF;
  GetWorld()->GetModule<ezJoltWorldModule>()->AddImpulse(uiBodyId, ref_msg.m_vImpulse * fImpulse, ref_msg.m_vGlobalPosition);
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
      return;
    }
  }

  auto& jo = m_JointOverrides.ExpandAndGetRef();
  jo.m_sJointName = sJointNameHashed;
  jo.m_JointType = type;
}

void ezJoltRagdollComponent::OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& ref_poseMsg)
{
  if (!IsActiveAndSimulating())
    return;

  if (HasCreatedLimbs())
  {
    // Note: if this code is reached, although the ragdoll is supposed to use the animation (controlled or powered anim mode)
    // then the component that generates the animation doesn't have "Apply IK" enabled, ie it doesn't send ezMsgAnimationPoseGeneration
    ref_poseMsg.m_bContinueAnimating = false;

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

static void ComputeFullBoneTransform(const ezMat4& mRootTransform, const ezMat4& mModelTransform, ezTransform& out_transform)
{
  ezMat4 mFullTransform = mRootTransform * mModelTransform;

  out_transform.m_qRotation.ReconstructFromMat4(mFullTransform);
  out_transform.m_vScale.Set(1);
  out_transform.m_vPosition = mFullTransform.GetTranslationVector();
}

void ezJoltRagdollComponent::OnMsgAnimationPoseGeneration(ezMsgAnimationPoseGeneration& ref_msg)
{
  if (m_AnimMode == ezJoltRagdollAnimMode::Limp)
    return;

  if (!HasCreatedLimbs())
    return;

  if (ref_msg.m_uiOrderNow > 0xFF00)
    return;

  if (ref_msg.m_uiOrderNow < 0xFF00)
  {
    ref_msg.m_uiOrderNext = ezMath::Min<ezUInt16>(ref_msg.m_uiOrderNext, 0xFF00);
    return;
  }

  if (m_AnimMode == ezJoltRagdollAnimMode::Powered && m_fMotorStrength == 0.0f)
  {
    // basically the same as Limp mode, but we don't want to deactivate animations, because motor strength can still be changed
    return;
  }

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);
  const ezMat4 mRootTransform = pSkeleton->GetDescriptor().m_RootTransform.GetAsMat4();

  const ezTransform tGlobal = GetOwner()->GetGlobalTransform();
  const auto& curPose = ref_msg.m_pGenerator->GetCurrentPose();

  if (m_pSkeletonPose == nullptr)
  {
    auto pMan = static_cast<ezJoltRagdollComponentManager*>(GetOwningManager());
    EZ_LOCK(pMan->m_SkeletonsMutex);
    if (!pMan->m_FreeSkeletonPoses.IsEmpty())
    {
      m_pSkeletonPose = std::move(pMan->m_FreeSkeletonPoses.PeekBack());
      pMan->m_FreeSkeletonPoses.PopBack();
    }
    else
    {
      m_pSkeletonPose = EZ_NEW(ezFoundation::GetAlignedAllocator(), JPH::SkeletonPose);
    }
  }

  m_pSkeletonPose->SetSkeleton(m_pRagdoll->GetRagdollSettings()->GetSkeleton());

  ezVec3 vRootOffset(0);

  for (ezUInt32 uiLimbIdx = 0; uiLimbIdx < m_Limbs.GetCount(); ++uiLimbIdx)
  {
    if (m_Limbs[uiLimbIdx].m_uiPartIndex != ezInvalidJointIndex)
    {
      JPH::Mat44& jointMat = m_pSkeletonPose->GetJointMatrix(m_Limbs[uiLimbIdx].m_uiPartIndex);

      ezTransform trans;
      ComputeFullBoneTransform(mRootTransform, curPose[uiLimbIdx], trans);

      trans = ezTransform::MakeGlobalTransform(tGlobal, trans);

      ezMat4 mGlobal = trans.GetAsMat4();
      jointMat = JPH::Mat44::sLoadFloat4x4((const JPH::Float4*)mGlobal.m_fElementsCM);
    }
  }

  m_pSkeletonPose->SetRootOffset(ezJoltConversionUtils::ToVec3(vRootOffset));
}

void ezJoltRagdollComponent::SetJointMotorStrength(float fStrength)
{
  if (m_fMotorStrength != fStrength)
  {
    ApplyJointMotorStrength(fStrength);
  }

  m_fMotorStrength = fStrength;
  m_fMotorTargetStrength = fStrength;
  m_MotorLerpDuration = ezTime::MakeZero();
}

float ezJoltRagdollComponent::GetJointMotorStrength() const
{
  return m_fMotorStrength;
}

void ezJoltRagdollComponent::FadeJointMotorStrength(float fTargetStrength, ezTime duration)
{
  m_fMotorTargetStrength = fTargetStrength;
  m_MotorLerpDuration = duration;
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

ezVec3 ezJoltRagdollComponent::RetrieveRagdollPose()
{
  const float fLerpToPos = (m_AnimMode != ezJoltRagdollAnimMode::Controlled) ? 0.1f : 0.0f;

  const JPH::RVec3 vCurPosition = ezJoltConversionUtils::ToVec3(GetOwner()->GetGlobalPosition());

  const int body_count = (int)m_pRagdoll->GetBodyCount();
  JPH::BodyLockMultiRead lock(static_cast<const JPH::BodyLockInterface&>(m_pJoltSystem->GetBodyLockInterface()), m_pRagdoll->GetBodyIDs().data(), body_count);

  const JPH::Body* root = lock.GetBody(0);
  JPH::RMat44 root_transform = root->GetWorldTransform();
  JPH::RVec3 vRootOffset = root_transform.GetTranslation();
  vRootOffset = vCurPosition + (vRootOffset - vCurPosition) * fLerpToPos; // interpolate the object position towards the root bone position


  const ezVec3 vObjectScale = GetOwner()->GetGlobalScaling();
  const float fObjectScale = ezMath::Max(vObjectScale.x, vObjectScale.y, vObjectScale.z);

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);
  const ezSkeleton& skeleton = pSkeleton->GetDescriptor().m_Skeleton;
  const ezTransform rootTransform = pSkeleton->GetDescriptor().m_RootTransform;
  const ezQuat qGlobalRot = GetOwner()->GetGlobalRotation();
  const ezMat4 mInvScale = ezMat4::MakeScaling(ezVec3(1.0f).CompDiv(ezVec3(fObjectScale)));

  const ezMat4 mInv = m_mInvSkeletonRootTransform * qGlobalRot.GetInverse().GetAsMat4() * mInvScale;
  const ezMat4 mScale = ezMat4::MakeScaling(rootTransform.m_vScale * fObjectScale);

  ezHybridArray<ezMat4, 128> relativeTransforms;

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
      const JPH::Body* pBody = lock.GetBody(m_Limbs[uiLimbIdx].m_uiPartIndex);
      const JPH::RMat44 transform = pBody->GetWorldTransform();
      const JPH::Mat44 jointMatrix = JPH::Mat44(transform.GetColumn4(0), transform.GetColumn4(1), transform.GetColumn4(2), JPH::Vec4(JPH::Vec3(transform.GetTranslation() - vRootOffset), 1));

      const ezMat4& mPose = (const ezMat4&)jointMatrix;

      m_CurrentLimbTransforms[uiLimbIdx] = (mInv * mPose) * mScale;
    }
  }

  return ezJoltConversionUtils::ToVec3(vRootOffset);
}

void ezJoltRagdollComponent::CreateLimbsFromPose(const ezMsgAnimationPoseUpdated& pose)
{
  EZ_ASSERT_DEBUG(!HasCreatedLimbs(), "Limbs are already created.");

  if (EnsureSkeletonIsKnown().Failed())
    return;

  const ezVec3 vObjectScale = GetOwner()->GetGlobalScaling();
  const float fObjectScale = ezMath::Max(vObjectScale.x, vObjectScale.y, vObjectScale.z);

  ezJoltWorldModule& worldModule = *GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
  m_pJoltSystem = worldModule.GetJoltSystem(); // cache this for later
  m_uiObjectFilterID = worldModule.CreateObjectFilterID();
  m_uiJoltUserDataIndex = worldModule.AllocateUserData(m_pJoltUserData);
  m_pJoltUserData->Init(this);

  ezResourceLock<ezSkeletonResource> pSkeletonResource(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);

  m_mInvSkeletonRootTransform = pSkeletonResource->GetDescriptor().m_RootTransform.GetAsMat4().GetInverse();

  // allocate the limbs array
  m_Limbs.SetCount(pose.m_ModelTransforms.GetCount());

  JPH::Ref<JPH::RagdollSettings> ragdollSettings = new JPH::RagdollSettings();

  ragdollSettings->mParts.reserve(pSkeletonResource->GetDescriptor().m_Skeleton.GetJointCount());
  ragdollSettings->mSkeleton = new JPH::Skeleton(); // TODO: share this in the resource
  ragdollSettings->mSkeleton->GetJoints().reserve(ragdollSettings->mParts.size());

  CreateAllLimbs(*pSkeletonResource.GetPointer(), pose, worldModule, fObjectScale, ragdollSettings);

  {
    const float fInitialMass = ezJoltCore::GetWeightCategoryConfig().GetMassForWeightCategory(m_uiWeightCategory, 50.0f, m_fWeightMass, m_fWeightScale);

    ApplyBodyMass(ragdollSettings, fInitialMass);
  }

  SetupLimbJoints(pSkeletonResource.GetPointer(), ragdollSettings);
  ApplyPartInitialVelocity(ragdollSettings);

  if (m_bSelfCollision)
  {
    // enables collisions between all bodies except the ones that are directly connected to each other
    ragdollSettings->DisableParentChildCollisions();
  }

  ragdollSettings->Stabilize();
  ragdollSettings->CalculateBodyIndexToConstraintIndex();
  ragdollSettings->CalculateConstraintIndexToBodyIdxPair();

  m_pRagdoll = ragdollSettings->CreateRagdoll(m_uiObjectFilterID, reinterpret_cast<ezUInt64>(m_pJoltUserData), m_pJoltSystem);

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
  pPart->mGravityFactor = m_AnimMode == ezJoltRagdollAnimMode::Controlled ? 0.0f : m_fGravityFactor;
  pPart->mUserData = reinterpret_cast<ezUInt64>(m_pJoltUserData);
  pPart->mObjectLayer = ezJoltCollisionFiltering::ConstructObjectLayer(uiCollisionLayer, ezJoltBroadphaseLayer::Ragdoll);
  pPart->mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  pPart->mCollisionGroup.SetGroupFilter(worldModule.GetGroupFilterIgnoreSame()); // this is used if m_bSelfCollision is off, otherwise it gets overridden below
}

void ezJoltRagdollComponent::ApplyPartInitialVelocity(JPH::RagdollSettings* pRagdollSettings)
{
  JPH::Vec3 vCommonVelocity = ezJoltConversionUtils::ToVec3(GetOwner()->GetLinearVelocity() * m_fOwnerVelocityScale);
  const JPH::Vec3 vCenterPos = ezJoltConversionUtils::ToVec3(GetOwner()->GetGlobalTransform() * m_vCenterPosition);

  ezCoordinateSystem coord;
  GetWorld()->GetCoordinateSystem(GetOwner()->GetGlobalPosition(), coord);
  ezRandom& rng = GetOwner()->GetWorld()->GetRandomNumberGenerator();

  for (JPH::RagdollSettings::Part& part : pRagdollSettings->mParts)
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

void ezJoltRagdollComponent::ResetJointMotors()
{
  if (!m_bIsPowered)
    return;

  m_bIsPowered = false;

  if (!m_pRagdoll)
    return;

  for (int i = 0; i < (int)m_pRagdoll->GetConstraintCount(); ++i)
  {
    JPH::TwoBodyConstraint* pConstraint = m_pRagdoll->GetConstraint(i);

    JPH::EConstraintSubType subType = pConstraint->GetSubType();
    if (subType == JPH::EConstraintSubType::SwingTwist)
    {
      JPH::SwingTwistConstraint* pStConstraint = static_cast<JPH::SwingTwistConstraint*>(pConstraint);
      pStConstraint->SetSwingMotorState(JPH::EMotorState::Off);
      pStConstraint->SetTwistMotorState(JPH::EMotorState::Off);
    }
  }
}

void ezJoltRagdollComponent::ApplyJointMotorStrength(float fStrength)
{
  if (!m_pRagdoll)
    return;

  for (int i = 0; i < (int)m_pRagdoll->GetConstraintCount(); ++i)
  {
    JPH::TwoBodyConstraint* pConstraint = m_pRagdoll->GetConstraint(i);

    JPH::EConstraintSubType subType = pConstraint->GetSubType();
    if (subType == JPH::EConstraintSubType::SwingTwist)
    {
      JPH::SwingTwistConstraint* pStConstraint = static_cast<JPH::SwingTwistConstraint*>(pConstraint);

      pStConstraint->GetSwingMotorSettings().SetForceLimit(m_fMotorStrength);
      pStConstraint->GetTwistMotorSettings().SetForceLimit(m_fMotorStrength);

      // torque is needed for the 'powered' animation mode
      pStConstraint->GetSwingMotorSettings().SetTorqueLimit(m_fMotorStrength);
      pStConstraint->GetTwistMotorSettings().SetTorqueLimit(m_fMotorStrength);
    }
  }
}

void ezJoltRagdollComponent::ApplyBodyMass(JPH::RagdollSettings* pRagdollSettings, float fMass)
{
  if (fMass <= 0.0f)
    return;

  float fPartMass = fMass / pRagdollSettings->mParts.size();

  for (auto& part : pRagdollSettings->mParts)
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

void ezJoltRagdollComponent::CreateAllLimbs(const ezSkeletonResource& skeletonResource, const ezMsgAnimationPoseUpdated& pose, ezJoltWorldModule& worldModule, float fObjectScale, JPH::RagdollSettings* pRagdollSettings)
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
      CreateLimb(skeletonResource, limbConstructionInfos, geometries, pose, worldModule, fObjectScale, pRagdollSettings);
      geometries.Clear();
      uiLastLimbIdx = geo.m_uiAttachedToJoint;
    }

    geometries.PushBack(&geo);
  }

  CreateLimb(skeletonResource, limbConstructionInfos, geometries, pose, worldModule, fObjectScale, pRagdollSettings);
}

void ezJoltRagdollComponent::CreateLimb(const ezSkeletonResource& skeletonResource, ezMap<ezUInt16, LimbConstructionInfo>& limbConstructionInfos, ezArrayPtr<const ezSkeletonResourceGeometry*> geometries, const ezMsgAnimationPoseUpdated& pose, ezJoltWorldModule& worldModule, float fObjectScale, JPH::RagdollSettings* pRagdollSettings)
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

  thisLimbInfo.m_uiJoltPartIndex = (ezUInt16)pRagdollSettings->mParts.size();
  pRagdollSettings->mParts.resize(pRagdollSettings->mParts.size() + 1);

  m_Limbs[uiThisJointIdx].m_uiPartIndex = thisLimbInfo.m_uiJoltPartIndex;

  pRagdollSettings->mSkeleton->AddJoint(thisLimbJoint.GetName().GetData(), parentLimbInfo.m_uiJoltPartIndex != ezInvalidJointIndex ? parentLimbInfo.m_uiJoltPartIndex : -1);

  ComputeLimbGlobalTransform(thisLimbInfo.m_GlobalTransform, pose, uiThisJointIdx);
  ConfigureRagdollPart(&pRagdollSettings->mParts[thisLimbInfo.m_uiJoltPartIndex], thisLimbInfo.m_GlobalTransform, thisLimbJoint.GetCollisionLayer(), worldModule);
  CreateAllLimbGeoShapes(thisLimbInfo, geometries, thisLimbJoint, skeletonResource, fObjectScale, pRagdollSettings);
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
      ezVec3 vHalfSize = geo.m_Transform.m_vScale * 0.5f * fObjectScale;
      vHalfSize.x = ezMath::Max(vHalfSize.x, JPH::cDefaultConvexRadius);
      vHalfSize.y = ezMath::Max(vHalfSize.y, JPH::cDefaultConvexRadius);
      vHalfSize.z = ezMath::Max(vHalfSize.z, JPH::cDefaultConvexRadius);
      shape.mHalfExtent = ezJoltConversionUtils::ToVec3(vHalfSize);

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

    case ezSkeletonJointGeometryType::CapsuleSideways:
    {
      JPH::CapsuleShapeSettings shape;
      shape.mUserData = reinterpret_cast<ezUInt64>(m_pJoltUserData);
      shape.mMaterial = pJoltMaterial;
      shape.mHalfHeightOfCylinder = geo.m_Transform.m_vScale.x * 0.5f * fObjectScale;
      shape.mRadius = geo.m_Transform.m_vScale.z * fObjectScale;

      // ezQuat qRot = ezQuat::MakeFromAxisAndAngle(ezVec3::MakeAxisZ(), ezAngle::MakeFromDegree(-90));
      out_shapeTransform.m_qRotation = out_shapeTransform.m_qRotation; // *qRot;
      // out_shapeTransform.m_vPosition += qBoneDirAdjustment * ezVec3(geo.m_Transform.m_vScale.x * 0.5f * fObjectScale, 0, 0);

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

void ezJoltRagdollComponent::CreateAllLimbGeoShapes(const LimbConstructionInfo& limbConstructionInfo, ezArrayPtr<const ezSkeletonResourceGeometry*> geometries, const ezSkeletonJoint& thisLimbJoint, const ezSkeletonResource& skeletonResource, float fObjectScale, JPH::RagdollSettings* pRagdollSettings)
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

  JPH::RagdollSettings::Part* pBodyDesc = &pRagdollSettings->mParts[limbConstructionInfo.m_uiJoltPartIndex];

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


void ezJoltRagdollComponent::SetupLimbJoints(const ezSkeletonResource* pSkeleton, JPH::RagdollSettings* pRagdollSettings)
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

    CreateLimbJoint(thisJoint, &pRagdollSettings->mParts[parentLimb.m_uiPartIndex], &pRagdollSettings->mParts[thisLimb.m_uiPartIndex]);
  }
}

void ezJoltRagdollComponent::CreateLimbJoint(const ezSkeletonJoint& thisJoint, void* pParentBodyDesc, void* pThisBodyDesc)
{
  ezEnum<ezSkeletonJointType> jointType = thisJoint.GetJointType();

  for (ezUInt32 i = 0; i < m_JointOverrides.GetCount(); ++i)
  {
    if (m_JointOverrides[i].m_sJointName == thisJoint.GetName())
    {
      jointType = m_JointOverrides[i].m_JointType;
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

    pJoint->mSpace = JPH::EConstraintSpace::WorldSpace;
    pJoint->mDrawConstraintSize = 0.15f;
    pJoint->mPosition1 = pLink->mPosition;
    pJoint->mPosition2 = pLink->mPosition;
    pJoint->mNormalHalfConeAngle = thisJoint.GetHalfSwingLimitZ().GetRadian();
    pJoint->mPlaneHalfConeAngle = thisJoint.GetHalfSwingLimitY().GetRadian();
    pJoint->mTwistMinAngle = thisJoint.GetTwistLimitLow().GetRadian();
    pJoint->mTwistMaxAngle = thisJoint.GetTwistLimitHigh().GetRadian();
    pJoint->mMaxFrictionTorque = m_fStiffnessFactor * thisJoint.GetStiffness();
    pJoint->mPlaneAxis1 = ezJoltConversionUtils::ToVec3(tParent.m_qRotation * offsetRot * ezVec3::MakeAxisZ()).Normalized();
    pJoint->mPlaneAxis2 = ezJoltConversionUtils::ToVec3(tThis.m_qRotation * ezVec3::MakeAxisZ()).Normalized();
    pJoint->mTwistAxis1 = ezJoltConversionUtils::ToVec3(tParent.m_qRotation * offsetRot * ezVec3::MakeAxisY()).Normalized();
    pJoint->mTwistAxis2 = ezJoltConversionUtils::ToVec3(tThis.m_qRotation * ezVec3::MakeAxisY()).Normalized();

    pJoint->mSwingMotorSettings.mSpringSettings.mFrequency = 20;
    pJoint->mSwingMotorSettings.mSpringSettings.mStiffness = 20;
    pJoint->mSwingMotorSettings.mSpringSettings.mDamping = 2;

    pJoint->mSwingMotorSettings.SetForceLimit(m_fMotorStrength);
    pJoint->mTwistMotorSettings.SetForceLimit(m_fMotorStrength);

    // torque is needed for the 'powered' animation mode
    pJoint->mSwingMotorSettings.SetTorqueLimit(m_fMotorStrength);
    pJoint->mTwistMotorSettings.SetTorqueLimit(m_fMotorStrength);
  }
}

EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltRagdollComponent);
