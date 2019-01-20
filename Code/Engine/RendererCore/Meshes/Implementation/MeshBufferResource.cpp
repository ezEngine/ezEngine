#include <PCH.h>

#include <Core/Graphics/Geometry.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshBufferResource, 1, ezRTTIDefaultAllocator<ezMeshBufferResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezMeshBufferResource);
// clang-format on

ezMeshBufferResourceDescriptor::ezMeshBufferResourceDescriptor()
{
  m_Topology = ezGALPrimitiveTopology::Triangles;
  m_uiVertexSize = 0;
  m_uiVertexCount = 0;
}

ezMeshBufferResourceDescriptor::~ezMeshBufferResourceDescriptor() = default;

void ezMeshBufferResourceDescriptor::Clear()
{
  m_Topology = ezGALPrimitiveTopology::Triangles;
  m_uiVertexSize = 0;
  m_uiVertexCount = 0;
  m_VertexDeclaration.m_uiHash = 0;
  m_VertexDeclaration.m_VertexStreams.Clear();
  m_VertexStreamData.Clear();
  m_IndexBufferData.Clear();
}

const ezDynamicArray<ezUInt8>& ezMeshBufferResourceDescriptor::GetVertexBufferData() const
{
  return m_VertexStreamData;
}

const ezDynamicArray<ezUInt8>& ezMeshBufferResourceDescriptor::GetIndexBufferData() const
{
  return m_IndexBufferData;
}

ezDynamicArray<ezUInt8>& ezMeshBufferResourceDescriptor::GetVertexBufferData()
{
  EZ_ASSERT_DEV(!m_VertexStreamData.IsEmpty(), "The vertex data must be allocated first");
  return m_VertexStreamData;
}

ezDynamicArray<ezUInt8>& ezMeshBufferResourceDescriptor::GetIndexBufferData()
{
  EZ_ASSERT_DEV(!m_IndexBufferData.IsEmpty(), "The index data must be allocated first");
  return m_IndexBufferData;
}

ezUInt32 ezMeshBufferResourceDescriptor::AddStream(ezGALVertexAttributeSemantic::Enum Semantic, ezGALResourceFormat::Enum Format)
{
  EZ_ASSERT_DEV(m_VertexStreamData.IsEmpty(), "This function can only be called before 'AllocateStreams' is called");

  for (ezUInt32 i = 0; i < m_VertexDeclaration.m_VertexStreams.GetCount(); ++i)
  {
    EZ_ASSERT_DEV(m_VertexDeclaration.m_VertexStreams[i].m_Semantic != Semantic,
                  "The given semantic {0} is already used by a previous stream", Semantic);
  }

  ezVertexStreamInfo si;

  si.m_Semantic = Semantic;
  si.m_Format = Format;
  si.m_uiOffset = 0;
  si.m_uiElementSize = ezGALResourceFormat::GetBitsPerElement(Format) / 8;
  m_uiVertexSize += si.m_uiElementSize;

  EZ_ASSERT_DEV(si.m_uiElementSize > 0, "Invalid Element Size. Format not supported?");

  if (!m_VertexDeclaration.m_VertexStreams.IsEmpty())
    si.m_uiOffset =
        m_VertexDeclaration.m_VertexStreams.PeekBack().m_uiOffset + m_VertexDeclaration.m_VertexStreams.PeekBack().m_uiElementSize;

  m_VertexDeclaration.m_VertexStreams.PushBack(si);

  return m_VertexDeclaration.m_VertexStreams.GetCount() - 1;
}

void ezMeshBufferResourceDescriptor::AllocateStreams(ezUInt32 uiNumVertices, ezGALPrimitiveTopology::Enum topology,
                                                     ezUInt32 uiNumPrimitives)
{
  EZ_ASSERT_DEV(!m_VertexDeclaration.m_VertexStreams.IsEmpty(), "You have to add streams via 'AddStream' before calling this function");

  m_Topology = topology;
  m_uiVertexCount = uiNumVertices;
  const ezUInt32 uiVertexStreamSize = m_uiVertexSize * uiNumVertices;

  m_VertexStreamData.SetCountUninitialized(uiVertexStreamSize);

  if (uiNumPrimitives > 0)
  {
    // use an index buffer at all
    ezUInt32 uiIndexBufferSize = uiNumPrimitives * ezGALPrimitiveTopology::VerticesPerPrimitive(topology);

    if (Uses32BitIndices())
    {
      uiIndexBufferSize *= sizeof(ezUInt32);
    }
    else
    {
      uiIndexBufferSize *= sizeof(ezUInt16);
    }

    m_IndexBufferData.SetCountUninitialized(uiIndexBufferSize);
  }
}

void ezMeshBufferResourceDescriptor::AllocateStreamsFromGeometry(const ezGeometry& geom, ezGALPrimitiveTopology::Enum topology)
{
  ezLogBlock _("Allocate Streams From Geometry");

  // Index Buffer Generation
  ezDynamicArray<ezUInt16> Indices;

  if (topology == ezGALPrimitiveTopology::Points)
  {
    // Leaving indices empty disables indexed rendering.
  }
  else if (topology == ezGALPrimitiveTopology::Lines)
  {
    Indices.Reserve(geom.GetLines().GetCount() * 2);

    for (ezUInt32 p = 0; p < geom.GetLines().GetCount(); ++p)
    {
      Indices.PushBack(geom.GetLines()[p].m_uiStartVertex);
      Indices.PushBack(geom.GetLines()[p].m_uiEndVertex);
    }
  }
  else if (topology == ezGALPrimitiveTopology::Triangles)
  {
    Indices.Reserve(geom.GetPolygons().GetCount() * 6);

    for (ezUInt32 p = 0; p < geom.GetPolygons().GetCount(); ++p)
    {
      for (ezUInt32 v = 0; v < geom.GetPolygons()[p].m_Vertices.GetCount() - 2; ++v)
      {
        Indices.PushBack(geom.GetPolygons()[p].m_Vertices[0]);
        Indices.PushBack(geom.GetPolygons()[p].m_Vertices[v + 1]);
        Indices.PushBack(geom.GetPolygons()[p].m_Vertices[v + 2]);
      }
    }
  }
  AllocateStreams(geom.GetVertices().GetCount(), topology, Indices.GetCount() / (topology + 1));

  // Fill vertex buffer.
  for (ezUInt32 s = 0; s < m_VertexDeclaration.m_VertexStreams.GetCount(); ++s)
  {
    const ezVertexStreamInfo& si = m_VertexDeclaration.m_VertexStreams[s];
    switch (si.m_Semantic)
    {
      case ezGALVertexAttributeSemantic::Position:
      {
        if (si.m_Format == ezGALResourceFormat::XYZFloat)
        {
          for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            SetVertexData<ezVec3>(s, v, geom.GetVertices()[v].m_vPosition);
          }
        }
        else
        {
          ezLog::Error("Position stream with format '{0}' is not supported.", (int)si.m_Format);
        }
      }
      break;

      case ezGALVertexAttributeSemantic::Normal:
      {
        if (si.m_Format == ezGALResourceFormat::XYZFloat)
        {
          for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            SetVertexData<ezVec3>(s, v, geom.GetVertices()[v].m_vNormal);
          }
        }
        else
        {
          ezLog::Error("Normal stream with format '{0}' is not supported.", (int)si.m_Format);
        }
      }
      break;

      case ezGALVertexAttributeSemantic::Tangent:
      {
        if (si.m_Format == ezGALResourceFormat::XYZFloat)
        {
          for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            SetVertexData<ezVec3>(s, v, geom.GetVertices()[v].m_vTangent);
          }
        }
        else
        {
          ezLog::Error("Tangent stream with format '{0}' is not supported.", (int)si.m_Format);
        }
      }
      break;

      case ezGALVertexAttributeSemantic::Color:
      {
        if (si.m_Format == ezGALResourceFormat::RGBAUByteNormalized)
        {
          for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            SetVertexData<ezColorLinearUB>(s, v, geom.GetVertices()[v].m_Color);
          }
        }
        else
        {
          ezLog::Error("Color stream with format '{0}' is not supported.", (int)si.m_Format);
        }
      }
      break;

      case ezGALVertexAttributeSemantic::TexCoord0:
      case ezGALVertexAttributeSemantic::TexCoord1:
      {
        if (si.m_Format == ezGALResourceFormat::UVFloat)
        {
          for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            SetVertexData<ezVec2>(s, v, geom.GetVertices()[v].m_vTexCoord);
          }
        }
        else
        {
          ezLog::Error("UV stream with format '{0}' is not supported.", (int)si.m_Format);
        }
      }
      break;

      case ezGALVertexAttributeSemantic::BoneIndices0:
      {
        // if a bone index array is available, move the custom index into it

        if (si.m_Format == ezGALResourceFormat::RGBAUShort)
        {
          for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            ezVec4Template<ezUInt16> storage(geom.GetVertices()[v].m_iCustomIndex, 0, 0, 0);
            SetVertexData<ezVec4Template<ezUInt16>>(s, v, storage);
          }
        }
      }
      break;

      case ezGALVertexAttributeSemantic::BoneWeights0:
      {
        // if a bone weight array is available, set it to fully use the first bone

        if (si.m_Format == ezGALResourceFormat::XYZWFloat)
        {
          ezVec4 storage(1, 0, 0, 0);

          for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            SetVertexData<ezVec4>(s, v, storage);
          }
        }
      }
      break;

      case ezGALVertexAttributeSemantic::BoneIndices1:
      case ezGALVertexAttributeSemantic::BoneWeights1:
        // Don't error out for these semantics as they may be used by the user (e.g. breakable mesh construction)
        break;

      default:
      {
        ezLog::Error("Streams semantic '{0}' is not supported.", (int)si.m_Semantic);
      }
      break;
    }
  }

  // Fill index buffer.
  if (topology == ezGALPrimitiveTopology::Points)
  {
    for (ezUInt32 t = 0; t < Indices.GetCount(); t += 1)
    {
      SetPointIndices(t, Indices[t]);
    }
  }
  else if (topology == ezGALPrimitiveTopology::Triangles)
  {
    for (ezUInt32 t = 0; t < Indices.GetCount(); t += 3)
    {
      SetTriangleIndices(t / 3, Indices[t], Indices[t + 1], Indices[t + 2]);
    }
  }
  else if (topology == ezGALPrimitiveTopology::Lines)
  {
    for (ezUInt32 t = 0; t < Indices.GetCount(); t += 2)
    {
      SetLineIndices(t / 2, Indices[t], Indices[t + 1]);
    }
  }
}

void ezMeshBufferResourceDescriptor::SetPointIndices(ezUInt32 uiPoint, ezUInt32 uiVertex0)
{
  EZ_ASSERT_DEBUG(m_Topology == ezGALPrimitiveTopology::Points, "Wrong topology");

  if (Uses32BitIndices())
  {
    ezUInt32* pIndices = reinterpret_cast<ezUInt32*>(&m_IndexBufferData[uiPoint * sizeof(ezUInt32) * 1]);
    pIndices[0] = uiVertex0;
  }
  else
  {
    ezUInt16* pIndices = reinterpret_cast<ezUInt16*>(&m_IndexBufferData[uiPoint * sizeof(ezUInt16) * 1]);
    pIndices[0] = uiVertex0;
  }
}

void ezMeshBufferResourceDescriptor::SetLineIndices(ezUInt32 uiLine, ezUInt32 uiVertex0, ezUInt32 uiVertex1)
{
  EZ_ASSERT_DEBUG(m_Topology == ezGALPrimitiveTopology::Lines, "Wrong topology");

  if (Uses32BitIndices())
  {
    ezUInt32* pIndices = reinterpret_cast<ezUInt32*>(&m_IndexBufferData[uiLine * sizeof(ezUInt32) * 2]);
    pIndices[0] = uiVertex0;
    pIndices[1] = uiVertex1;
  }
  else
  {
    ezUInt16* pIndices = reinterpret_cast<ezUInt16*>(&m_IndexBufferData[uiLine * sizeof(ezUInt16) * 2]);
    pIndices[0] = uiVertex0;
    pIndices[1] = uiVertex1;
  }
}

void ezMeshBufferResourceDescriptor::SetTriangleIndices(ezUInt32 uiTriangle, ezUInt32 uiVertex0, ezUInt32 uiVertex1, ezUInt32 uiVertex2)
{
  EZ_ASSERT_DEBUG(m_Topology == ezGALPrimitiveTopology::Triangles, "Wrong topology");

  if (Uses32BitIndices())
  {
    ezUInt32* pIndices = reinterpret_cast<ezUInt32*>(&m_IndexBufferData[uiTriangle * sizeof(ezUInt32) * 3]);
    pIndices[0] = uiVertex0;
    pIndices[1] = uiVertex1;
    pIndices[2] = uiVertex2;
  }
  else
  {
    ezUInt16* pIndices = reinterpret_cast<ezUInt16*>(&m_IndexBufferData[uiTriangle * sizeof(ezUInt16) * 3]);
    pIndices[0] = uiVertex0;
    pIndices[1] = uiVertex1;
    pIndices[2] = uiVertex2;
  }
}

ezUInt32 ezMeshBufferResourceDescriptor::GetPrimitiveCount() const
{
  const ezUInt32 divider = m_Topology + 1;

  if (!m_IndexBufferData.IsEmpty())
  {
    if (Uses32BitIndices())
      return (m_IndexBufferData.GetCount() / sizeof(ezUInt32)) / divider;
    else
      return (m_IndexBufferData.GetCount() / sizeof(ezUInt16)) / divider;
  }
  else
  {
    return m_uiVertexCount / divider;
  }
}

ezBoundingBoxSphere ezMeshBufferResourceDescriptor::ComputeBounds() const
{
  ezBoundingBoxSphere bounds;
  bounds.SetInvalid();

  for (ezUInt32 i = 0; i < m_VertexDeclaration.m_VertexStreams.GetCount(); ++i)
  {
    if (m_VertexDeclaration.m_VertexStreams[i].m_Semantic == ezGALVertexAttributeSemantic::Position)
    {
      EZ_ASSERT_DEBUG(m_VertexDeclaration.m_VertexStreams[i].m_Format == ezGALResourceFormat::XYZFloat, "Position format is not usable");

      const ezUInt32 offset = m_VertexDeclaration.m_VertexStreams[i].m_uiOffset;

      if (!m_VertexStreamData.IsEmpty() && m_uiVertexCount > 0)
      {
        bounds.SetFromPoints(reinterpret_cast<const ezVec3*>(&m_VertexStreamData[offset]), m_uiVertexCount, m_uiVertexSize);
      }

      return bounds;
    }
  }

  return bounds;
}

ezMeshBufferResource::ezMeshBufferResource()
    : ezResource(DoUpdate::OnAnyThread, 1)
{
}

ezMeshBufferResource::~ezMeshBufferResource()
{
  EZ_ASSERT_DEBUG(m_hVertexBuffer.IsInvalidated(), "Implementation error");
  EZ_ASSERT_DEBUG(m_hIndexBuffer.IsInvalidated(), "Implementation error");
}

ezResourceLoadDesc ezMeshBufferResource::UnloadData(Unload WhatToUnload)
{
  if (!m_hVertexBuffer.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hVertexBuffer);
    m_hVertexBuffer.Invalidate();
  }

  if (!m_hIndexBuffer.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hIndexBuffer);
    m_hIndexBuffer.Invalidate();
  }

  m_uiPrimitiveCount = 0;

  // we cannot compute this in UpdateMemoryUsage(), so we only read the data there, therefore we need to update this information here
  ModifyMemoryUsage().m_uiMemoryGPU = 0;

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezMeshBufferResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_REPORT_FAILURE("This resource type does not support loading data from file.");

  return ezResourceLoadDesc();
}

void ezMeshBufferResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  // we cannot compute this data here, so we update it wherever we know the memory usage

  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezMeshBufferResource);
  out_NewMemoryUsage.m_uiMemoryGPU = ModifyMemoryUsage().m_uiMemoryGPU;
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezMeshBufferResource, ezMeshBufferResourceDescriptor)
{
  EZ_ASSERT_DEBUG(m_hVertexBuffer.IsInvalidated(), "Implementation error");
  EZ_ASSERT_DEBUG(m_hIndexBuffer.IsInvalidated(), "Implementation error");

  m_VertexDeclaration = descriptor.GetVertexDeclaration();
  m_VertexDeclaration.ComputeHash();

  m_uiPrimitiveCount = descriptor.GetPrimitiveCount();
  m_Topology = descriptor.GetTopology();

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  m_hVertexBuffer =
      pDevice->CreateVertexBuffer(descriptor.GetVertexDataSize(), descriptor.GetVertexCount(), descriptor.GetVertexBufferData());

  ezStringBuilder sName;
  sName.Format("{0} Vertex Buffer", GetResourceDescription());
  pDevice->GetBuffer(m_hVertexBuffer)->SetDebugName(sName);

  if (descriptor.HasIndexBuffer())
  {
    m_hIndexBuffer = pDevice->CreateIndexBuffer(descriptor.Uses32BitIndices() ? ezGALIndexType::UInt : ezGALIndexType::UShort,
                                                m_uiPrimitiveCount * ezGALPrimitiveTopology::VerticesPerPrimitive(m_Topology),
                                                descriptor.GetIndexBufferData());

    sName.Format("{0} Index Buffer", GetResourceDescription());
    pDevice->GetBuffer(m_hIndexBuffer)->SetDebugName(sName);
  }

  // we only know the memory usage here, so we write it back to the internal variable directly and then read it in UpdateMemoryUsage() again
  ModifyMemoryUsage().m_uiMemoryGPU = descriptor.GetVertexBufferData().GetCount() + descriptor.GetIndexBufferData().GetCount();

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  m_Bounds = descriptor.ComputeBounds();

  return res;
}

void ezVertexDeclarationInfo::ComputeHash()
{
  m_uiHash = 0;

  for (const auto& vs : m_VertexStreams)
  {
    m_uiHash += vs.CalculateHash();

    EZ_ASSERT_DEBUG(m_uiHash != 0, "Invalid Hash Value");
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshBufferResource);
