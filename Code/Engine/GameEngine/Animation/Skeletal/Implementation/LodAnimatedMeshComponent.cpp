#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/LodAnimatedMeshComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/span.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezLodAnimatedMeshLod, ezNoBase, 2, ezRTTIDefaultAllocator<ezLodAnimatedMeshLod>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_MEMBER_PROPERTY("Mesh", m_hMesh)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Mesh_Skinned")),
    EZ_MEMBER_PROPERTY("Threshold", m_fThreshold)
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_COMPONENT_TYPE(ezLodAnimatedMeshComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new ezExposeColorAlphaAttribute()),
    EZ_ACCESSOR_PROPERTY("CustomData", GetCustomData, SetCustomData)->AddAttributes(new ezDefaultValueAttribute(ezVec4(0, 1, 0, 1))),
    EZ_ACCESSOR_PROPERTY("SortingDepthOffset", GetSortingDepthOffset, SetSortingDepthOffset),
    EZ_MEMBER_PROPERTY("BoundsOffset", m_vBoundsOffset),
    EZ_MEMBER_PROPERTY("BoundsRadius", m_fBoundsRadius)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.01f, 100.0f)),
    EZ_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    EZ_ACCESSOR_PROPERTY("OverlapRanges", GetOverlapRanges, SetOverlapRanges)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ARRAY_MEMBER_PROPERTY("Meshes", m_Meshes),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Animation"),
    new ezSphereVisualizerAttribute("BoundsRadius", ezColor::MediumVioletRed, nullptr, ezVisualizerAnchor::Center, ezVec3(1.0f), "BoundsOffset"),
    new ezTransformManipulatorAttribute("BoundsOffset"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
    EZ_MESSAGE_HANDLER(ezMsgSetColor, OnMsgSetColor),
    EZ_MESSAGE_HANDLER(ezMsgSetCustomData, OnMsgSetCustomData),
    EZ_MESSAGE_HANDLER(ezMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
    EZ_MESSAGE_HANDLER(ezMsgQueryAnimationSkeleton, OnQueryAnimationSkeleton),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

struct LodAnimatedMeshCompFlags
{
  enum Enum
  {
    ShowDebugInfo = 0,
    OverlapRanges = 1,
  };
};

ezLodAnimatedMeshComponent::ezLodAnimatedMeshComponent() = default;
ezLodAnimatedMeshComponent::~ezLodAnimatedMeshComponent() = default;

void ezLodAnimatedMeshComponent::SetShowDebugInfo(bool bShow)
{
  SetUserFlag(LodAnimatedMeshCompFlags::ShowDebugInfo, bShow);
}

bool ezLodAnimatedMeshComponent::GetShowDebugInfo() const
{
  return GetUserFlag(LodAnimatedMeshCompFlags::ShowDebugInfo);
}

void ezLodAnimatedMeshComponent::SetOverlapRanges(bool bShow)
{
  SetUserFlag(LodAnimatedMeshCompFlags::OverlapRanges, bShow);
}

bool ezLodAnimatedMeshComponent::GetOverlapRanges() const
{
  return GetUserFlag(LodAnimatedMeshCompFlags::OverlapRanges);
}

void ezLodAnimatedMeshComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  ezStreamWriter& s = inout_stream.GetStream();

  s << m_Meshes.GetCount();
  for (const auto& mesh : m_Meshes)
  {
    s << mesh.m_hMesh;
    s << mesh.m_fThreshold;
  }

  s << m_Color;
  s << m_fSortingDepthOffset;

  s << m_vBoundsOffset;
  s << m_fBoundsRadius;

  s << m_vCustomData;
}

void ezLodAnimatedMeshComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  ezStreamReader& s = inout_stream.GetStream();

  ezUInt32 uiMeshes = 0;
  s >> uiMeshes;

  m_Meshes.SetCount(uiMeshes);

  for (auto& mesh : m_Meshes)
  {
    s >> mesh.m_hMesh;
    s >> mesh.m_fThreshold;
  }

  s >> m_Color;
  s >> m_fSortingDepthOffset;

  s >> m_vBoundsOffset;
  s >> m_fBoundsRadius;

  if (uiVersion >= 2)
  {
    s >> m_vCustomData;
  }
}

ezResult ezLodAnimatedMeshComponent::GetLocalBounds(ezBoundingBoxSphere& out_bounds, bool& out_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  out_bounds = ezBoundingSphere::MakeFromCenterAndRadius(m_vBoundsOffset, m_fBoundsRadius);
  out_bAlwaysVisible = false;
  return EZ_SUCCESS;
}

void ezLodAnimatedMeshComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (m_Meshes.IsEmpty())
    return;

  if (msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::EditorView || msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::MainView)
  {
    UpdateSelectedLod(*msg.m_pView);
  }

  if (m_iCurLod >= (ezInt32)m_Meshes.GetCount())
    return;

  auto hMesh = m_Meshes[m_iCurLod].m_hMesh;

  if (!hMesh.IsValid())
    return;

  ezResourceLock<ezMeshResource> pMesh(hMesh, ezResourceAcquireMode::AllowLoadingFallback);
  ezArrayPtr<const ezMeshResourceDescriptor::SubMesh> parts = pMesh->GetSubMeshes();

  for (ezUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const ezUInt32 uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    ezMaterialResourceHandle hMaterial;

    hMaterial = pMesh->GetMaterials()[uiMaterialIndex];

    ezMeshRenderData* pRenderData = CreateRenderData();
    {
      pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform() * pRenderData->m_GlobalTransform;
      pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
      pRenderData->m_fSortingDepthOffset = m_fSortingDepthOffset;
      pRenderData->m_hMesh = hMesh;
      pRenderData->m_hMaterial = hMaterial;
      pRenderData->m_Color = m_Color;
      pRenderData->m_vCustomData = m_vCustomData;
      pRenderData->m_uiSubMeshIndex = uiPartIndex;
      pRenderData->m_uiUniqueID = GetUniqueIdForRendering(uiMaterialIndex);

      pRenderData->FillSortingKey();
    }

    // Determine render data category.
    ezRenderData::Category category = ezDefaultRenderDataCategories::LitOpaque;
    if (hMaterial.IsValid())
    {
      ezResourceLock<ezMaterialResource> pMaterial(hMaterial, ezResourceAcquireMode::AllowLoadingFallback);

      category = pMaterial->GetRenderDataCategory();
    }

    msg.AddRenderData(pRenderData, category, ezRenderData::Caching::Never);
  }
}

void ezLodAnimatedMeshComponent::MapModelSpacePoseToSkinningSpace(const ezHashTable<ezHashedString, ezMeshResourceDescriptor::BoneData>& bones, const ezSkeleton& skeleton, ezArrayPtr<const ezMat4> modelSpaceTransforms, ezBoundingBox* bounds)
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

void ezLodAnimatedMeshComponent::SetColor(const ezColor& color)
{
  m_Color = color;

  InvalidateCachedRenderData();
}

const ezColor& ezLodAnimatedMeshComponent::GetColor() const
{
  return m_Color;
}

void ezLodAnimatedMeshComponent::SetCustomData(const ezVec4& vData)
{
  m_vCustomData = vData;

  InvalidateCachedRenderData();
}

const ezVec4& ezLodAnimatedMeshComponent::GetCustomData() const
{
  return m_vCustomData;
}

void ezLodAnimatedMeshComponent::SetSortingDepthOffset(float fOffset)
{
  m_fSortingDepthOffset = fOffset;

  InvalidateCachedRenderData();
}

float ezLodAnimatedMeshComponent::GetSortingDepthOffset() const
{
  return m_fSortingDepthOffset;
}

void ezLodAnimatedMeshComponent::OnMsgSetColor(ezMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_Color);

  InvalidateCachedRenderData();
}

void ezLodAnimatedMeshComponent::OnMsgSetCustomData(ezMsgSetCustomData& ref_msg)
{
  m_vCustomData.Set(ref_msg.m_fData0, ref_msg.m_fData1, ref_msg.m_fData2, ref_msg.m_fData3);

  InvalidateCachedRenderData();
}

void ezLodAnimatedMeshComponent::RetrievePose(ezDynamicArray<ezMat4>& out_modelTransforms, ezTransform& out_rootTransform, const ezSkeleton& skeleton)
{
  out_modelTransforms.Clear();

  if (m_Meshes.IsEmpty())
    return;

  auto hMesh = m_Meshes[0].m_hMesh;

  if (!hMesh.IsValid())
    return;

  out_rootTransform = m_RootTransform;

  ezResourceLock<ezMeshResource> pMesh(hMesh, ezResourceAcquireMode::BlockTillLoaded);

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

ezMeshRenderData* ezLodAnimatedMeshComponent::CreateRenderData() const
{
  auto pRenderData = ezCreateRenderDataForThisFrame<ezSkinnedMeshRenderData>(GetOwner());
  pRenderData->m_GlobalTransform = m_RootTransform;

  pRenderData->m_hSkinningTransforms = m_SkinningState.m_hGpuBuffer;

  return pRenderData;
}

static float CalculateSphereScreenSpaceCoverage(const ezBoundingSphere& sphere, const ezCamera& camera)
{
  if (camera.IsPerspective())
  {
    return ezGraphicsUtils::CalculateSphereScreenCoverage(sphere, camera.GetCenterPosition(), camera.GetFovY(1.0f));
  }
  else
  {
    return ezGraphicsUtils::CalculateSphereScreenCoverage(sphere.m_fRadius, camera.GetDimensionY(1.0f));
  }
}

void ezLodAnimatedMeshComponent::UpdateSelectedLod(const ezView& view) const
{
  const ezInt32 iNumLods = (ezInt32)m_Meshes.GetCount();

  const ezVec3 vScale = GetOwner()->GetGlobalScaling();
  const float fScale = ezMath::Max(vScale.x, vScale.y, vScale.z);
  const ezVec3 vCenter = GetOwner()->GetGlobalTransform() * m_vBoundsOffset;

  const float fCoverage = CalculateSphereScreenSpaceCoverage(ezBoundingSphere::MakeFromCenterAndRadius(vCenter, fScale * m_fBoundsRadius), *view.GetLodCamera());

  // clamp the input value, this is to prevent issues while editing the threshold array
  ezInt32 iNewLod = ezMath::Clamp<ezInt32>(m_iCurLod, 0, iNumLods);

  float fCoverageP = 1;
  float fCoverageN = 0;

  if (iNewLod > 0)
  {
    fCoverageP = m_Meshes[iNewLod - 1].m_fThreshold;
  }

  if (iNewLod < iNumLods)
  {
    fCoverageN = m_Meshes[iNewLod].m_fThreshold;
  }

  if (GetOverlapRanges())
  {
    const float fLodRangeOverlap = 0.40f;

    if (iNewLod + 1 < iNumLods)
    {
      float range = (fCoverageN - m_Meshes[iNewLod + 1].m_fThreshold);
      fCoverageN -= range * fLodRangeOverlap; // overlap into the next range
    }
    else
    {
      float range = (fCoverageN - 0.0f);
      fCoverageN -= range * fLodRangeOverlap; // overlap into the next range
    }
  }

  if (fCoverage < fCoverageN)
  {
    ++iNewLod;
  }
  else if (fCoverage > fCoverageP)
  {
    --iNewLod;
  }

  iNewLod = ezMath::Clamp(iNewLod, 0, iNumLods);
  m_iCurLod = iNewLod;

  if (GetShowDebugInfo())
  {
    ezStringBuilder sb;
    sb.SetFormat("Coverage: {}\nLOD {}\nRange: {} - {}", ezArgF(fCoverage, 3), iNewLod, ezArgF(fCoverageP, 3), ezArgF(fCoverageN, 3));
    ezDebugRenderer::Draw3DText(view.GetHandle(), sb, GetOwner()->GetGlobalPosition(), ezColor::White);
  }
}

void ezLodAnimatedMeshComponent::OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& msg)
{
  if (m_Meshes.IsEmpty() || !m_Meshes[0].m_hMesh.IsValid())
    return;

  m_RootTransform = *msg.m_pRootTransform;

  ezResourceLock<ezMeshResource> pMesh(m_Meshes[0].m_hMesh, ezResourceAcquireMode::BlockTillLoaded);

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

void ezLodAnimatedMeshComponent::OnQueryAnimationSkeleton(ezMsgQueryAnimationSkeleton& msg)
{
  if (m_Meshes.IsEmpty() || !m_Meshes[0].m_hMesh.IsValid())
    return;

  if (!msg.m_hSkeleton.IsValid())
  {
    // only overwrite, if no one else had a better skeleton (e.g. the ezSkeletonComponent)

    ezResourceLock<ezMeshResource> pMesh(m_Meshes[0].m_hMesh, ezResourceAcquireMode::BlockTillLoaded);
    if (pMesh.GetAcquireResult() == ezResourceAcquireResult::Final)
    {
      msg.m_hSkeleton = pMesh->m_hDefaultSkeleton;
    }
  }
}

void ezLodAnimatedMeshComponent::OnActivated()
{
  SUPER::OnActivated();

  InitializeAnimationPose();
}

void ezLodAnimatedMeshComponent::OnDeactivated()
{
  m_SkinningState.Clear();

  SUPER::OnDeactivated();
}

void ezLodAnimatedMeshComponent::InitializeAnimationPose()
{
  m_MaxBounds = ezBoundingBox::MakeInvalid();

  if (m_Meshes.IsEmpty() || !m_Meshes[0].m_hMesh.IsValid())
    return;

  ezResourceLock<ezMeshResource> pMesh(m_Meshes[0].m_hMesh, ezResourceAcquireMode::BlockTillLoaded);
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

//////////////////////////////////////////////////////////////////////////


ezLodAnimatedMeshComponentManager::ezLodAnimatedMeshComponentManager(ezWorld* pWorld)
  : ezComponentManager<ComponentType, ezBlockStorageType::FreeList>(pWorld)
{
  ezResourceManager::GetResourceEvents().AddEventHandler(ezMakeDelegate(&ezLodAnimatedMeshComponentManager::ResourceEventHandler, this));
}

ezLodAnimatedMeshComponentManager::~ezLodAnimatedMeshComponentManager()
{
  ezResourceManager::GetResourceEvents().RemoveEventHandler(ezMakeDelegate(&ezLodAnimatedMeshComponentManager::ResourceEventHandler, this));
}

void ezLodAnimatedMeshComponentManager::Initialize()
{
  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezLodAnimatedMeshComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void ezLodAnimatedMeshComponentManager::ResourceEventHandler(const ezResourceEvent& e)
{
  if (e.m_Type == ezResourceEvent::Type::ResourceContentUnloading)
  {
    if (ezMeshResource* pResource = ezDynamicCast<ezMeshResource*>(e.m_pResource))
    {
      ezMeshResourceHandle hMesh(pResource);

      for (auto it = GetComponents(); it.IsValid(); it.Next())
      {
        for (auto& am : it->m_Meshes)
        {
          if (am.m_hMesh == hMesh)
          {
            AddToUpdateList(it);
          }
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

void ezLodAnimatedMeshComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  for (auto hComp : m_ComponentsToUpdate)
  {
    ezLodAnimatedMeshComponent* pComponent = nullptr;
    if (!TryGetComponent(hComp, pComponent))
      continue;

    if (!pComponent->IsActive())
      continue;

    pComponent->InitializeAnimationPose();
  }

  m_ComponentsToUpdate.Clear();
}

void ezLodAnimatedMeshComponentManager::AddToUpdateList(ezLodAnimatedMeshComponent* pComponent)
{
  ezComponentHandle hComponent = pComponent->GetHandle();

  if (m_ComponentsToUpdate.IndexOf(hComponent) == ezInvalidIndex)
  {
    m_ComponentsToUpdate.PushBack(hComponent);
  }
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_LodAnimatedMeshComponent);
