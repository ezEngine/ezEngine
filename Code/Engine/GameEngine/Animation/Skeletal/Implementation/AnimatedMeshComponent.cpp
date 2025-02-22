#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererFoundation/Device/Device.h>

#include <RendererCore/RenderWorld/RenderWorld.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/span.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezAnimatedMeshComponent, 13, ezComponentMode::Dynamic); // TODO: why dynamic ? (I guess because the overridden CreateRenderData() has to be called every frame)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_ACCESSOR_PROPERTY("Mesh", GetMesh, SetMesh)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Mesh_Skinned")),
    EZ_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new ezExposeColorAlphaAttribute()),
    EZ_ACCESSOR_PROPERTY("CustomData", GetCustomData, SetCustomData)->AddAttributes(new ezDefaultValueAttribute(ezVec4(0, 1, 0, 1))),
    EZ_ARRAY_ACCESSOR_PROPERTY("Materials", Materials_GetCount, Materials_GetValue, Materials_SetValue, Materials_Insert, Materials_Remove)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material")),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
      new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES;

  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
    EZ_MESSAGE_HANDLER(ezMsgQueryAnimationSkeleton, OnQueryAnimationSkeleton),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezRootMotionMode, 1)
  EZ_ENUM_CONSTANTS(ezRootMotionMode::Ignore, ezRootMotionMode::ApplyToOwner, ezRootMotionMode::SendMoveCharacterMsg)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

ezAnimatedMeshComponent::ezAnimatedMeshComponent() = default;
ezAnimatedMeshComponent::~ezAnimatedMeshComponent() = default;

void ezAnimatedMeshComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
}

void ezAnimatedMeshComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  EZ_ASSERT_DEV(uiVersion >= 13, "Unsupported version, delete the file and reexport it");
}

void ezAnimatedMeshComponent::OnActivated()
{
  SUPER::OnActivated();

  InitializeAnimationPose();
}

void ezAnimatedMeshComponent::OnDeactivated()
{
  m_SkinningState.Clear();

  SUPER::OnDeactivated();
}

void ezAnimatedMeshComponent::InitializeAnimationPose()
{
  m_MaxBounds = ezBoundingBox::MakeInvalid();

  if (!m_hMesh.IsValid())
    return;

  ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::BlockTillLoaded);
  if (pMesh.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  m_hDefaultSkeleton = pMesh->m_hDefaultSkeleton;
  const auto hSkeleton = m_hDefaultSkeleton;

  if (!hSkeleton.IsValid())
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(hSkeleton, ezResourceAcquireMode::BlockTillLoaded);
  if (pSkeleton.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  {
    const ozz::animation::Skeleton* pOzzSkeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();
    const ezUInt32 uiNumSkeletonJoints = pOzzSkeleton->num_joints();

    ezArrayPtr<ozz::math::Float4x4> pPoseMatrices = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ozz::math::Float4x4, uiNumSkeletonJoints);
    EZ_ASSERT_DEBUG(ezMemoryUtils::IsAligned(pPoseMatrices.GetPtr(), alignof(ozz::math::Float4x4)), "Unaligned cast");
    {
      ozz::animation::LocalToModelJob job;
      job.input = pOzzSkeleton->joint_rest_poses();
      job.output = ozz::span<ozz::math::Float4x4>(pPoseMatrices.GetPtr(), pPoseMatrices.GetEndPtr());
      job.skeleton = pOzzSkeleton;
      job.Run();
    }

    ezMsgAnimationPoseUpdated msg;
    msg.m_ModelTransforms = ezMakeArrayPtr(reinterpret_cast<const ezMat4*>(pPoseMatrices.GetPtr()), pPoseMatrices.GetCount());
    msg.m_pRootTransform = &pSkeleton->GetDescriptor().m_RootTransform;
    msg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;

    OnAnimationPoseUpdated(msg);
  }

  TriggerLocalBoundsUpdate();
}


void ezAnimatedMeshComponent::MapModelSpacePoseToSkinningSpace(const ezHashTable<ezHashedString, ezMeshResourceDescriptor::BoneData>& bones, const ezSkeleton& skeleton, ezArrayPtr<const ezMat4> modelSpaceTransforms, ezBoundingBox* bounds)
{
  m_SkinningState.m_Transforms.SetCountUninitialized(bones.GetCount());

  if (bounds)
  {
    for (auto itBone : bones)
    {
      const ezUInt16 uiJointIdx = skeleton.FindJointByName(itBone.Key());

      if (uiJointIdx == ezInvalidJointIndex)
        continue;

      bounds->ExpandToInclude(modelSpaceTransforms[uiJointIdx].GetTranslationVector());
      m_SkinningState.m_Transforms[itBone.Value().m_uiBoneIndex] = modelSpaceTransforms[uiJointIdx] * itBone.Value().m_GlobalInverseRestPoseMatrix;
    }
  }
  else
  {
    for (auto itBone : bones)
    {
      const ezUInt16 uiJointIdx = skeleton.FindJointByName(itBone.Key());

      if (uiJointIdx == ezInvalidJointIndex)
        continue;

      m_SkinningState.m_Transforms[itBone.Value().m_uiBoneIndex] = modelSpaceTransforms[uiJointIdx] * itBone.Value().m_GlobalInverseRestPoseMatrix;
    }
  }
}

ezMeshRenderData* ezAnimatedMeshComponent::CreateRenderData() const
{
  auto pRenderData = ezCreateRenderDataForThisFrame<ezSkinnedMeshRenderData>(GetOwner());
  pRenderData->m_GlobalTransform = m_RootTransform;

  pRenderData->m_hSkinningTransforms = m_SkinningState.m_hGpuBuffer;

  return pRenderData;
}

void ezAnimatedMeshComponent::RetrievePose(ezDynamicArray<ezMat4>& out_modelTransforms, ezTransform& out_rootTransform, const ezSkeleton& skeleton)
{
  out_modelTransforms.Clear();

  if (!m_hMesh.IsValid())
    return;

  out_rootTransform = m_RootTransform;

  ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::BlockTillLoaded);

  const ezHashTable<ezHashedString, ezMeshResourceDescriptor::BoneData>& bones = pMesh->m_Bones;

  out_modelTransforms.SetCount(skeleton.GetJointCount(), ezMat4::MakeIdentity());

  for (auto itBone : bones)
  {
    const ezUInt16 uiJointIdx = skeleton.FindJointByName(itBone.Key());

    if (uiJointIdx == ezInvalidJointIndex)
      continue;

    out_modelTransforms[uiJointIdx] = m_SkinningState.m_Transforms[itBone.Value().m_uiBoneIndex].GetAsMat4() * itBone.Value().m_GlobalInverseRestPoseMatrix.GetInverse();
  }
}

void ezAnimatedMeshComponent::OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& msg)
{
  if (!m_hMesh.IsValid())
    return;

  m_RootTransform = *msg.m_pRootTransform;

  ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::BlockTillLoaded);

  ezBoundingBox poseBounds;
  poseBounds = ezBoundingBox::MakeInvalid();
  MapModelSpacePoseToSkinningSpace(pMesh->m_Bones, *msg.m_pSkeleton, msg.m_ModelTransforms, &poseBounds);

  if (poseBounds.IsValid() && (!m_MaxBounds.IsValid() || !m_MaxBounds.Contains(poseBounds)))
  {
    m_MaxBounds.ExpandToInclude(poseBounds);
    TriggerLocalBoundsUpdate();
  }
  else if (((ezRenderWorld::GetFrameCounter() + GetUniqueIdForRendering()) & (EZ_BIT(10) - 1)) == 0) // reset the bbox every once in a while
  {
    m_MaxBounds = poseBounds;
    TriggerLocalBoundsUpdate();
  }

  m_SkinningState.TransformsChanged();
}

void ezAnimatedMeshComponent::OnQueryAnimationSkeleton(ezMsgQueryAnimationSkeleton& msg)
{
  if (!msg.m_hSkeleton.IsValid() && m_hMesh.IsValid())
  {
    // only overwrite, if no one else had a better skeleton (e.g. the ezSkeletonComponent)

    ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::BlockTillLoaded);
    if (pMesh.GetAcquireResult() == ezResourceAcquireResult::Final)
    {
      msg.m_hSkeleton = pMesh->m_hDefaultSkeleton;
    }
  }
}

ezResult ezAnimatedMeshComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible, ezMsgUpdateLocalBounds& msg)
{
  if (!m_MaxBounds.IsValid() || !m_hMesh.IsValid())
    return EZ_FAILURE;

  ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::BlockTillLoaded);
  if (pMesh.GetAcquireResult() != ezResourceAcquireResult::Final)
    return EZ_FAILURE;

  ezBoundingBox bbox = m_MaxBounds;
  bbox.Grow(ezVec3(pMesh->m_fMaxBoneVertexOffset));
  bounds = ezBoundingBoxSphere::MakeFromBox(bbox);
  bounds.Transform(m_RootTransform.GetAsMat4());
  return EZ_SUCCESS;
}

void ezRootMotionMode::Apply(ezRootMotionMode::Enum mode, ezGameObject* pObject, const ezVec3& vTranslation, ezAngle rotationX, ezAngle rotationY, ezAngle rotationZ)
{
  switch (mode)
  {
    case ezRootMotionMode::Ignore:
      return;

    case ezRootMotionMode::ApplyToOwner:
    {
      ezVec3 vNewPos = pObject->GetLocalPosition();
      vNewPos += pObject->GetLocalRotation() * vTranslation;
      pObject->SetLocalPosition(vNewPos);

      // not tested whether this is actually correct
      ezQuat rotation = ezQuat::MakeFromEulerAngles(rotationX, rotationY, rotationZ);

      pObject->SetLocalRotation(rotation * pObject->GetLocalRotation());

      return;
    }

    case ezRootMotionMode::SendMoveCharacterMsg:
    {
      ezMsgApplyRootMotion msg;
      msg.m_vTranslation = vTranslation;
      msg.m_RotationX = rotationX;
      msg.m_RotationY = rotationY;
      msg.m_RotationZ = rotationZ;

      while (pObject != nullptr)
      {
        pObject->SendMessage(msg);
        pObject = pObject->GetParent();
      }

      return;
    }
  }
}

//////////////////////////////////////////////////////////////////////////


ezAnimatedMeshComponentManager::ezAnimatedMeshComponentManager(ezWorld* pWorld)
  : ezComponentManager<ComponentType, ezBlockStorageType::FreeList>(pWorld)
{
  ezResourceManager::GetResourceEvents().AddEventHandler(ezMakeDelegate(&ezAnimatedMeshComponentManager::ResourceEventHandler, this));
}

ezAnimatedMeshComponentManager::~ezAnimatedMeshComponentManager()
{
  ezResourceManager::GetResourceEvents().RemoveEventHandler(ezMakeDelegate(&ezAnimatedMeshComponentManager::ResourceEventHandler, this));
}

void ezAnimatedMeshComponentManager::Initialize()
{
  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezAnimatedMeshComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void ezAnimatedMeshComponentManager::ResourceEventHandler(const ezResourceEvent& e)
{
  if (e.m_Type == ezResourceEvent::Type::ResourceContentUnloading)
  {
    if (ezMeshResource* pResource = ezDynamicCast<ezMeshResource*>(e.m_pResource))
    {
      ezMeshResourceHandle hMesh(pResource);

      for (auto it = GetComponents(); it.IsValid(); it.Next())
      {
        if (it->m_hMesh == hMesh)
        {
          AddToUpdateList(it);
        }
      }
    }

    if (ezSkeletonResource* pResource = ezDynamicCast<ezSkeletonResource*>(e.m_pResource))
    {
      ezSkeletonResourceHandle hSkeleton(pResource);

      for (auto it = GetComponents(); it.IsValid(); it.Next())
      {
        if (it->m_hDefaultSkeleton == hSkeleton)
        {
          AddToUpdateList(it);
        }
      }
    }
  }
}

void ezAnimatedMeshComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  for (auto hComp : m_ComponentsToUpdate)
  {
    ezAnimatedMeshComponent* pComponent = nullptr;
    if (!TryGetComponent(hComp, pComponent))
      continue;

    if (!pComponent->IsActive())
      continue;

    pComponent->InitializeAnimationPose();
  }

  m_ComponentsToUpdate.Clear();
}

void ezAnimatedMeshComponentManager::AddToUpdateList(ezAnimatedMeshComponent* pComponent)
{
  ezComponentHandle hComponent = pComponent->GetHandle();

  if (m_ComponentsToUpdate.IndexOf(hComponent) == ezInvalidIndex)
  {
    m_ComponentsToUpdate.PushBack(hComponent);
  }
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_AnimatedMeshComponent);
