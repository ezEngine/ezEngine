#include <RendererCore/RendererCorePCH.h>

#include <../../Data/Base/Shaders/Common/ObjectConstants.h>
#include <Core/Graphics/Geometry.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Meshes/CustomMeshComponent.h>
#include <RendererCore/Meshes/DynamicMeshBufferResource.h>
#include <RendererCore/Pipeline/InstanceDataProvider.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Device/Device.h>

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

ezCustomMeshComponent::ezCustomMeshComponent()
{
  m_Bounds = ezBoundingBoxSphere::MakeInvalid();
}

ezCustomMeshComponent::~ezCustomMeshComponent() = default;

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
  m_vCustomData = ref_msg.m_vData;

  InvalidateCachedRenderData();
}

void ezCustomMeshComponent::SetUsePrimitiveRange(ezUInt32 uiFirstPrimitive /*= 0*/, ezUInt32 uiNumPrimitives /*= ezMath::MaxValue<ezUInt32>()*/)
{
  m_uiFirstPrimitive = uiFirstPrimitive;
  m_uiNumPrimitives = uiNumPrimitives;
}

void ezCustomMeshComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_hDynamicMesh.IsValid() || !m_hMaterial.IsValid())
    return;

  ezResourceLock<ezDynamicMeshBufferResource> pMesh(m_hDynamicMesh, ezResourceAcquireMode::BlockTillLoaded);

  ezCustomMeshRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezCustomMeshRenderData>(GetOwner());
  {
    pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
    pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
    pRenderData->m_hMesh = m_hDynamicMesh;
    pRenderData->m_hMaterial = m_hMaterial;
    pRenderData->m_Color = m_Color;
    pRenderData->m_vCustomData = m_vCustomData;
    pRenderData->m_uiUniqueID = GetUniqueIdForRendering();
    pRenderData->m_uiFirstPrimitive = ezMath::Min(m_uiFirstPrimitive, pMesh->GetDescriptor().m_uiMaxPrimitives);
    pRenderData->m_uiNumPrimitives = ezMath::Min(m_uiNumPrimitives, pMesh->GetDescriptor().m_uiMaxPrimitives - pRenderData->m_uiFirstPrimitive);

    pRenderData->FillBatchIdAndSortingKey();
  }

  ezResourceLock<ezMaterialResource> pMaterial(m_hMaterial, ezResourceAcquireMode::AllowLoadingFallback);
  ezRenderData::Category category = pMaterial->GetRenderDataCategory();
  bool bDontCacheYet = pMaterial.GetAcquireResult() == ezResourceAcquireResult::LoadingFallback;

  msg.AddRenderData(pRenderData, category, bDontCacheYet ? ezRenderData::Caching::Never : ezRenderData::Caching::IfStatic);
}

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

    auto verts = pMesh->AccessVertexData();
    auto cols = pMesh->AccessColorData();

    for (ezUInt32 v = 0; v < verts.GetCount(); ++v)
    {
      verts[v].m_vPosition = geo.GetVertices()[v].m_vPosition;
      verts[v].m_vTexCoord.SetZero();
      verts[v].EncodeNormal(geo.GetVertices()[v].m_vNormal);
      verts[v].EncodeTangent(geo.GetVertices()[v].m_vTangent, 1.0f);

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

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCustomMeshRenderData, 1, ezRTTIDefaultAllocator<ezCustomMeshRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


void ezCustomMeshRenderData::FillBatchIdAndSortingKey()
{
  const ezUInt32 uiAdditionalBatchData = 0;

  m_uiFlipWinding = m_GlobalTransform.HasMirrorScaling() ? 1 : 0;
  m_uiUniformScale = m_GlobalTransform.ContainsUniformScale() ? 1 : 0;

  const ezUInt32 uiMeshIDHash = ezHashingUtils::StringHashTo32(m_hMesh.GetResourceIDHash());
  const ezUInt32 uiMaterialIDHash = m_hMaterial.IsValid() ? ezHashingUtils::StringHashTo32(m_hMaterial.GetResourceIDHash()) : 0;

  // Generate batch id from mesh, material and part index.
  ezUInt32 data[] = {uiMeshIDHash, uiMaterialIDHash, 0 /*m_uiSubMeshIndex*/, m_uiFlipWinding, uiAdditionalBatchData};
  m_uiBatchId = ezHashingUtils::xxHash32(data, sizeof(data));

  // Sort by material and then by mesh
  m_uiSortingKey = (uiMaterialIDHash << 16) | ((uiMeshIDHash + 0 /*m_uiSubMeshIndex*/) & 0xFFFE) | m_uiFlipWinding;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCustomMeshRenderer, 1, ezRTTIDefaultAllocator<ezCustomMeshRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezCustomMeshRenderer::ezCustomMeshRenderer() = default;
ezCustomMeshRenderer::~ezCustomMeshRenderer() = default;

void ezCustomMeshRenderer::GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(ezDefaultRenderDataCategories::LitOpaque);
  ref_categories.PushBack(ezDefaultRenderDataCategories::LitMasked);
  ref_categories.PushBack(ezDefaultRenderDataCategories::LitTransparent);
  ref_categories.PushBack(ezDefaultRenderDataCategories::Selection);
}

void ezCustomMeshRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(ezGetStaticRTTI<ezCustomMeshRenderData>());
}

void ezCustomMeshRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
  ezRenderContext* pRenderContext = renderViewContext.m_pRenderContext;
  ezGALCommandEncoder* pGALCommandEncoder = pRenderContext->GetCommandEncoder();

  ezInstanceData* pInstanceData = pPass->GetPipeline()->GetFrameDataProvider<ezInstanceDataProvider>()->GetData(renderViewContext);
  pInstanceData->BindResources(pRenderContext);

  const ezCustomMeshRenderData* pRenderData1st = batch.GetFirstData<ezCustomMeshRenderData>();

  if (pRenderData1st->m_uiFlipWinding)
  {
    pRenderContext->SetShaderPermutationVariable("FLIP_WINDING", "TRUE");
  }
  else
  {
    pRenderContext->SetShaderPermutationVariable("FLIP_WINDING", "FALSE");
  }

  pRenderContext->SetShaderPermutationVariable("VERTEX_SKINNING", "FALSE");

  for (auto it = batch.GetIterator<ezCustomMeshRenderData>(0, batch.GetCount()); it.IsValid(); ++it)
  {
    const ezCustomMeshRenderData* pRenderData = it;

    ezResourceLock<ezDynamicMeshBufferResource> pBuffer(pRenderData->m_hMesh, ezResourceAcquireMode::BlockTillLoaded);

    pRenderContext->BindMaterial(pRenderData->m_hMaterial);

    ezUInt32 uiInstanceDataOffset = 0;
    ezArrayPtr<ezPerInstanceData> instanceData = pInstanceData->GetInstanceData(pRenderContext,1, uiInstanceDataOffset);

    instanceData[0].GameObjectID = pRenderData->m_uiUniqueID;
    instanceData[0].Color = pRenderData->m_Color;
    instanceData[0].CustomData = pRenderData->m_vCustomData;
    instanceData[0].ObjectToWorld = pRenderData->m_GlobalTransform;

    if (pRenderData->m_uiUniformScale)
    {
      instanceData[0].ObjectToWorldNormal = instanceData[0].ObjectToWorld;
    }
    else
    {
      ezMat4 objectToWorld = pRenderData->m_GlobalTransform.GetAsMat4();

      ezMat3 mInverse = objectToWorld.GetRotationalPart();
      mInverse.Invert(0.0f).IgnoreResult();
      // we explicitly ignore the return value here (success / failure)
      // because when we have a scale of 0 (which happens temporarily during editing) that would be annoying
      instanceData[0].ObjectToWorldNormal = mInverse.GetTranspose();
    }

    pInstanceData->UpdateInstanceData(pRenderContext, 1);

    const auto& desc = pBuffer->GetDescriptor();
    pBuffer->UpdateGpuBuffer(pGALCommandEncoder);

    // redo this after the primitive count has changed
    pRenderContext->BindMeshBuffer(pRenderData->m_hMesh);

    renderViewContext.m_pRenderContext->DrawMeshBuffer(pRenderData->m_uiNumPrimitives, pRenderData->m_uiFirstPrimitive).IgnoreResult();
  }
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_CustomMeshComponent);
