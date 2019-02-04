#include <PCH.h>

#include <Core/Utils/WorldGeoExtractionUtil.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <Foundation/Utilities/GraphicsUtils.h>

namespace
{
  template <typename T, bool flip>
  void FillIndices(const void* pIndices, ezUInt32 uiTriangleCount, ezUInt32 uiVertexIdxOffset, ezWorldGeoExtractionUtil::Geometry& geo)
  {
    const T* pTypedIndices = static_cast<const T*>(pIndices);

    for (ezUInt32 p = 0; p < uiTriangleCount; ++p)
    {
      auto& tri = geo.m_Triangles.ExpandAndGetRef();
      tri.m_uiVertexIndices[0] = uiVertexIdxOffset + pTypedIndices[p * 3 + (flip ? 2 : 0)];
      tri.m_uiVertexIndices[1] = uiVertexIdxOffset + pTypedIndices[p * 3 + 1];
      tri.m_uiVertexIndices[2] = uiVertexIdxOffset + pTypedIndices[p * 3 + (flip ? 0 : 2)];
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off

EZ_BEGIN_COMPONENT_TYPE(ezMeshComponent, 3, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Mesh", GetMeshFile, SetMeshFile)->AddAttributes(new ezAssetBrowserAttribute("Mesh;Animated Mesh")),
    EZ_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new ezExposeColorAlphaAttribute()),
    EZ_ARRAY_ACCESSOR_PROPERTY("Materials", Materials_GetCount, Materials_GetValue, Materials_SetValue, Materials_Insert, Materials_Remove)->AddAttributes(new ezAssetBrowserAttribute("Material")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractGeometry, OnExtractGeometry)
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezMeshComponent::ezMeshComponent() = default;
ezMeshComponent::~ezMeshComponent() = default;

void ezMeshComponent::OnExtractGeometry(ezMsgExtractGeometry& msg)
{
  if (msg.m_Mode != ezWorldGeoExtractionUtil::ExtractionMode::RenderMesh)
    return;

  // ignore invalid and created resources
  {
    ezMeshResourceHandle hRenderMesh = GetMesh();
    if (!hRenderMesh.IsValid())
      return;

    ezResourceLock<ezMeshResource> pRenderMesh(hRenderMesh, ezResourceAcquireMode::PointerOnly);
    if (pRenderMesh->GetBaseResourceFlags().IsAnySet(ezResourceFlags::IsCreatedResource))
      return;
  }

  const char* szMesh = GetMeshFile();


  EZ_LOG_BLOCK("ExtractWorldGeometry_RenderMesh", szMesh);

  ezCpuMeshResourceHandle hCpuMesh = ezResourceManager::LoadResource<ezCpuMeshResource>(szMesh);

  ezResourceLock<ezCpuMeshResource> pCpuMesh(hCpuMesh, ezResourceAcquireMode::NoFallbackAllowMissing);

  if (pCpuMesh.GetAcquireResult() != ezResourceAcquireResult::Final)
  {
    ezLog::Warning("Failed to retrieve CPU mesh");
    return;
  }

  const auto& mb = pCpuMesh->GetDescriptor().MeshBufferDesc();

  if (mb.GetTopology() != ezGALPrimitiveTopology::Triangles || mb.GetPrimitiveCount() == 0 || !mb.HasIndexBuffer())
  {
    ezLog::Warning("Unsupported CPU mesh topology {0}", (int)mb.GetTopology());
    return;
  }

  const ezTransform transform = GetOwner()->GetGlobalTransform();

  const ezVertexDeclarationInfo& vdi = mb.GetVertexDeclaration();
  const ezUInt8* pRawVertexData = mb.GetVertexBufferData().GetData();

  const float* pPositions = nullptr;
  ezUInt32 uiElementStride = 0;

  for (ezUInt32 vs = 0; vs < vdi.m_VertexStreams.GetCount(); ++vs)
  {
    uiElementStride += vdi.m_VertexStreams[vs].m_uiElementSize;

    if (vdi.m_VertexStreams[vs].m_Semantic == ezGALVertexAttributeSemantic::Position)
    {
      if (vdi.m_VertexStreams[vs].m_Format != ezGALResourceFormat::RGBFloat)
      {
        ezLog::Warning("Unsupported CPU mesh vertex position format {0}", (int)vdi.m_VertexStreams[vs].m_Format);
        return; // other position formats are not supported
      }

      pPositions = (const float*)(pRawVertexData + vdi.m_VertexStreams[vs].m_uiOffset);
    }
  }

  if (pPositions == nullptr)
  {
    ezLog::Warning("No position stream found in CPU mesh");
    return;
  }

  auto& geo = *msg.m_pWorldGeometry;

  // remember the vertex index at the start
  const ezUInt32 uiVertexIdxOffset = geo.m_Vertices.GetCount();

  // write out all vertices
  for (ezUInt32 i = 0; i < mb.GetVertexCount(); ++i)
  {
    const ezVec3 pos(pPositions[0], pPositions[1], pPositions[2]);
    pPositions = ezMemoryUtils::AddByteOffset(pPositions, uiElementStride);

    auto& vert = geo.m_Vertices.ExpandAndGetRef();
    vert.m_vPosition = transform * pos;
    // vert.m_TexCoord.SetZero();
  }

  const bool bFlipTriangles = ezGraphicsUtils::IsTriangleFlipRequired(transform.GetAsMat4().GetRotationalPart());
  if (bFlipTriangles)
  {
    if (mb.Uses32BitIndices())
    {
      FillIndices<ezUInt32, true>(mb.GetIndexBufferData().GetData(), mb.GetPrimitiveCount(), uiVertexIdxOffset, geo);
    }
    else
    {
      FillIndices<ezUInt16, true>(mb.GetIndexBufferData().GetData(), mb.GetPrimitiveCount(), uiVertexIdxOffset, geo);
    }
  }
  else
  {
    if (mb.Uses32BitIndices())
    {
      FillIndices<ezUInt32, false>(mb.GetIndexBufferData().GetData(), mb.GetPrimitiveCount(), uiVertexIdxOffset, geo);
    }
    else
    {
      FillIndices<ezUInt16, false>(mb.GetIndexBufferData().GetData(), mb.GetPrimitiveCount(), uiVertexIdxOffset, geo);
    }
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshComponent);

