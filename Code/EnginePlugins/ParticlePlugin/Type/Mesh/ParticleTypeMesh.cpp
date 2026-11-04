#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Type/Mesh/ParticleTypeMesh.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Textures/Texture2DResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeMesh, 1, ezRTTIDefaultAllocator<ezParticleTypeMesh>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Mesh", m_sMesh)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Mesh_Static"), new ezRequiredAttribute()),
    EZ_MEMBER_PROPERTY("Material", m_sMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material")),
    EZ_MEMBER_PROPERTY("Scale", m_fScale)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("TintColorParam", m_sTintColorParameter),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

enum class TypeMeshVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added material
  Version_3, // added scale

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleTypeMesh::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = (int)TypeMeshVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_sMesh;
  inout_stream << m_sTintColorParameter;

  // Version 2
  inout_stream << m_sMaterial;

  // Version 3
  inout_stream << m_fScale;
}

void ezParticleTypeMesh::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)TypeMeshVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_sMesh;
  inout_stream >> m_sTintColorParameter;

  if (uiVersion >= 2)
  {
    inout_stream >> m_sMaterial;
  }

  if (uiVersion >= 3)
  {
    inout_stream >> m_fScale;
  }

  if (!m_sMesh.IsEmpty())
  {
    m_hMesh = ezResourceManager::LoadResource<ezMeshResource>(m_sMesh);
  }

  if (!m_sMaterial.IsEmpty())
  {
    m_hMaterial = ezResourceManager::LoadResource<ezMaterialResource>(m_sMaterial);
  }
}

bool ezParticleTypeMesh::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void ezParticleTypeMesh::OnDeinitialize(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  if (data.m_pRenderDataManager != nullptr)
  {
    data.m_pRenderDataManager->DeleteInstanceData(data.m_InstanceDataOffset);
  }
  else
  {
    EZ_ASSERT_DEBUG(data.m_InstanceDataOffset.IsInvalidated(), "Implementation error");
  }
}

void ezParticleTypeMesh::CreateRequiredStreams(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  data.m_pRenderDataManager = (ezRenderDataManager*)ctx.m_pSystem->GetOwnerWorldModule()->GetCachedWorldModule(ezGetStaticRTTI<ezRenderDataManager>());
  data.m_hMaterial = m_hMaterial;
  data.m_pStreamVariation = nullptr;

  // during a reconfigure this may be a different mesh, which may not be loaded yet, so the extraction has to query it again
  data.m_bRenderDataCached = false;
  QueryMeshAndMaterialInfo(data);

  ctx.CreateStream(ezParticleStreamNames::s_sPosition, ezProcessingStream::DataType::Float4, &data.m_pStreamPosition, false);
  ctx.CreateStream(ezParticleStreamNames::s_sSize, ezProcessingStream::DataType::Half, &data.m_pStreamSize, false);
  ctx.CreateStream(ezParticleStreamNames::s_sColor, ezProcessingStream::DataType::Half4, &data.m_pStreamColor, false);
  ctx.CreateStream(ezParticleStreamNames::s_sRotationSpeed, ezProcessingStream::DataType::Half, &data.m_pStreamRotationSpeed, false);
  ctx.CreateStream(ezParticleStreamNames::s_sRotationOffset, ezProcessingStream::DataType::Half, &data.m_pStreamRotationOffset, false);
  ctx.CreateStream(ezParticleStreamNames::s_sAxis, ezProcessingStream::DataType::Float3, &data.m_pStreamAxis, true);

  if (data.m_uiNumSubMeshes > 1)
  {
    // only create this stream when necessary
    ctx.CreateStream(ezParticleStreamNames::s_sVariation, ezProcessingStream::DataType::Int, &data.m_pStreamVariation, false);
  }
}

void ezParticleTypeMesh::InitializeElements(const ezParticleModuleContext& ctx, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  ezVec3* pAxis = data.m_pStreamAxis->GetWritableData<ezVec3>();
  ezRandom& rng = ctx.GetRNG();

  for (ezUInt32 i = 0; i < uiNumElements; ++i)
  {
    const ezUInt64 uiElementIdx = uiStartIndex + i;

    pAxis[uiElementIdx] = ezVec3::MakeRandomDirection(rng);
  }
}

bool ezParticleTypeMesh::QueryMeshAndMaterialInfo(InstanceData& data) const
{
  if (!m_hMesh.IsValid())
  {
    data.m_bRenderDataCached = true;
    data.m_hMaterial.Invalidate();
    data.m_uiNumSubMeshes = 0;
    data.m_CachedSubMeshMaterials.Clear();
    return true;
  }

  ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowLoadingFallback);
  if (pMesh.GetAcquireResult() != ezResourceAcquireResult::Final)
    return false;

  data.m_uiNumSubMeshes = static_cast<ezUInt8>(pMesh->GetSubMeshes().GetCount());

  if (!data.m_hMaterial.IsValid() && !pMesh->GetMaterials().IsEmpty())
  {
    // When no material is specified, we'll use per-submesh materials during rendering
    // For now, use the first material to determine the render category
    data.m_hMaterial = pMesh->GetMaterials()[0];
  }

  // Cache materials for each submesh to avoid locking the mesh resource during rendering
  if (m_sMaterial.IsEmpty() && data.m_uiNumSubMeshes > 0)
  {
    const auto& subMeshes = pMesh->GetSubMeshes();
    const auto& materials = pMesh->GetMaterials();

    data.m_CachedSubMeshMaterials.SetCount(data.m_uiNumSubMeshes);
    for (ezUInt32 i = 0; i < data.m_uiNumSubMeshes; ++i)
    {
      const ezUInt32 uiMaterialIdx = subMeshes[i].m_uiMaterialIndex;
      if (uiMaterialIdx < materials.GetCount() && materials[uiMaterialIdx].IsValid())
      {
        data.m_CachedSubMeshMaterials[i] = materials[uiMaterialIdx];
      }
      else
      {
        data.m_CachedSubMeshMaterials[i].Invalidate();
      }
    }
  }
  else
  {
    data.m_CachedSubMeshMaterials.Clear();
  }

  if (!data.m_hMaterial.IsValid())
  {
    data.m_bRenderDataCached = true;
    return true;
  }

  ezResourceLock<ezMaterialResource> pMaterial(data.m_hMaterial, ezResourceAcquireMode::AllowLoadingFallback);
  if (pMaterial.GetAcquireResult() != ezResourceAcquireResult::Final)
    return false;

  data.m_RenderCategory = pMaterial->GetRenderDataCategory();

  data.m_bRenderDataCached = true;
  return true;
}

void ezParticleTypeMesh::RequestRequiredWorldModulesForCache(ezParticleWorldModule* pParticleModule) const
{
  pParticleModule->CacheWorldModule<ezRenderDataManager>();
}

void ezParticleTypeMesh::ExtractTypeRenderData(const ezParticleModuleContext& ctx, ezMsgExtractRenderData& ref_msg, const ezTransform& instanceTransform) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  if (!data.m_bRenderDataCached)
  {
    // check if we now know how to render this thing
    if (!QueryMeshAndMaterialInfo(data))
      return;
  }

  if (data.m_uiNumSubMeshes == 0 || !data.m_hMaterial.IsValid())
    return;

  if (data.m_RenderCategory.m_uiValue == 0xFFFF)
  {
    data.m_bRenderDataCached = false;
    return;
  }

  const ezUInt32 numParticles = (ezUInt32)ctx.m_pSystem->GetNumActiveParticles();

  if (numParticles == 0)
    return;

  EZ_PROFILE_SCOPE("PFX: Mesh");

  const ezTime tCur = ctx.GetEffect()->GetTotalEffectLifeTime();
  const ezColor tintColor = ctx.GetEffect()->GetColorParameter(m_sTintColorParameter, ezColor::White);

  const ezVec4* pPosition = data.m_pStreamPosition->GetData<ezVec4>();
  const ezFloat16* pSize = data.m_pStreamSize->GetData<ezFloat16>();
  const ezColorLinear16f* pColor = data.m_pStreamColor->GetData<ezColorLinear16f>();
  const ezFloat16* pRotationSpeed = data.m_pStreamRotationSpeed->GetData<ezFloat16>();
  const ezFloat16* pRotationOffset = data.m_pStreamRotationOffset->GetData<ezFloat16>();
  const ezVec3* pAxis = data.m_pStreamAxis->GetData<ezVec3>();
  const ezInt32* pVariation = (data.m_pStreamVariation != nullptr) ? data.m_pStreamVariation->GetData<ezInt32>() : nullptr;

  const bool bIsOpaque = data.m_RenderCategory == ezDefaultRenderDataCategories::LitOpaque ||
                         data.m_RenderCategory == ezDefaultRenderDataCategories::LitMasked ||
                         data.m_RenderCategory == ezDefaultRenderDataCategories::SimpleOpaque;

  const ezUInt8 uiNumSubMeshes = data.m_uiNumSubMeshes;

  {
    const bool bDynamic = true;
    ezGALDynamicBufferHandle hInstanceDataBuffer;
    const ezUInt32 uiMaxNumParticles = (ezUInt32)ctx.m_pSystem->GetMaxParticles();
    auto instanceData = ref_msg.m_pRenderDataManager->GetOrCreateInstanceData(nullptr, bDynamic, hInstanceDataBuffer, data.m_InstanceDataOffset, uiMaxNumParticles);

    // Opaque particles with a single submesh can be batched into one render data
    if (bIsOpaque && uiNumSubMeshes == 1)
    {
      for (ezUInt32 p = 0; p < numParticles; ++p)
      {
        const ezUInt32 idx = p;

        ezTransform trans;
        trans.m_qRotation = ezQuat::MakeFromAxisAndAngle(pAxis[p], ezAngle::MakeFromRadian((float)(tCur.GetSeconds() * pRotationSpeed[idx]) + pRotationOffset[idx]));
        trans.m_vPosition = pPosition[idx].GetAsVec3();
        trans.m_vScale.Set(pSize[idx] * m_fScale);

        ezRenderDataManager::FillPerInstanceData(instanceData[p], nullptr, trans, ezInvalidIndex, pColor[idx].ToLinearFloat() * tintColor);
      }

      ezMeshRenderData* pRenderData = ref_msg.m_pRenderDataManager->CreateRenderDataForThisFrame<ezMeshRenderData>(nullptr);
      pRenderData->m_vGlobalPosition = ctx.m_pSystem->GetTransform().m_vPosition;
      pRenderData->Fill(data.m_InstanceDataOffset, hInstanceDataBuffer, data.m_hMaterial, m_hMesh, 0, 0, numParticles);

      ref_msg.AddRenderData(pRenderData, data.m_RenderCategory, ezRenderData::Caching::Never);
    }
    else
    {
      // Non-opaque particles or multiple submeshes require per-particle render data
      for (ezUInt32 p = 0; p < numParticles; ++p)
      {
        const ezUInt32 idx = p;

        ezTransform trans;
        trans.m_qRotation = ezQuat::MakeFromAxisAndAngle(pAxis[p], ezAngle::MakeFromRadian((float)(tCur.GetSeconds() * pRotationSpeed[idx]) + pRotationOffset[idx]));
        trans.m_vPosition = pPosition[idx].GetAsVec3();
        trans.m_vScale.Set(pSize[idx] * m_fScale);

        ezRenderDataManager::FillPerInstanceData(instanceData[p], nullptr, trans, ezInvalidIndex, pColor[idx].ToLinearFloat() * tintColor);

        // Determine submesh index from variation
        // There is no variation stream for a single submesh, or if the mesh wasn't loaded yet when the streams were created.
        const ezUInt32 uiSubMeshIdx = (pVariation != nullptr) ? static_cast<ezUInt32>(ezMath::Abs(pVariation[idx])) % uiNumSubMeshes : 0;

        // Determine material for this submesh
        ezMaterialResourceHandle hMaterial = data.m_hMaterial;
        if (m_sMaterial.IsEmpty() && uiSubMeshIdx < data.m_CachedSubMeshMaterials.GetCount())
        {
          hMaterial = data.m_CachedSubMeshMaterials[uiSubMeshIdx];
        }

        ezInstanceDataOffset perParticleOffset = data.m_InstanceDataOffset;
        perParticleOffset.m_uiOffset += p;

        ezMeshRenderData* pRenderData = ref_msg.m_pRenderDataManager->CreateRenderDataForThisFrame<ezMeshRenderData>(nullptr);
        pRenderData->m_vGlobalPosition = trans.m_vPosition;
        pRenderData->Fill(perParticleOffset, hInstanceDataBuffer, hMaterial, m_hMesh, 0, uiSubMeshIdx);

        ref_msg.AddRenderData(pRenderData, data.m_RenderCategory, ezRenderData::Caching::Never);
      }
    }
  }
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Mesh_ParticleTypeMesh);
