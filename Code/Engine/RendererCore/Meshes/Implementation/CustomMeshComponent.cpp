#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Meshes/CustomMeshComponent.h>
#include <RendererCore/Meshes/DynamicMeshBufferResource.h>
#include <RendererCore/Pipeline/RenderDataManager.h>

/////////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCustomMeshRenderData, 1, ezRTTIDefaultAllocator<ezCustomMeshRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezCustomMeshRenderData::FillSortingKey()
{
  const ezUInt32 uiMeshIDHash = ezHashingUtils::StringHashTo32(m_hDynamicMeshBuffer.GetResourceIDHash());
  const ezUInt32 uiMaterialIDHash = m_hMaterial.IsValid() ? ezHashingUtils::StringHashTo32(m_hMaterial.GetResourceIDHash()) : 0;
  const ezUInt32 uiFlipWinding = m_Flags.IsSet(Flags::FlipWinding) ? 1 : 0;

  // Sort by material and then by mesh
  m_uiSortingKey = (uiMaterialIDHash << 16) | ((uiMeshIDHash) & 0xFFFE) | uiFlipWinding;
}

bool ezCustomMeshRenderData::CanBatch(const ezRenderData& other0) const
{
  const auto& other = ezStaticCast<const ezCustomMeshRenderData&>(other0);

  return m_hDynamicMeshBuffer == other.m_hDynamicMeshBuffer &&
         m_hMaterial == other.m_hMaterial &&
         CanBatchByBaseValues(other);
}

/////////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezCustomMeshComponent, 3, ezComponentMode::Static)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new ezExposeColorAlphaAttribute()),
    EZ_ACCESSOR_PROPERTY("CustomData", GetCustomData, SetCustomData)->AddAttributes(new ezDefaultValueAttribute(ezVec4(0, 1, 0, 1))),
    EZ_RESOURCE_MEMBER_PROPERTY("Material", m_hMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
    EZ_MESSAGE_HANDLER(ezMsgSetMeshMaterial, OnMsgSetMeshMaterial),
    EZ_MESSAGE_HANDLER(ezMsgSetColor, OnMsgSetColor),
    EZ_MESSAGE_HANDLER(ezMsgSetCustomData, OnMsgSetCustomData),
  } EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezAtomicInteger32 s_iCustomMeshResources;

ezCustomMeshComponent::ezCustomMeshComponent() = default;
ezCustomMeshComponent::~ezCustomMeshComponent() = default;

void ezCustomMeshComponent::OnActivated()
{
  SUPER::OnActivated();

  if (false)
  {
    ezGeometry geo;
    geo.AddTorus(1.0f, 1.5f, 32, 16, false);
    geo.TriangulatePolygons();
    geo.ComputeTangents();

    auto hMesh = CreateMeshResource(ezGALPrimitiveTopology::Triangles, geo.GetVertices().GetCount(), geo.GetPolygons().GetCount(), ezGALIndexType::UInt);

    ezResourceLock<ezDynamicMeshBufferResource> pMesh(hMesh, ezResourceAcquireMode::BlockTillLoaded);

    auto positions = pMesh->AccessPositionData();
    auto ntts = pMesh->AccessNormalTangentTexCoord0Data();
    auto cols = pMesh->AccessColorData();

    for (ezUInt32 v = 0; v < positions.GetCount(); ++v)
    {
      positions[v] = geo.GetVertices()[v].m_vPosition;

      ntts[v].EncodeNormal(geo.GetVertices()[v].m_vNormal);
      ntts[v].EncodeTangent(geo.GetVertices()[v].m_vTangent, 1.0f);
      ntts[v].m_vTexCoord.SetZero();

      cols[v] = ezColor::CornflowerBlue;
    }

    auto ind = pMesh->AccessIndex32Data();

    for (ezUInt32 i = 0; i < geo.GetPolygons().GetCount(); ++i)
    {
      ind[i * 3 + 0] = geo.GetPolygons()[i].m_Vertices[0];
      ind[i * 3 + 1] = geo.GetPolygons()[i].m_Vertices[1];
      ind[i * 3 + 2] = geo.GetPolygons()[i].m_Vertices[2];
    }

    SetBounds(ezBoundingSphere::MakeFromCenterAndRadius(ezVec3::MakeZero(), 1.5f));
  }
}

void ezCustomMeshComponent::OnDeactivated()
{
  ezRenderDataManager* pRenderDataManager = GetWorld()->GetModule<ezRenderDataManager>();
  pRenderDataManager->DeleteInstanceData(m_InstanceDataOffset);

  SUPER::OnDeactivated();
}

void ezCustomMeshComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  ezStreamWriter& s = inout_stream.GetStream();

  s << m_Color;
  s << m_hMaterial;

  s << m_vCustomData;
}

void ezCustomMeshComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  ezStreamReader& s = inout_stream.GetStream();

  s >> m_Color;
  s >> m_hMaterial;

  if (uiVersion < 2)
  {
    ezUInt32 uiCategory = 0;
    s >> uiCategory;
  }

  if (uiVersion >= 3)
  {
    s >> m_vCustomData;
  }
}

ezResult ezCustomMeshComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  if (m_Bounds.IsValid())
  {
    ref_bounds = m_Bounds;
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezDynamicMeshBufferResourceHandle ezCustomMeshComponent::CreateMeshResource(ezGALPrimitiveTopology::Enum topology, ezUInt32 uiMaxVertices, ezUInt32 uiMaxPrimitives, ezGALIndexType::Enum indexType)
{
  ezDynamicMeshBufferResourceDescriptor desc;
  desc.m_Topology = topology;
  desc.m_uiMaxVertices = uiMaxVertices;
  desc.m_uiMaxPrimitives = uiMaxPrimitives;
  desc.m_IndexType = indexType;
  desc.m_bColorStream = true;

  ezStringBuilder sGuid;
  sGuid.SetFormat("CustomMesh_{}", s_iCustomMeshResources.Increment());

  m_hDynamicMesh = ezResourceManager::CreateResource<ezDynamicMeshBufferResource>(sGuid, std::move(desc));

  InvalidateCachedRenderData();

  return m_hDynamicMesh;
}

void ezCustomMeshComponent::SetMeshResource(const ezDynamicMeshBufferResourceHandle& hMesh)
{
  m_hDynamicMesh = hMesh;
  InvalidateCachedRenderData();
}

void ezCustomMeshComponent::SetBounds(const ezBoundingBoxSphere& bounds)
{
  m_Bounds = bounds;
  TriggerLocalBoundsUpdate();
}

void ezCustomMeshComponent::SetMaterial(const ezMaterialResourceHandle& hMaterial)
{
  m_hMaterial = hMaterial;
  InvalidateCachedRenderData();
}

ezMaterialResourceHandle ezCustomMeshComponent::GetMaterial() const
{
  return m_hMaterial;
}

void ezCustomMeshComponent::SetColor(const ezColor& color)
{
  m_Color = color;

  InvalidateCachedRenderData();
}

const ezColor& ezCustomMeshComponent::GetColor() const
{
  return m_Color;
}

void ezCustomMeshComponent::SetCustomData(const ezVec4& vData)
{
  m_vCustomData = vData;

  InvalidateCachedRenderData();
}

const ezVec4& ezCustomMeshComponent::GetCustomData() const
{
  return m_vCustomData;
}

void ezCustomMeshComponent::OnMsgSetMeshMaterial(ezMsgSetMeshMaterial& ref_msg)
{
  SetMaterial(ref_msg.m_hMaterial);
}

void ezCustomMeshComponent::OnMsgSetColor(ezMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_Color);

  InvalidateCachedRenderData();
}

void ezCustomMeshComponent::OnMsgSetCustomData(ezMsgSetCustomData& ref_msg)
{
  m_vCustomData.Set(ref_msg.m_fData0, ref_msg.m_fData1, ref_msg.m_fData2, ref_msg.m_fData3);

  InvalidateCachedRenderData();
}

void ezCustomMeshComponent::SetUsePrimitiveRange(ezUInt32 uiFirstPrimitive /*= 0*/, ezUInt32 uiNumPrimitives /*= ezMath::MaxValue<ezUInt32>()*/)
{
  m_uiFirstPrimitive = uiFirstPrimitive;
  m_uiNumPrimitives = uiNumPrimitives;

  InvalidateCachedRenderData();
}

void ezCustomMeshComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_hDynamicMesh.IsValid() || !m_hMaterial.IsValid())
    return;

  const bool bDynamic = GetOwner()->IsDynamic();
  auto hInstanceDataBuffer = msg.m_pRenderDataManager->GetOrCreateInstanceDataAndFill(*this, bDynamic, GetOwner()->GetGlobalTransform(), m_InstanceDataOffset, GetUniqueIdForRendering(), m_Color, m_vCustomData);

  ezResourceLock<ezDynamicMeshBufferResource> pMesh(m_hDynamicMesh, ezResourceAcquireMode::BlockTillLoaded);

  ezCustomMeshRenderData* pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<ezCustomMeshRenderData>(GetOwner());
  {
    pRenderData->m_uiNumInstances = 1;
    pRenderData->m_DataOffsets.m_uiInstance = m_InstanceDataOffset.m_uiOffset;
    pRenderData->m_hInstanceDataBuffer = hInstanceDataBuffer;
    pRenderData->m_fSortingDepthOffset = 0.0f;

    pRenderData->m_hMaterial = m_hMaterial;
    pRenderData->m_hDynamicMeshBuffer = m_hDynamicMesh;
    pRenderData->m_uiFirstPrimitive = ezMath::Min(m_uiFirstPrimitive, pMesh->GetDescriptor().m_uiMaxPrimitives);
    pRenderData->m_uiNumPrimitives = ezMath::Min(m_uiNumPrimitives, pMesh->GetDescriptor().m_uiMaxPrimitives - pRenderData->m_uiFirstPrimitive);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    pRenderData->m_FallbackGlobalBBox = GetOwner()->GetGlobalBounds().GetBox();
#endif

    pRenderData->FillSortingKey();
  }

  ezResourceLock<ezMaterialResource> pMaterial(m_hMaterial, ezResourceAcquireMode::AllowLoadingFallback);
  ezRenderData::Category category = pMaterial->GetRenderDataCategory();
  bool bDontCacheYet = pMaterial.GetAcquireResult() == ezResourceAcquireResult::LoadingFallback;

  msg.AddRenderData(pRenderData, category, bDontCacheYet ? ezRenderData::Caching::Never : ezRenderData::Caching::IfStatic);
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_CustomMeshComponent);
