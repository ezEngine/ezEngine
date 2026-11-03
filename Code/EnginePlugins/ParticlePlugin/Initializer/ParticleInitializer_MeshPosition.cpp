#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <GameEngine/Animation/Skeletal/LodAnimatedMeshComponent.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_MeshPosition.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Meshes/MeshResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializerFactory_MeshPosition, 1, ezRTTIDefaultAllocator<ezParticleInitializerFactory_MeshPosition>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("UseOwnerMesh", m_bUseOwnerMesh)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Mesh", m_sMesh)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Mesh_Static;CompatibleAsset_Mesh_Skinned")),
    EZ_MEMBER_PROPERTY("SetVelocity", m_bSetVelocity),
    EZ_MEMBER_PROPERTY("Speed", m_Speed),
    EZ_MEMBER_PROPERTY("ReferenceArea", m_fReferenceArea)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezSuffixAttribute(" m^2")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializer_MeshPosition, 1, ezRTTIDefaultAllocator<ezParticleInitializer_MeshPosition>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleInitializerFactory_MeshPosition::ezParticleInitializerFactory_MeshPosition() = default;

const ezRTTI* ezParticleInitializerFactory_MeshPosition::GetInitializerType() const
{
  return ezGetStaticRTTI<ezParticleInitializer_MeshPosition>();
}

void ezParticleInitializerFactory_MeshPosition::CopyInitializerProperties(ezParticleInitializer* pInitializer0, bool bFirstTime) const
{
  ezParticleInitializer_MeshPosition* pInitializer = static_cast<ezParticleInitializer_MeshPosition*>(pInitializer0);

  pInitializer->m_hMesh = m_hMesh;
  pInitializer->m_bUseOwnerMesh = m_bUseOwnerMesh;
  pInitializer->m_bSetVelocity = m_bSetVelocity;
  pInitializer->m_Speed = m_Speed;
  pInitializer->m_fReferenceArea = m_fReferenceArea;
}

void ezParticleInitializerFactory_MeshPosition::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 1;
  inout_stream << uiVersion;

  inout_stream << m_sMesh;
  inout_stream << m_bSetVelocity;
  inout_stream << m_Speed.m_Value;
  inout_stream << m_Speed.m_fVariance;
  inout_stream << m_bUseOwnerMesh;
  inout_stream << m_fReferenceArea;
}

void ezParticleInitializerFactory_MeshPosition::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion == 1, "Invalid version {0}", uiVersion);

  inout_stream >> m_sMesh;
  inout_stream >> m_bSetVelocity;
  inout_stream >> m_Speed.m_Value;
  inout_stream >> m_Speed.m_fVariance;
  inout_stream >> m_bUseOwnerMesh;
  inout_stream >> m_fReferenceArea;

  m_hMesh.Invalidate();

  if (!m_sMesh.IsEmpty())
  {
    m_hMesh = ezResourceManager::LoadResource<ezCpuMeshResource>(m_sMesh);
  }
}

void ezParticleInitializerFactory_MeshPosition::QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const
{
  if (m_bSetVelocity)
  {
    inout_finalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizerFactory_ApplyVelocity>());
  }
}

//////////////////////////////////////////////////////////////////////////

void ezParticleInitializer_MeshPosition::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, true);

  m_pStreamVelocity = nullptr;

  if (m_bSetVelocity)
  {
    CreateStream("Velocity", ezProcessingStream::DataType::Half4, &m_pStreamVelocity, true);
  }
}

void ezParticleInitializer_MeshPosition::OnFinalize()
{
  if (m_bUseOwnerMesh)
  {
    RequestUpdateFromWorld();
    RequestOwnerMessages();
  }
}

void ezParticleInitializer_MeshPosition::OnReset()
{
  m_bOwnerMeshSearched = false;
  m_hOwnerMeshObject.Invalidate();
  m_hOwnerMesh.Invalidate();
  m_OwnerMeshTransform.SetIdentity();

  m_pPoseSkeleton = nullptr;
  m_uiPoseMeshChangeCounter = 0;
  m_uiNumPoseBones = 0;
  m_PoseBones.Clear();
  m_bPosePending = false;
  m_bCurrentPoseChecked = false;
  m_PendingSkinningMatrices.Clear();
  m_SkinningMatrices.Clear();
}

void ezParticleInitializer_MeshPosition::UpdateFromWorld(const ezWorld& world, ezGameObjectHandle hOwnerObject)
{
  if (!m_bOwnerMeshSearched)
  {
    m_bOwnerMeshSearched = true;
    FindOwnerMesh(world, hOwnerObject);
  }

  // the mesh object may move relative to the effect, and with 'IgnoreOwnerRotation' the effect doesn't follow the owner rotation
  const ezGameObject* pMeshObject = nullptr;
  if (!m_hOwnerMeshObject.IsInvalidated() && world.TryGetObject(m_hOwnerMeshObject, pMeshObject))
  {
    m_OwnerMeshTransform = ezTransform::MakeLocalTransform(GetOwnerEffect()->GetTransformForNextFrame(), pMeshObject->GetGlobalTransform());
  }

  // the surface data is set during the particle update, which is finished at this point
  // the rest pose skeleton is loaded by then as well, retrieving the pose would otherwise block on it
  if (m_SkinningMatrices.IsEmpty() && !m_bPosePending && UsesSkinnedOwnerMesh() && !m_hRestPoseSkeleton.IsValid())
  {
    if (pMeshObject != nullptr)
    {
      RetrieveCurrentPose(*pMeshObject);
    }

    m_bCurrentPoseChecked = true;
  }

  if (m_bPosePending)
  {
    m_bPosePending = false;
    m_SkinningMatrices.Swap(m_PendingSkinningMatrices);
  }
}

void ezParticleInitializer_MeshPosition::RetrieveCurrentPose(const ezGameObject& meshObject)
{
  ezSkeletonResourceHandle hSkeleton;
  {
    ezResourceLock<ezCpuMeshResource> pMesh(m_hOwnerMesh, ezResourceAcquireMode::AllowLoadingFallback);
    if (pMesh.GetAcquireResult() != ezResourceAcquireResult::Final)
      return;

    hSkeleton = pMesh->GetDescriptor().m_hDefaultSkeleton;
  }

  if (!hSkeleton.IsValid())
    return;

  // tried again next frame, skeletons have no missing fallback, so this must not fail on a missing skeleton
  ezResourceLock<ezSkeletonResource> pSkeleton(hSkeleton, ezResourceAcquireMode::AllowLoadingFallback_NeverFail);
  if (pSkeleton.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  const ezSkeleton& skeleton = pSkeleton->GetDescriptor().m_Skeleton;

  ezDynamicArray<ezMat4> modelTransforms;
  ezTransform rootTransform = ezTransform::MakeIdentity();

  const ezAnimatedMeshComponent* pAnimMesh = nullptr;
  const ezLodAnimatedMeshComponent* pLodAnimMesh = nullptr;

  if (meshObject.TryGetComponentOfBaseType(pAnimMesh))
  {
    pAnimMesh->RetrievePose(modelTransforms, rootTransform, skeleton);
  }
  else if (meshObject.TryGetComponentOfBaseType(pLodAnimMesh))
  {
    pLodAnimMesh->RetrievePose(modelTransforms, rootTransform, skeleton);
  }

  if (modelTransforms.IsEmpty())
    return;

  ezMsgAnimationPoseUpdated msg;
  msg.m_pSkeleton = &skeleton;
  msg.m_pRootTransform = &rootTransform;
  msg.m_ModelTransforms = modelTransforms;

  UpdatePendingPose(msg);
}

void ezParticleInitializer_MeshPosition::FindOwnerMesh(const ezWorld& world, ezGameObjectHandle hOwnerObject)
{
  const ezGameObject* pOwner = nullptr;
  if (!world.TryGetObject(hOwnerObject, pOwner))
    return;

  // the effect may be attached somewhere below the mesh object, e.g. on a collider deep in the hierarchy
  for (const ezGameObject* pObject = pOwner; pObject != nullptr; pObject = pObject->GetParent())
  {
    ezMsgQueryMesh msg;
    pObject->SendMessage(msg);

    if (!msg.m_hMesh.IsValid())
      continue;

    // a procedurally created mesh only exists on the GPU side, there is no file to load a CPU mesh from
    {
      ezResourceLock<ezMeshResource> pMesh(msg.m_hMesh, ezResourceAcquireMode::PointerOnly);
      if (pMesh->GetBaseResourceFlags().IsAnySet(ezResourceFlags::IsCreatedResource))
      {
        if (!m_hMesh.IsValid())
        {
          ezLog::Warning("Particle effect '{}': the owner mesh '{}' is procedurally created and can't be used as a spawn surface.", GetOwnerEffect()->GetResource().GetResourceID(), msg.m_hMesh.GetResourceID());
        }
        return;
      }
    }

    m_hOwnerMesh = ezResourceManager::LoadResource<ezCpuMeshResource>(msg.m_hMesh.GetResourceID());
    m_hOwnerMeshObject = pObject->GetHandle();
    return;
  }
}

void ezParticleInitializer_MeshPosition::OnOwnerMessage(const ezMessage& msg)
{
  if (msg.IsInstanceOf<ezMsgAnimationPoseUpdated>())
  {
    // skinning is the expensive part, while the effect isn't visible, particles keep spawning on the last pose
    // the first pose is always used, otherwise particles would spawn on the rest pose until the effect gets rendered
    if (!GetOwnerEffect()->IsVisible() && (m_bPosePending || !m_SkinningMatrices.IsEmpty()))
      return;

    UpdatePendingPose(static_cast<const ezMsgAnimationPoseUpdated&>(msg));
  }
}

void ezParticleInitializer_MeshPosition::UpdatePendingPose(const ezMsgAnimationPoseUpdated& msg)
{
  if (!m_hOwnerMesh.IsValid() || msg.m_pSkeleton == nullptr)
    return;

  ezResourceLock<ezCpuMeshResource> pMesh(m_hOwnerMesh, ezResourceAcquireMode::AllowLoadingFallback);
  if (pMesh.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  const ezSkeleton& skeleton = *msg.m_pSkeleton;

  if (m_pPoseSkeleton != msg.m_pSkeleton || m_uiPoseMeshChangeCounter != pMesh->GetCurrentResourceChangeCounter())
  {
    m_pPoseSkeleton = msg.m_pSkeleton;
    m_uiPoseMeshChangeCounter = pMesh->GetCurrentResourceChangeCounter();
    m_uiNumPoseBones = 0;
    m_PoseBones.Clear();

    for (auto itBone : pMesh->GetDescriptor().m_Bones)
    {
      const ezUInt16 uiJointIdx = skeleton.FindJointByName(itBone.Key());

      if (uiJointIdx == ezInvalidJointIndex)
        continue;

      PoseBone& bone = m_PoseBones.ExpandAndGetRef();
      bone.m_mInverseRestPose = itBone.Value().m_GlobalInverseRestPoseMatrix;
      bone.m_uiBoneIndex = itBone.Value().m_uiBoneIndex;
      bone.m_uiJointIndex = uiJointIdx;

      m_uiNumPoseBones = ezMath::Max<ezUInt16>(m_uiNumPoseBones, bone.m_uiBoneIndex + 1);
    }
  }

  // same as the rendering of animated meshes, which applies the root transform on top of the skinned vertices
  const ezMat4 mRoot = msg.m_pRootTransform ? msg.m_pRootTransform->GetAsMat4() : ezMat4::MakeIdentity();
  const ezUInt32 uiNumJoints = ezMath::Min<ezUInt32>(skeleton.GetJointCount(), msg.m_ModelTransforms.GetCount());

  m_PendingSkinningMatrices.SetCount(m_uiNumPoseBones, ezMat4::MakeIdentity());

  for (const PoseBone& bone : m_PoseBones)
  {
    if (bone.m_uiJointIndex < uiNumJoints)
    {
      m_PendingSkinningMatrices[bone.m_uiBoneIndex] = mRoot * msg.m_ModelTransforms[bone.m_uiJointIndex] * bone.m_mInverseRestPose;
    }
  }

  m_bPosePending = true;
}

bool ezParticleInitializer_MeshPosition::PrepareToSpawn()
{
  // the owner mesh is only known after the first UpdateFromWorld()
  if (m_bUseOwnerMesh && !m_bOwnerMeshSearched)
    return false;

  if (m_hOwnerMesh.IsValid())
  {
    m_hActiveMesh = m_hOwnerMesh;
    m_ActiveMeshTransform = m_OwnerMeshTransform;
    m_bActiveMeshIsOwnerMesh = true;
  }
  else
  {
    m_hActiveMesh = m_hMesh;
    m_ActiveMeshTransform.SetIdentity();
    m_bActiveMeshIsOwnerMesh = false;
  }

  // without a mesh, particles spawn at the system origin
  if (!m_hActiveMesh.IsValid())
  {
    m_pSurfaceDataRef = nullptr;
    m_pSurfaceData = nullptr;
    m_bSurfaceDataValid = false;
    return true;
  }

  if (!UpdateSurfaceData())
    return false;

  if (!UpdateRestPoseTransform())
    return false;

  // A new initializer has no pose yet, and a burst would spawn all its particles right away on the rest pose.
  // This happens every time an effect restarts, so wait for UpdateFromWorld() to get the current pose. A mesh without a pose then uses the rest pose.
  if (UsesSkinnedOwnerMesh() && m_SkinningMatrices.IsEmpty() && !m_bCurrentPoseChecked)
    return false;

  return true;
}

float ezParticleInitializer_MeshPosition::GetSpawnCountMultiplier() const
{
  // without a usable surface, particles spawn at the origin, scaling the count by zero would be surprising
  if (m_fReferenceArea <= 0.0f || !HasSurface())
    return 1.0f;

  // the triangles are in the space of the mesh, positions get transformed by all of these, see InitializeElements() and SampleSurface()
  // not the transform of the system, that is the identity when simulating in local space, the effect transform is then applied during rendering
  const ezTransform meshToWorld = GetOwnerEffect()->GetTransform() * m_ActiveMeshTransform * m_RestPoseTransform;
  const ezVec3 vScale = meshToWorld.m_vScale;
  const float fAreaScale = ezMath::Pow(ezMath::Abs(vScale.x * vScale.y * vScale.z), 2.0f / 3.0f);

  return m_pSurfaceData->m_AreaPrefixSum.PeekBack() * fAreaScale / m_fReferenceArea;
}

bool ezParticleInitializer_MeshPosition::UpdateSurfaceData()
{
  ezResourceLock<ezCpuMeshResource> pMesh(m_hActiveMesh, ezResourceAcquireMode::AllowLoadingFallback);

  if (pMesh.GetAcquireResult() == ezResourceAcquireResult::LoadingFallback)
    return false;

  if (pMesh.GetAcquireResult() != ezResourceAcquireResult::Final)
  {
    // don't block the effect forever, spawn at the origin instead
    if (!m_bSurfaceDataValid || m_hSurfaceDataMesh != m_hActiveMesh)
    {
      ezLog::Warning("Mesh '{}' could not be loaded as a particle spawn area.", m_hActiveMesh.GetResourceID());
      m_bSurfaceDataValid = true;
      m_hSurfaceDataMesh = m_hActiveMesh;
      m_uiSurfaceDataChangeCounter = 0xFFFFFFFF;
      m_pSurfaceDataRef = nullptr;
      m_pSurfaceData = nullptr;
    }

    return true;
  }

  if (m_bSurfaceDataValid && m_hSurfaceDataMesh == m_hActiveMesh && m_uiSurfaceDataChangeCounter == pMesh->GetCurrentResourceChangeCounter())
    return true;

  m_bSurfaceDataValid = true;
  m_hSurfaceDataMesh = m_hActiveMesh;
  m_uiSurfaceDataChangeCounter = pMesh->GetCurrentResourceChangeCounter();
  m_RestPoseTransform.SetIdentity();
  m_hRestPoseSkeleton.Invalidate();

  ezStringBuilder sKey;
  sKey.SetFormat("MeshPosition|{}|{}", pMesh->GetResourceID(), m_uiSurfaceDataChangeCounter);

  const ezCpuMeshResource& mesh = *pMesh.GetPointer();
  m_pSurfaceDataRef = GetOwnerSystem()->GetOwnerWorldModule()->GetOrCreateSharedData(sKey, [&mesh]()
    { return CreateSurfaceData(mesh); });
  m_pSurfaceData = static_cast<const SurfaceData*>(m_pSurfaceDataRef.Borrow());

  if (m_pSurfaceData != nullptr && !m_pSurfaceData->m_TriangleSkinning.IsEmpty())
  {
    m_hRestPoseSkeleton = pMesh->GetDescriptor().m_hDefaultSkeleton;
  }

  return true;
}

ezSharedPtr<ezRefCounted> ezParticleInitializer_MeshPosition::CreateSurfaceData(const ezCpuMeshResource& mesh)
{
  EZ_PROFILE_SCOPE("PFX: Mesh Position Setup");

  const ezMeshBufferResourceDescriptor& mb = mesh.GetDescriptor().MeshBufferDesc();

  if (mb.GetTopology() != ezGALPrimitiveTopology::Triangles)
  {
    ezLog::Warning("Mesh '{}' can't be used as a particle spawn area, it doesn't consist of triangles.", mesh.GetResourceIdOrDescription());
    return nullptr;
  }

  const ezVec3* pPositions = mb.GetPositionData().GetPtr();
  if (pPositions == nullptr)
    return nullptr;

  ezSharedPtr<SurfaceData> pData = EZ_DEFAULT_NEW(SurfaceData);

  const bool bHasNormals = mb.GetVertexStreamConfig().HasNormal();
  const bool bHasSkinning = mb.GetVertexStreamConfig().HasSkinningData();

  const ezUInt32 uiNumTriangles = mb.GetPrimitiveCount();
  const ezUInt8* pIndices = mb.HasIndexBuffer() ? mb.GetIndexBufferData().GetPtr() : nullptr;
  const bool b32BitIndices = mb.Uses32BitIndices();

  pData->m_Triangles.Reserve(uiNumTriangles);
  pData->m_AreaPrefixSum.Reserve(uiNumTriangles);

  if (bHasSkinning)
  {
    pData->m_TriangleSkinning.Reserve(uiNumTriangles);
  }

  float fAreaSum = 0.0f;

  for (ezUInt32 t = 0; t < uiNumTriangles; ++t)
  {
    ezUInt32 idx[3];

    for (ezUInt32 v = 0; v < 3; ++v)
    {
      if (pIndices == nullptr)
        idx[v] = t * 3 + v;
      else if (b32BitIndices)
        idx[v] = reinterpret_cast<const ezUInt32*>(pIndices)[t * 3 + v];
      else
        idx[v] = reinterpret_cast<const ezUInt16*>(pIndices)[t * 3 + v];
    }

    Triangle tri;

    for (ezUInt32 v = 0; v < 3; ++v)
    {
      tri.m_vPosition[v] = pPositions[idx[v]];
    }

    const ezVec3 vCross = (tri.m_vPosition[1] - tri.m_vPosition[0]).CrossRH(tri.m_vPosition[2] - tri.m_vPosition[0]);
    const float fArea = vCross.GetLength() * 0.5f;

    // degenerate triangles would never be picked anyway
    if (fArea <= 0.0f)
      continue;

    const ezVec3 vFaceNormal = vCross.GetNormalized();

    for (ezUInt32 v = 0; v < 3; ++v)
    {
      tri.m_vNormal[v] = bHasNormals ? mb.GetNormal(idx[v]) : vFaceNormal;
    }

    fAreaSum += fArea;

    pData->m_Triangles.PushBack(tri);
    pData->m_AreaPrefixSum.PushBack(fAreaSum);

    if (bHasSkinning)
    {
      TriangleSkinning& skin = pData->m_TriangleSkinning.ExpandAndGetRef();

      for (ezUInt32 v = 0; v < 3; ++v)
      {
        skin.m_vBoneIndices[v] = mb.GetBoneIndices(idx[v]);
        skin.m_vBoneWeights[v] = mb.GetBoneWeights(idx[v]);
      }
    }
  }

  return pData;
}

bool ezParticleInitializer_MeshPosition::UpdateRestPoseTransform()
{
  if (!m_hRestPoseSkeleton.IsValid())
    return true;

  ezResourceState state;
  {
    ezResourceLock<ezSkeletonResource> pSkeleton(m_hRestPoseSkeleton, ezResourceAcquireMode::PointerOnly);
    state = pSkeleton->GetLoadingState();
  }

  // skeletons have no loading fallback, acquiring the data before it is loaded would block
  if (state != ezResourceState::Loaded && state != ezResourceState::LoadedResourceMissing)
  {
    ezResourceManager::PreloadResource(m_hRestPoseSkeleton);
    return false;
  }

  if (state == ezResourceState::Loaded)
  {
    ezResourceLock<ezSkeletonResource> pSkeleton(m_hRestPoseSkeleton, ezResourceAcquireMode::AllowLoadingFallback);
    m_RestPoseTransform = pSkeleton->GetDescriptor().m_RootTransform;
  }

  m_hRestPoseSkeleton.Invalidate();
  return true;
}

namespace
{
  /// Returns the index of the first entry in the prefix sums that is larger than fValue.
  ezUInt32 FindInPrefixSum(const ezDynamicArray<float>& prefixSum, float fValue)
  {
    ezUInt32 uiLow = 0;
    ezUInt32 uiHigh = prefixSum.GetCount() - 1;

    while (uiLow < uiHigh)
    {
      const ezUInt32 uiMid = (uiLow + uiHigh) / 2;

      if (prefixSum[uiMid] <= fValue)
        uiLow = uiMid + 1;
      else
        uiHigh = uiMid;
    }

    return uiLow;
  }

  void SkinVertex(ezArrayPtr<const ezMat4> skinningMatrices, const ezVec4U16& vBoneIndices, const ezVec4& vBoneWeights, ezVec3& inout_vPosition, ezVec3& inout_vNormal)
  {
    ezVec3 vPos = ezVec3::MakeZero();
    ezVec3 vNormal = ezVec3::MakeZero();
    float fWeightSum = 0.0f;

    for (ezUInt32 i = 0; i < 4; ++i)
    {
      const float fWeight = vBoneWeights.GetData()[i];
      const ezUInt16 uiBone = vBoneIndices.GetData()[i];

      if (fWeight <= 0.0f || uiBone >= skinningMatrices.GetCount())
        continue;

      const ezMat4& mSkin = skinningMatrices[uiBone];
      vPos += mSkin.TransformPosition(inout_vPosition) * fWeight;
      vNormal += mSkin.TransformDirection(inout_vNormal) * fWeight;
      fWeightSum += fWeight;
    }

    if (fWeightSum > 0.0f)
    {
      inout_vPosition = vPos / fWeightSum;
      inout_vNormal = vNormal;
    }
  }
} // namespace

void ezParticleInitializer_MeshPosition::SampleSurface(ezRandom& rng, ezArrayPtr<const ezMat4> skinningMatrices, ezVec3& out_vPosition, ezVec3& out_vNormal) const
{
  // picks each triangle with a probability proportional to its area
  const SurfaceData& surface = *m_pSurfaceData;
  const ezUInt32 uiTriangle = FindInPrefixSum(surface.m_AreaPrefixSum, rng.FloatZeroToOneExclusive() * surface.m_AreaPrefixSum.PeekBack());
  const Triangle& tri = surface.m_Triangles[uiTriangle];

  // uniformly distributed barycentric coordinates
  float u = rng.FloatZeroToOneExclusive();
  float v = rng.FloatZeroToOneExclusive();

  if (u + v > 1.0f)
  {
    u = 1.0f - u;
    v = 1.0f - v;
  }

  const float w = 1.0f - u - v;

  ezVec3 vPositions[3] = {tri.m_vPosition[0], tri.m_vPosition[1], tri.m_vPosition[2]};
  ezVec3 vNormals[3] = {tri.m_vNormal[0], tri.m_vNormal[1], tri.m_vNormal[2]};

  if (!skinningMatrices.IsEmpty())
  {
    // the triangle was picked by its rest pose area, which is close enough for typical deformations
    const TriangleSkinning& skin = surface.m_TriangleSkinning[uiTriangle];

    for (ezUInt32 i = 0; i < 3; ++i)
    {
      SkinVertex(skinningMatrices, skin.m_vBoneIndices[i], skin.m_vBoneWeights[i], vPositions[i], vNormals[i]);
    }
  }
  else
  {
    for (ezUInt32 i = 0; i < 3; ++i)
    {
      vPositions[i] = m_RestPoseTransform * vPositions[i];
      vNormals[i] = m_RestPoseTransform.m_qRotation * vNormals[i];
    }
  }

  out_vPosition = vPositions[0] * w + vPositions[1] * u + vPositions[2] * v;
  out_vNormal = vNormals[0] * w + vNormals[1] * u + vNormals[2] * v;
}

void ezParticleInitializer_MeshPosition::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  EZ_PROFILE_SCOPE("PFX: Mesh Position");

  const ezVec3 startVel = GetOwnerSystem()->GetParticleStartVelocity();

  ezVec4* pPosition = m_pStreamPosition->GetWritableData<ezVec4>();
  ezFloat16Vec4* pVelocity = m_bSetVelocity ? m_pStreamVelocity->GetWritableData<ezFloat16Vec4>() : nullptr;

  ezRandom& rng = GetRNG();

  const ezTransform trans = GetOwnerSystem()->GetTransform();

  const bool bSurface = HasSurface();

  // only the owner mesh can be animated, the pose is empty until the first one arrives
  ezArrayPtr<const ezMat4> skinningMatrices;
  if (bSurface && UsesSkinnedOwnerMesh())
  {
    skinningMatrices = m_SkinningMatrices;
  }

  for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    ezVec3 pos = ezVec3::MakeZero();
    ezVec3 normal = ezVec3(0, 0, 1);

    if (bSurface)
    {
      SampleSurface(rng, skinningMatrices, pos, normal);
    }

    pos = m_ActiveMeshTransform * pos;
    normal = m_ActiveMeshTransform.m_qRotation * normal;
    normal.NormalizeIfNotZero(ezVec3(0, 0, 1)).IgnoreResult();

    if (m_bSetVelocity)
    {
      const float fSpeed = (float)rng.DoubleVariance(m_Speed.m_Value, m_Speed.m_fVariance);

      const ezVec3 vel = startVel + trans.m_qRotation * normal * fSpeed;
      const float fVelLength = vel.GetLength();
      const ezVec3 velDir = fVelLength > 0.0f ? vel / fVelLength : ezVec3(0, 0, 1);

      pVelocity[i] = ezVec4(velDir.x, velDir.y, velDir.z, fVelLength);
    }

    pPosition[i] = (trans * pos).GetAsVec4(0);
  }
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_MeshPosition);
