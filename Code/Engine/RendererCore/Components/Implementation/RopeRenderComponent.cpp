#include <RendererCorePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/Components/RopeRenderComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/SkinnedMeshComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezRopeRenderComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new ezAssetBrowserAttribute("Material")),
    EZ_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new ezDefaultValueAttribute(ezColor::White), new ezExposeColorAlphaAttribute())
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
    EZ_MESSAGE_HANDLER(ezMsgRopePoseUpdated, OnRopePoseUpdated),
    EZ_MESSAGE_HANDLER(ezMsgSetColor, OnMsgSetColor),
    EZ_MESSAGE_HANDLER(ezMsgSetMeshMaterial, OnMsgSetMeshMaterial),      
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Effects"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezRopeRenderComponent::ezRopeRenderComponent() = default;
ezRopeRenderComponent::~ezRopeRenderComponent() = default;

void ezRopeRenderComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_Color;
  s << m_hMaterial;
}

void ezRopeRenderComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_Color;
  s >> m_hMaterial;
}

void ezRopeRenderComponent::OnDeactivated()
{
  if (!m_hSkinningTransformsBuffer.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hSkinningTransformsBuffer);
    m_hSkinningTransformsBuffer.Invalidate();
  }

  SUPER::OnDeactivated();
}

ezResult ezRopeRenderComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  bounds = m_LocalBounds;
  return EZ_SUCCESS;
}

void ezRopeRenderComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  const ezUInt32 uiFlipWinding = GetOwner()->GetGlobalTransformSimd().ContainsNegativeScale() ? 1 : 0;
  const ezUInt32 uiUniformScale = GetOwner()->GetGlobalTransformSimd().ContainsUniformScale() ? 1 : 0;

  ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowLoadingFallback);
  ezMaterialResourceHandle hMaterial = m_hMaterial.IsValid() ? m_hMaterial : pMesh->GetMaterials()[0];

  ezSkinnedMeshRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezSkinnedMeshRenderData>(GetOwner());
  {
    pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
    pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
    pRenderData->m_hMesh = m_hMesh;
    pRenderData->m_hMaterial = hMaterial;
    pRenderData->m_Color = m_Color;

    pRenderData->m_uiSubMeshIndex = 0;
    pRenderData->m_uiFlipWinding = uiFlipWinding;
    pRenderData->m_uiUniformScale = uiUniformScale;

    pRenderData->m_uiUniqueID = GetUniqueIdForRendering();

    pRenderData->m_hSkinningMatrices = m_hSkinningTransformsBuffer;
    if (m_uiSkinningMatricesValidFrame == ezRenderWorld::GetFrameCounter())
    {
      pRenderData->m_pNewSkinningMatricesData = m_SkinningMatrices.ToByteArray();
    }

    pRenderData->FillBatchIdAndSortingKey();
  }

  bool bDontCacheYet = false;

  // Determine render data category.
  ezRenderData::Category category = ezDefaultRenderDataCategories::LitOpaque;

  if (hMaterial.IsValid())
  {
    ezResourceLock<ezMaterialResource> pMaterial(hMaterial, ezResourceAcquireMode::AllowLoadingFallback);

    if (pMaterial.GetAcquireResult() == ezResourceAcquireResult::LoadingFallback)
      bDontCacheYet = true;

    ezTempHashedString blendModeValue = pMaterial->GetPermutationValue("BLEND_MODE");
    if (blendModeValue == "BLEND_MODE_OPAQUE" || blendModeValue == "")
    {
      category = ezDefaultRenderDataCategories::LitOpaque;
    }
    else if (blendModeValue == "BLEND_MODE_MASKED")
    {
      category = ezDefaultRenderDataCategories::LitMasked;
    }
    else
    {
      category = ezDefaultRenderDataCategories::LitTransparent;
    }
  }

  msg.AddRenderData(pRenderData, category, bDontCacheYet ? ezRenderData::Caching::Never : ezRenderData::Caching::IfStatic);
}

void ezRopeRenderComponent::SetMaterialFile(const char* szFile)
{
  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hMaterial = ezResourceManager::LoadResource<ezMaterialResource>(szFile);
  }
  else
  {
    m_hMaterial.Invalidate();
  }
}

const char* ezRopeRenderComponent::GetMaterialFile() const
{
  if (!m_hMaterial.IsValid())
    return "";

  return m_hMaterial.GetResourceID();
}

void ezRopeRenderComponent::OnMsgSetColor(ezMsgSetColor& msg)
{
  msg.ModifyColor(m_Color);

  InvalidateCachedRenderData();
}

void ezRopeRenderComponent::OnMsgSetMeshMaterial(ezMsgSetMeshMaterial& msg)
{
  SetMaterial(msg.m_hMaterial);
}

void ezRopeRenderComponent::OnRopePoseUpdated(ezMsgRopePoseUpdated& msg)
{
  if (msg.m_LinkTransforms.IsEmpty())
    return;

  if (false)
  {
    ezHybridArray<ezDebugRenderer::Line, 128> lines;
    lines.Reserve(msg.m_LinkTransforms.GetCount());

    for (ezUInt32 i = 1; i < msg.m_LinkTransforms.GetCount(); ++i)
    {
      auto& l = lines.ExpandAndGetRef();
      l.m_start = msg.m_LinkTransforms[i - 1].m_vPosition;
      l.m_end = msg.m_LinkTransforms[i].m_vPosition;
      l.m_startColor = ezColor::White;
      l.m_endColor = ezColor::White;
    }

    ezDebugRenderer::DrawLines(GetWorld(), lines, m_Color, GetOwner()->GetGlobalTransform());
  }

  if (m_uiNumRopePieces != msg.m_LinkTransforms.GetCount())
  {
    m_uiNumRopePieces = msg.m_LinkTransforms.GetCount();

    GenerateRenderMesh();
  }

  UpdateSkinningTransformBuffer(msg.m_LinkTransforms);

  ezBoundingSphere newBounds;
  newBounds.SetFromPoints(&msg.m_LinkTransforms[0].m_vPosition, msg.m_LinkTransforms.GetCount(), sizeof(ezTransform));

  // if the existing bounds are big enough, don't update them
  if (!m_LocalBounds.IsValid() || !m_LocalBounds.GetSphere().Contains(newBounds))
  {
    m_LocalBounds = newBounds;

    TriggerLocalBoundsUpdate();
  }
}

void ezRopeRenderComponent::GenerateRenderMesh()
{
  // TODO: expose
  float m_fRadius = 0.1f;
  ezUInt32 uiNumSegments = 6;  

  ezStringBuilder sResourceName;
  sResourceName.Format("Rope-Mesh:{}-r{}-s{}", m_uiNumRopePieces, m_fRadius, uiNumSegments);

  m_hMesh = ezResourceManager::GetExistingResource<ezMeshResource>(sResourceName);
  if (m_hMesh.IsValid())
    return;

  ezGeometry geom;

  // vertices
  const ezAngle fDegStep = ezAngle::Degree(360.0f / uiNumSegments);
  for (ezUInt32 p = 0; p < m_uiNumRopePieces; ++p)
  {
    for (ezUInt32 s = 0; s <= uiNumSegments; ++s)
    {
      const ezAngle deg = (float)s * fDegStep;

      const float fY = ezMath::Cos(deg);
      const float fZ = ezMath::Sin(deg);

      const ezVec3 vDir(0, fY, fZ);

      geom.AddVertex(vDir * m_fRadius, vDir, ezVec2(0, 0), ezColor::White, p);
    }
  }

  // side polygons
  for (ezUInt32 p = 1; p < m_uiNumRopePieces; ++p)
  {
    ezUInt32 startIndex = (p - 1) * (uiNumSegments + 1);
    ezUInt32 endIndex = (p) * (uiNumSegments + 1);

    for (ezUInt32 s = 0; s < uiNumSegments; ++s)
    {
      ezUInt32 quad[4];
      quad[0] = startIndex + s;
      quad[1] = startIndex + s + 1;
      quad[2] = endIndex + s + 1;
      quad[3] = endIndex + s;

      geom.AddPolygon(quad, false);
    }
  }

  // cap polygons
  {
    ezHybridArray<ezUInt32, 16> indices;
    indices.SetCount(uiNumSegments);
    for (ezUInt32 s = 0; s < uiNumSegments; ++s)
    {
      indices[s] = s;
    }

    geom.AddPolygon(indices, true);
  }

  geom.TriangulatePolygons();
  geom.ComputeTangents();

  ezMeshResourceDescriptor desc;

  // Data/Base/Materials/Prototyping/PrototypeBlack.ezMaterialAsset
  desc.SetMaterial(0, "{ d615cd66-0904-00ca-81f9-768ff4fc24ee }");

  desc.MeshBufferDesc().AddCommonStreams();
  desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::BoneWeights0, ezGALResourceFormat::RGBAUByteNormalized);
  desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::BoneIndices0, ezGALResourceFormat::RGBAUByte);
  desc.MeshBufferDesc().AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

  desc.AddSubMesh(desc.MeshBufferDesc().GetPrimitiveCount(), 0, 0);

  desc.ComputeBounds();

  m_hMesh = ezResourceManager::CreateResource<ezMeshResource>(sResourceName, std::move(desc), sResourceName);
}

void ezRopeRenderComponent::UpdateSkinningTransformBuffer(ezArrayPtr<const ezTransform> skinningTransforms)
{
  ezArrayPtr<ezMat4> pRenderMatrices = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezMat4, skinningTransforms.GetCount());
  for (ezUInt32 i = 0; i < skinningTransforms.GetCount(); ++i)
  {
    pRenderMatrices[i] = skinningTransforms[i].GetAsMat4();
  }

  if (m_hSkinningTransformsBuffer.IsInvalidated())
  {
    ezGALBufferCreationDescription BufferDesc;
    BufferDesc.m_uiStructSize = sizeof(ezMat4);
    BufferDesc.m_uiTotalSize = BufferDesc.m_uiStructSize * pRenderMatrices.GetCount();
    BufferDesc.m_bUseAsStructuredBuffer = true;
    BufferDesc.m_bAllowShaderResourceView = true;
    BufferDesc.m_ResourceAccess.m_bImmutable = false;

    m_hSkinningTransformsBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(BufferDesc, pRenderMatrices.ToByteArray());
  }
  else
  {
    m_SkinningMatrices = pRenderMatrices;
    m_uiSkinningMatricesValidFrame = ezRenderWorld::GetFrameCounter();
  }
}
