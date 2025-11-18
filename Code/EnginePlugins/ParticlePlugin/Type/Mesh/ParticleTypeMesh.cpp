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
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeMeshFactory, 1, ezRTTIDefaultAllocator<ezParticleTypeMeshFactory>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Mesh", m_sMesh)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
    EZ_MEMBER_PROPERTY("Material", m_sMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material")),
    EZ_MEMBER_PROPERTY("TintColorParam", m_sTintColorParameter),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeMesh, 1, ezRTTIDefaultAllocator<ezParticleTypeMesh>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const ezRTTI* ezParticleTypeMeshFactory::GetTypeType() const
{
  return ezGetStaticRTTI<ezParticleTypeMesh>();
}


void ezParticleTypeMeshFactory::CopyTypeProperties(ezParticleType* pObject, bool bFirstTime) const
{
  ezParticleTypeMesh* pType = static_cast<ezParticleTypeMesh*>(pObject);

  pType->m_hMesh.Invalidate();
  pType->m_hMaterial.Invalidate();
  pType->m_sTintColorParameter = ezTempHashedString(m_sTintColorParameter.GetData());

  if (!m_sMesh.IsEmpty())
    pType->m_hMesh = ezResourceManager::LoadResource<ezMeshResource>(m_sMesh);

  if (!m_sMaterial.IsEmpty())
    pType->m_hMaterial = ezResourceManager::LoadResource<ezMaterialResource>(m_sMaterial);

  pType->m_pRenderDataManager = (ezRenderDataManager*)pType->GetOwnerSystem()->GetOwnerWorldModule()->GetCachedWorldModule(ezGetStaticRTTI<ezRenderDataManager>());
}

enum class TypeMeshVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added material

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleTypeMeshFactory::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = (int)TypeMeshVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_sMesh;
  inout_stream << m_sTintColorParameter;

  // Version 2
  inout_stream << m_sMaterial;
}

void ezParticleTypeMeshFactory::Load(ezStreamReader& inout_stream)
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
}

ezParticleTypeMesh::ezParticleTypeMesh() = default;

ezParticleTypeMesh::~ezParticleTypeMesh()
{
  if (m_pRenderDataManager != nullptr)
  {
    m_pRenderDataManager->DeleteInstanceData(m_InstanceDataOffset);
  }
  else
  {
    EZ_ASSERT_DEBUG(m_InstanceDataOffset.IsInvalidated(), "Implementation error");
  }
}

void ezParticleTypeMesh::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Size", ezProcessingStream::DataType::Half, &m_pStreamSize, false);
  CreateStream("Color", ezProcessingStream::DataType::Half4, &m_pStreamColor, false);
  CreateStream("RotationSpeed", ezProcessingStream::DataType::Half, &m_pStreamRotationSpeed, false);
  CreateStream("RotationOffset", ezProcessingStream::DataType::Half, &m_pStreamRotationOffset, false);
  CreateStream("Axis", ezProcessingStream::DataType::Float3, &m_pStreamAxis, true);
}

void ezParticleTypeMesh::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  ezVec3* pAxis = m_pStreamAxis->GetWritableData<ezVec3>();
  ezRandom& rng = GetRNG();

  for (ezUInt32 i = 0; i < uiNumElements; ++i)
  {
    const ezUInt64 uiElementIdx = uiStartIndex + i;

    pAxis[uiElementIdx] = ezVec3::MakeRandomDirection(rng);
  }
}

bool ezParticleTypeMesh::QueryMeshAndMaterialInfo() const
{
  if (!m_hMesh.IsValid())
  {
    m_bRenderDataCached = true;
    m_hMaterial.Invalidate();
    return true;
  }

  ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowLoadingFallback);
  if (pMesh.GetAcquireResult() != ezResourceAcquireResult::Final)
    return false;

  if (!m_hMaterial.IsValid())
  {
    m_hMaterial = pMesh->GetMaterials()[0];

    if (!m_hMaterial.IsValid())
    {
      m_bRenderDataCached = true;
      return true;
    }
  }

  ezResourceLock<ezMaterialResource> pMaterial(m_hMaterial, ezResourceAcquireMode::AllowLoadingFallback);
  if (pMaterial.GetAcquireResult() != ezResourceAcquireResult::Final)
    return false;

  m_RenderCategory = pMaterial->GetRenderDataCategory();

  m_bRenderDataCached = true;
  return true;
}

void ezParticleTypeMesh::RequestRequiredWorldModulesForCache(ezParticleWorldModule* pParticleModule)
{
  pParticleModule->CacheWorldModule<ezRenderDataManager>();
}

void ezParticleTypeMesh::ExtractTypeRenderData(ezMsgExtractRenderData& ref_msg, const ezTransform& instanceTransform) const
{
  if (!m_bRenderDataCached)
  {
    // check if we now know how to render this thing
    if (!QueryMeshAndMaterialInfo())
      return;
  }

  if (!m_hMaterial.IsValid())
    return;

  if (m_RenderCategory.m_uiValue == 0xFFFF)
  {
    m_bRenderDataCached = false;
    return;
  }

  const ezUInt32 numParticles = (ezUInt32)GetOwnerSystem()->GetNumActiveParticles();

  if (numParticles == 0)
    return;

  EZ_PROFILE_SCOPE("PFX: Mesh");

  const ezTime tCur = GetOwnerEffect()->GetTotalEffectLifeTime();
  const ezColor tintColor = GetOwnerEffect()->GetColorParameter(m_sTintColorParameter, ezColor::White);

  const ezVec4* pPosition = m_pStreamPosition->GetData<ezVec4>();
  const ezFloat16* pSize = m_pStreamSize->GetData<ezFloat16>();
  const ezColorLinear16f* pColor = m_pStreamColor->GetData<ezColorLinear16f>();
  const ezFloat16* pRotationSpeed = m_pStreamRotationSpeed->GetData<ezFloat16>();
  const ezFloat16* pRotationOffset = m_pStreamRotationOffset->GetData<ezFloat16>();
  const ezVec3* pAxis = m_pStreamAxis->GetData<ezVec3>();

  const bool bIsOpaque = m_RenderCategory == ezDefaultRenderDataCategories::LitOpaque ||
                         m_RenderCategory == ezDefaultRenderDataCategories::LitMasked ||
                         m_RenderCategory == ezDefaultRenderDataCategories::SimpleOpaque;

  {
    const bool bDynamic = true;
    ezGALDynamicBufferHandle hInstanceDataBuffer;
    const ezUInt32 uiMaxNumParticles = (ezUInt32)GetOwnerSystem()->GetMaxParticles();
    auto instanceData = ref_msg.m_pRenderDataManager->GetOrCreateInstanceData(nullptr, bDynamic, hInstanceDataBuffer, m_InstanceDataOffset, uiMaxNumParticles);

    for (ezUInt32 p = 0; p < numParticles; ++p)
    {
      const ezUInt32 idx = p;

      ezTransform trans;
      trans.m_qRotation = ezQuat::MakeFromAxisAndAngle(pAxis[p], ezAngle::MakeFromRadian((float)(tCur.GetSeconds() * pRotationSpeed[idx]) + pRotationOffset[idx]));
      trans.m_vPosition = pPosition[idx].GetAsVec3();
      trans.m_vScale.Set(pSize[idx]);

      ezRenderDataManager::FillPerInstanceData(instanceData[p], nullptr, trans, ezInvalidIndex, pColor[idx].ToLinearFloat() * tintColor);

      // If not rendered as opaque we need one render data per particle to allow for proper sorting
      if (!bIsOpaque)
      {
        ezInstanceDataOffset perParticleOffset = m_InstanceDataOffset;
        perParticleOffset.m_uiOffset += p;

        ezMeshRenderData* pRenderData = ref_msg.m_pRenderDataManager->CreateRenderDataForThisFrame<ezMeshRenderData>(nullptr);
        pRenderData->m_vGlobalPosition = trans.m_vPosition;
        pRenderData->Fill(perParticleOffset, hInstanceDataBuffer, m_hMaterial, m_hMesh);

        ref_msg.AddRenderData(pRenderData, m_RenderCategory, ezRenderData::Caching::Never);
      }
    }

    // If rendered as opaque we can pack everything into one render data
    if (bIsOpaque)
    {
      ezMeshRenderData* pRenderData = ref_msg.m_pRenderDataManager->CreateRenderDataForThisFrame<ezMeshRenderData>(nullptr);
      pRenderData->m_vGlobalPosition = GetOwnerSystem()->GetTransform().m_vPosition;
      pRenderData->Fill(m_InstanceDataOffset, hInstanceDataBuffer, m_hMaterial, m_hMesh, 0, 0, numParticles);

      ref_msg.AddRenderData(pRenderData, m_RenderCategory, ezRenderData::Caching::Never);
    }
  }
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Mesh_ParticleTypeMesh);
