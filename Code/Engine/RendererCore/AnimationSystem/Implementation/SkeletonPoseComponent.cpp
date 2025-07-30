#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/SkeletonPoseComponent.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezSkeletonPoseMode, 1)
  EZ_ENUM_CONSTANTS(ezSkeletonPoseMode::CustomPose, ezSkeletonPoseMode::RestPose, ezSkeletonPoseMode::Disabled)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_COMPONENT_TYPE(ezSkeletonPoseComponent, 4, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_ACCESSOR_PROPERTY("Skeleton", GetSkeleton, SetSkeleton)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Mesh_Skeleton")),
    EZ_ENUM_ACCESSOR_PROPERTY("Mode", ezSkeletonPoseMode, GetPoseMode, SetPoseMode),
    EZ_MEMBER_PROPERTY("EditBones", m_fDummy),
    EZ_MAP_ACCESSOR_PROPERTY("Bones", GetBones, GetBone, SetBone, RemoveBone)->AddAttributes(new ezExposedParametersAttribute("Skeleton"), new ezContainerAttribute(false, true, false)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Animation"),
    new ezBoneManipulatorAttribute("Bones", "EditBones"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSkeletonPoseComponent::ezSkeletonPoseComponent() = default;
ezSkeletonPoseComponent::~ezSkeletonPoseComponent() = default;

void ezSkeletonPoseComponent::Update()
{
  if (m_uiResendPose == 0)
    return;

  if (--m_uiResendPose > 0)
  {
    static_cast<ezSkeletonPoseComponentManager*>(GetOwningManager())->EnqueueUpdate(GetHandle());
  }

  if (m_PoseMode == ezSkeletonPoseMode::RestPose)
  {
    SendRestPose();
    return;
  }

  if (m_PoseMode == ezSkeletonPoseMode::CustomPose)
  {
    SendCustomPose();
    return;
  }
}

void ezSkeletonPoseComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_hSkeleton;
  s << m_PoseMode;

  m_Bones.Sort();
  ezUInt16 numBones = static_cast<ezUInt16>(m_Bones.GetCount());
  s << numBones;

  for (ezUInt16 i = 0; i < numBones; ++i)
  {
    s << m_Bones.GetKey(i);
    s << m_Bones.GetValue(i).m_sName;
    s << m_Bones.GetValue(i).m_sParent;
    s << m_Bones.GetValue(i).m_Transform;
  }
}

void ezSkeletonPoseComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_hSkeleton;
  s >> m_PoseMode;

  ezHashedString sKey;
  ezExposedBone bone;

  ezUInt16 numBones = 0;
  s >> numBones;
  m_Bones.Reserve(numBones);

  for (ezUInt16 i = 0; i < numBones; ++i)
  {
    s >> sKey;
    s >> bone.m_sName;
    s >> bone.m_sParent;
    s >> bone.m_Transform;

    m_Bones[sKey] = bone;
  }
  ResendPose();
}

void ezSkeletonPoseComponent::OnActivated()
{
  SUPER::OnActivated();

  ResendPose();
}

void ezSkeletonPoseComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  ResendPose();
}

void ezSkeletonPoseComponent::SetSkeleton(const ezSkeletonResourceHandle& hResource)
{
  if (m_hSkeleton != hResource)
  {
    m_hSkeleton = hResource;
    ResendPose();
  }
}

void ezSkeletonPoseComponent::SetPoseMode(ezEnum<ezSkeletonPoseMode> mode)
{
  m_PoseMode = mode;
  ResendPose();
}

void ezSkeletonPoseComponent::ResendPose()
{
  if (m_uiResendPose == 2)
    return;

  m_uiResendPose = 2;
  static_cast<ezSkeletonPoseComponentManager*>(GetOwningManager())->EnqueueUpdate(GetHandle());
}

const ezRangeView<const char*, ezUInt32> ezSkeletonPoseComponent::GetBones() const
{
  return ezRangeView<const char*, ezUInt32>([]() -> ezUInt32
    { return 0; },
    [this]() -> ezUInt32
    { return m_Bones.GetCount(); },
    [](ezUInt32& ref_uiIt)
    { ++ref_uiIt; },
    [this](const ezUInt32& uiIt) -> const char*
    { return m_Bones.GetKey(uiIt).GetString().GetData(); });
}

void ezSkeletonPoseComponent::SetBone(const char* szKey, const ezVariant& value)
{
  ezHashedString hs;
  hs.Assign(szKey);

  if (value.GetReflectedType() == ezGetStaticRTTI<ezExposedBone>())
  {
    m_Bones[hs] = *reinterpret_cast<const ezExposedBone*>(value.GetData());
  }

  // TODO
  // if (IsActiveAndInitialized())
  //{
  //  // only add to update list, if not yet activated,
  //  // since OnActivate will do the instantiation anyway
  //  GetWorld()->GetComponentManager<ezPrefabReferenceComponentManager>()->AddToUpdateList(this);
  //}
  ResendPose();
}

void ezSkeletonPoseComponent::RemoveBone(const char* szKey)
{
  if (m_Bones.RemoveAndCopy(ezTempHashedString(szKey)))
  {
    // TODO
    // if (IsActiveAndInitialized())
    //{
    //  // only add to update list, if not yet activated,
    //  // since OnActivate will do the instantiation anyway
    //  GetWorld()->GetComponentManager<ezPrefabReferenceComponentManager>()->AddToUpdateList(this);
    //}

    ResendPose();
  }
}

bool ezSkeletonPoseComponent::GetBone(const char* szKey, ezVariant& out_value) const
{
  ezUInt32 it = m_Bones.Find(szKey);

  if (it == ezInvalidIndex)
    return false;

  out_value.CopyTypedObject(&m_Bones.GetValue(it), ezGetStaticRTTI<ezExposedBone>());
  return true;
}

void ezSkeletonPoseComponent::SendRestPose()
{
  if (!m_hSkeleton.IsValid())
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pSkeleton.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  const auto& desc = pSkeleton->GetDescriptor();
  const auto& skel = desc.m_Skeleton;

  if (skel.GetJointCount() == 0)
    return;

  ezArrayPtr<ozz::math::Float4x4> pFinalTransforms = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ozz::math::Float4x4, skel.GetJointCount());
  EZ_ASSERT_DEBUG(ezMemoryUtils::IsAligned(pFinalTransforms.GetPtr(), alignof(ozz::math::Float4x4)), "Unaligned cast");

  {
    ozz::animation::LocalToModelJob job;
    job.input = skel.GetOzzSkeleton().joint_rest_poses();
    job.output = ozz::span<ozz::math::Float4x4>(pFinalTransforms.GetPtr(), pFinalTransforms.GetEndPtr());
    job.skeleton = &skel.GetOzzSkeleton();
    job.Run();
  }

  ezMsgAnimationPoseUpdated msg;
  msg.m_pRootTransform = &desc.m_RootTransform;
  msg.m_pSkeleton = &skel;
  msg.m_ModelTransforms = ezMakeArrayPtr(reinterpret_cast<const ezMat4*>(pFinalTransforms.GetPtr()), pFinalTransforms.GetCount());

  GetOwner()->SendMessageRecursive(msg);

  if (msg.m_bContinueAnimating == false)
    m_PoseMode = ezSkeletonPoseMode::Disabled;
}

void ezSkeletonPoseComponent::SendCustomPose()
{
  if (!m_hSkeleton.IsValid())
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);
  const auto& desc = pSkeleton->GetDescriptor();
  const auto& skel = desc.m_Skeleton;

  ezArrayPtr<ozz::math::Float4x4> pFinalTransforms = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ozz::math::Float4x4, skel.GetJointCount());
  EZ_ASSERT_DEBUG(ezMemoryUtils::IsAligned(pFinalTransforms.GetPtr(), alignof(ozz::math::Float4x4)), "Unaligned cast");

  for (ezUInt32 i = 0; i < pFinalTransforms.GetCount(); ++i)
  {
    pFinalTransforms[i] = ozz::math::Float4x4::identity();
  }

  ozz::vector<ozz::math::SoaTransform> ozzLocalTransforms;
  ozzLocalTransforms.resize((skel.GetJointCount() + 3) / 4);

  auto restPoses = skel.GetOzzSkeleton().joint_rest_poses();

  // initialize the skeleton with the rest pose
  for (ezUInt32 i = 0; i < ozzLocalTransforms.size(); ++i)
  {
    ozzLocalTransforms[i] = restPoses[i];
  }

  for (const auto& boneIt : m_Bones)
  {
    const ezUInt16 uiBone = skel.FindJointByName(boneIt.key);
    if (uiBone == ezInvalidJointIndex)
      continue;

    const ezExposedBone& thisBone = boneIt.value;

    // this can happen when the property was reverted
    if (thisBone.m_sName.IsEmpty() || thisBone.m_sParent.IsEmpty())
      continue;

    EZ_ASSERT_DEBUG(!thisBone.m_Transform.m_qRotation.IsNaN(), "Invalid bone transform in pose component");

    const ezQuat& boneRot = thisBone.m_Transform.m_qRotation;

    const ezUInt32 idx0 = uiBone / 4;
    const ezUInt32 idx1 = uiBone % 4;

    ozz::math::SoaQuaternion& q = ozzLocalTransforms[idx0].rotation;
    reinterpret_cast<float*>(&q.x)[idx1] = boneRot.x;
    reinterpret_cast<float*>(&q.y)[idx1] = boneRot.y;
    reinterpret_cast<float*>(&q.z)[idx1] = boneRot.z;
    reinterpret_cast<float*>(&q.w)[idx1] = boneRot.w;
  }

  EZ_ASSERT_DEBUG(ezMemoryUtils::IsAligned(pFinalTransforms.GetPtr(), alignof(ozz::math::Float4x4)), "Unaligned cast");
  ozz::animation::LocalToModelJob job;
  job.input = ozz::span<const ozz::math::SoaTransform>(ozzLocalTransforms.data(), ozzLocalTransforms.size());
  job.output = ozz::span<ozz::math::Float4x4>(pFinalTransforms.GetPtr(), pFinalTransforms.GetEndPtr());
  job.skeleton = &skel.GetOzzSkeleton();
  EZ_ASSERT_DEBUG(job.Validate(), "");
  job.Run();


  ezMsgAnimationPoseUpdated msg;
  msg.m_pRootTransform = &desc.m_RootTransform;
  msg.m_pSkeleton = &skel;
  msg.m_ModelTransforms = ezMakeArrayPtr(reinterpret_cast<const ezMat4*>(pFinalTransforms.GetPtr()), pFinalTransforms.GetCount());

  GetOwner()->SendMessageRecursive(msg);

  if (msg.m_bContinueAnimating == false)
    m_PoseMode = ezSkeletonPoseMode::Disabled;
}

//////////////////////////////////////////////////////////////////////////

void ezSkeletonPoseComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  ezDeque<ezComponentHandle> requireUpdate;

  {
    EZ_LOCK(m_Mutex);
    requireUpdate.Swap(m_RequireUpdate);
  }

  for (const auto& hComp : requireUpdate)
  {
    ezSkeletonPoseComponent* pComp = nullptr;
    if (!TryGetComponent(hComp, pComp) || !pComp->IsActiveAndInitialized())
      continue;

    pComp->Update();
  }
}

void ezSkeletonPoseComponentManager::EnqueueUpdate(ezComponentHandle hComponent)
{
  EZ_LOCK(m_Mutex);

  if (m_RequireUpdate.IndexOf(hComponent) != ezInvalidIndex)
    return;

  m_RequireUpdate.PushBack(hComponent);
}

void ezSkeletonPoseComponentManager::Initialize()
{
  SUPER::Initialize();

  ezWorldModule::UpdateFunctionDesc desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezSkeletonPoseComponentManager::Update, this);
  desc.m_Phase = ezWorldUpdatePhase::PreAsync;

  RegisterUpdateFunction(desc);
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_SkeletonPoseComponent);
