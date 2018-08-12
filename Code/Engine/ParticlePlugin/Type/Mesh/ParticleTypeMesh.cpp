#include <PCH.h>

#include <Core/World/World.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Type/Mesh/ParticleTypeMesh.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Textures/Texture2DResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeMeshFactory, 1, ezRTTIDefaultAllocator<ezParticleTypeMeshFactory>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Mesh", m_sMesh)->AddAttributes(new ezAssetBrowserAttribute("Mesh;Animated Mesh")),
    EZ_MEMBER_PROPERTY("Material", m_sMaterial)->AddAttributes(new ezAssetBrowserAttribute("Material")),
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

void ezParticleTypeMeshFactory::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)TypeMeshVersion::Version_Current;
  stream << uiVersion;

  stream << m_sMesh;
  stream << m_sTintColorParameter;

  // Version 2
  stream << m_sMaterial;
}

void ezParticleTypeMeshFactory::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)TypeMeshVersion::Version_Current, "Invalid version {0}", uiVersion);

  stream >> m_sMesh;
  stream >> m_sTintColorParameter;

  if (uiVersion >= 2)
  {
    stream >> m_sMaterial;
  }
}

ezParticleTypeMesh::ezParticleTypeMesh() = default;
ezParticleTypeMesh::~ezParticleTypeMesh() = default;

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

    pAxis[uiElementIdx] = ezVec3::CreateRandomDirection(rng);
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

  ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowFallback);
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

  ezResourceLock<ezMaterialResource> pMaterial(m_hMaterial, ezResourceAcquireMode::AllowFallback);
  if (pMaterial.GetAcquireResult() != ezResourceAcquireResult::Final)
    return false;

  m_Bounds = pMesh->GetBounds();

  const ezUInt32 uiMeshIDHash = m_hMesh.GetResourceIDHash();
  const ezUInt32 uiMaterialIDHash = m_hMaterial.GetResourceIDHash();

  // Generate batch id from mesh, material and part index.
  const ezUInt32 data[] = {uiMeshIDHash, uiMaterialIDHash, 0, 0};
  m_uiBatchId = ezHashing::xxHash32(data, sizeof(data));

  // Sort by material and then by mesh
  const ezUInt32 uiFlipWinding = 0;
  m_uiSortingKey = (uiMaterialIDHash << 16) | (uiMeshIDHash & 0xFFFE) | uiFlipWinding;

  {
    m_RenderCategory = ezDefaultRenderDataCategories::LitOpaque;

    ezTempHashedString blendModeValue = pMaterial->GetPermutationValue("BLEND_MODE");
    if (blendModeValue == "BLEND_MODE_OPAQUE" || blendModeValue == "")
    {
      m_RenderCategory = ezDefaultRenderDataCategories::LitOpaque;
    }
    else if (blendModeValue == "BLEND_MODE_MASKED")
    {
      m_RenderCategory = ezDefaultRenderDataCategories::LitMasked;
    }
    else
    {
      m_RenderCategory = ezDefaultRenderDataCategories::LitTransparent;
    }
  }

  m_bRenderDataCached = true;
  return true;
}

void ezParticleTypeMesh::ExtractTypeRenderData(const ezView& view, ezExtractedRenderData& extractedRenderData,
                                               const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const
{
  if (!m_bRenderDataCached)
  {
    // check if we now know how to render this thing
    if (!QueryMeshAndMaterialInfo())
      return;
  }

  if (!m_hMaterial.IsValid())
    return;

  const ezUInt32 numParticles = (ezUInt32)GetOwnerSystem()->GetNumActiveParticles();

  if (numParticles == 0)
    return;

  EZ_PROFILE("PFX: Mesh");

  const ezTime tCur = GetOwnerSystem()->GetWorld()->GetClock().GetAccumulatedTime();
  const ezColor tintColor = GetOwnerEffect()->GetColorParameter(m_sTintColorParameter, ezColor::White);

  m_uiLastExtractedFrame = uiExtractedFrame;

  const ezVec4* pPosition = m_pStreamPosition->GetData<ezVec4>();
  const ezFloat16* pSize = m_pStreamSize->GetData<ezFloat16>();
  const ezColorLinear16f* pColor = m_pStreamColor->GetData<ezColorLinear16f>();
  const ezFloat16* pRotationSpeed = m_pStreamRotationSpeed->GetData<ezFloat16>();
  const ezFloat16* pRotationOffset = m_pStreamRotationOffset->GetData<ezFloat16>();
  const ezVec3* pAxis = m_pStreamAxis->GetData<ezVec3>();

  {
    const ezUInt32 uiFlipWinding = 0;

    for (ezUInt32 p = 0; p < numParticles; ++p)
    {
      const ezUInt32 idx = p;

      ezTransform trans;
      trans.m_qRotation.SetFromAxisAndAngle(pAxis[p],
                                            ezAngle::Radian((float)(tCur.GetSeconds() * pRotationSpeed[idx]) + pRotationOffset[idx]));
      trans.m_vPosition = pPosition[idx].GetAsVec3();
      trans.m_vScale.Set(pSize[idx]);

      ezMeshRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezMeshRenderData>(nullptr, m_uiBatchId);
      {
        pRenderData->m_GlobalTransform = trans;
        pRenderData->m_GlobalBounds = m_Bounds;
        pRenderData->m_hMesh = m_hMesh;
        pRenderData->m_hMaterial = m_hMaterial;
        pRenderData->m_Color = pColor[idx].ToLinearFloat() * tintColor;

        pRenderData->m_uiPartIndex = 0;
        pRenderData->m_uiFlipWinding = uiFlipWinding;
        pRenderData->m_uiUniformScale = 1;

        pRenderData->m_uiUniqueID = 0xFFFFFFFF;
      }

      extractedRenderData.AddRenderData(pRenderData, m_RenderCategory, m_uiSortingKey);
    }
  }
}
