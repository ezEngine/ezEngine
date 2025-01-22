#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshBufferResource, 1, ezRTTIDefaultAllocator<ezMeshBufferResource>)
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

ezArrayPtr<const ezUInt8> ezMeshBufferResourceDescriptor::GetVertexBufferData() const
{
  return m_VertexStreamData.GetArrayPtr();
}

ezArrayPtr<const ezUInt8> ezMeshBufferResourceDescriptor::GetIndexBufferData() const
{
  return m_IndexBufferData.GetArrayPtr();
}

ezDynamicArray<ezUInt8, ezAlignedAllocatorWrapper>& ezMeshBufferResourceDescriptor::GetVertexBufferData()
{
  return m_VertexStreamData;
}

ezDynamicArray<ezUInt8, ezAlignedAllocatorWrapper>& ezMeshBufferResourceDescriptor::GetIndexBufferData()
{
  return m_IndexBufferData;
}

ezUInt32 ezMeshBufferResourceDescriptor::AddStream(ezGALVertexAttributeSemantic::Enum semantic, ezGALResourceFormat::Enum format)
{
  EZ_ASSERT_DEV(m_VertexStreamData.IsEmpty(), "This function can only be called before 'AllocateStreams' is called");

  for (ezUInt32 i = 0; i < m_VertexDeclaration.m_VertexStreams.GetCount(); ++i)
  {
    EZ_ASSERT_DEV(m_VertexDeclaration.m_VertexStreams[i].m_Semantic != semantic, "The given semantic {0} is already used by a previous stream", semantic);
  }

  ezVertexStreamInfo si;

  si.m_Semantic = semantic;
  si.m_Format = format;
  si.m_uiOffset = 0;
  si.m_uiElementSize = static_cast<ezUInt16>(ezGALResourceFormat::GetBitsPerElement(format) / 8);
  m_uiVertexSize += si.m_uiElementSize;

  EZ_ASSERT_DEV(si.m_uiElementSize > 0, "Invalid Element Size. Format not supported?");

  if (!m_VertexDeclaration.m_VertexStreams.IsEmpty())
    si.m_uiOffset = m_VertexDeclaration.m_VertexStreams.PeekBack().m_uiOffset + m_VertexDeclaration.m_VertexStreams.PeekBack().m_uiElementSize;

  m_VertexDeclaration.m_VertexStreams.PushBack(si);

  return m_VertexDeclaration.m_VertexStreams.GetCount() - 1;
}

void ezMeshBufferResourceDescriptor::AddCommonStreams()
{
  AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
  AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezMeshTexCoordPrecision::ToResourceFormat(ezMeshTexCoordPrecision::Default));
  AddStream(ezGALVertexAttributeSemantic::Normal, ezMeshNormalPrecision::ToResourceFormatNormal(ezMeshNormalPrecision::Default));
  AddStream(ezGALVertexAttributeSemantic::Tangent, ezMeshNormalPrecision::ToResourceFormatTangent(ezMeshNormalPrecision::Default));
}

void ezMeshBufferResourceDescriptor::AllocateStreams(ezUInt32 uiNumVertices, ezGALPrimitiveTopology::Enum topology, ezUInt32 uiNumPrimitives, bool bZeroFill /*= false*/)
{
  EZ_ASSERT_DEV(!m_VertexDeclaration.m_VertexStreams.IsEmpty(), "You have to add streams via 'AddStream' before calling this function");

  m_Topology = topology;
  m_uiVertexCount = uiNumVertices;
  const ezUInt32 uiVertexStreamSize = m_uiVertexSize * uiNumVertices;

  if (bZeroFill)
  {
    m_VertexStreamData.SetCount(uiVertexStreamSize);
  }
  else
  {
    m_VertexStreamData.SetCountUninitialized(uiVertexStreamSize);
  }

  if (uiNumPrimitives > 0)
  {
    // use an index buffer at all
    ezUInt32 uiIndexBufferSize = ezGALPrimitiveTopology::GetIndexCount(topology, uiNumPrimitives);

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
  ezDynamicArray<ezUInt32> Indices;

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
        for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
        {
          if (ezMeshBufferUtils::EncodeNormal(geom.GetVertices()[v].m_vNormal, GetVertexData(s, v), si.m_Format).Failed())
          {
            ezLog::Error("Normal stream with format '{0}' is not supported.", (int)si.m_Format);
            break;
          }
        }
      }
      break;

      case ezGALVertexAttributeSemantic::Tangent:
      {
        for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
        {
          if (ezMeshBufferUtils::EncodeTangent(geom.GetVertices()[v].m_vTangent, geom.GetVertices()[v].m_fBiTangentSign, GetVertexData(s, v), si.m_Format).Failed())
          {
            ezLog::Error("Tangent stream with format '{0}' is not supported.", (int)si.m_Format);
            break;
          }
        }
      }
      break;

      case ezGALVertexAttributeSemantic::Color0:
      case ezGALVertexAttributeSemantic::Color1:
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
        for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
        {
          if (ezMeshBufferUtils::EncodeTexCoord(geom.GetVertices()[v].m_vTexCoord, GetVertexData(s, v), si.m_Format).Failed())
          {
            ezLog::Error("UV stream with format '{0}' is not supported.", (int)si.m_Format);
            break;
          }
        }
      }
      break;

      case ezGALVertexAttributeSemantic::BoneIndices0:
      {
        // if a bone index array is available, move the custom index into it

        if (si.m_Format == ezGALResourceFormat::RGBAUByte)
        {
          for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            ezVec4U16 boneIndices = geom.GetVertices()[v].m_BoneIndices;
            ezVec4U8 storage(static_cast<ezUInt8>(boneIndices.x), static_cast<ezUInt8>(boneIndices.y), static_cast<ezUInt8>(boneIndices.z), static_cast<ezUInt8>(boneIndices.w));
            SetVertexData<ezVec4U8>(s, v, storage);
          }
        }
        else if (si.m_Format == ezGALResourceFormat::RGBAUShort)
        {
          for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            SetVertexData<ezVec4U16>(s, v, geom.GetVertices()[v].m_BoneIndices);
          }
        }
      }
      break;

      case ezGALVertexAttributeSemantic::BoneWeights0:
      {
        // if a bone weight array is available, set it to fully use the first bone

        if (si.m_Format == ezGALResourceFormat::RGBAUByteNormalized)
        {
          for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            SetVertexData<ezColorLinearUB>(s, v, geom.GetVertices()[v].m_BoneWeights);
          }
        }

        if (si.m_Format == ezGALResourceFormat::XYZWFloat)
        {
          for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            SetVertexData<ezVec4>(s, v, ezColor(geom.GetVertices()[v].m_BoneWeights).GetAsVec4());
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
    pIndices[0] = static_cast<ezUInt16>(uiVertex0);
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
    pIndices[0] = static_cast<ezUInt16>(uiVertex0);
    pIndices[1] = static_cast<ezUInt16>(uiVertex1);
  }
}

void ezMeshBufferResourceDescriptor::SetTriangleIndices(ezUInt32 uiTriangle, ezUInt32 uiVertex0, ezUInt32 uiVertex1, ezUInt32 uiVertex2)
{
  EZ_ASSERT_DEBUG(m_Topology == ezGALPrimitiveTopology::Triangles, "Wrong topology");
  EZ_ASSERT_DEBUG(uiVertex0 < m_uiVertexCount && uiVertex1 < m_uiVertexCount && uiVertex2 < m_uiVertexCount, "Vertex indices out of range.");

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
    pIndices[0] = static_cast<ezUInt16>(uiVertex0);
    pIndices[1] = static_cast<ezUInt16>(uiVertex1);
    pIndices[2] = static_cast<ezUInt16>(uiVertex2);
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
  ezBoundingBoxSphere bounds = ezBoundingBoxSphere::MakeInvalid();

  for (ezUInt32 i = 0; i < m_VertexDeclaration.m_VertexStreams.GetCount(); ++i)
  {
    if (m_VertexDeclaration.m_VertexStreams[i].m_Semantic == ezGALVertexAttributeSemantic::Position)
    {
      EZ_ASSERT_DEBUG(m_VertexDeclaration.m_VertexStreams[i].m_Format == ezGALResourceFormat::XYZFloat, "Position format is not usable");

      const ezUInt32 offset = m_VertexDeclaration.m_VertexStreams[i].m_uiOffset;

      if (!m_VertexStreamData.IsEmpty() && m_uiVertexCount > 0)
      {
        bounds = ezBoundingBoxSphere::MakeFromPoints(reinterpret_cast<const ezVec3*>(&m_VertexStreamData[offset]), m_uiVertexCount, m_uiVertexSize);
      }

      return bounds;
    }
  }

  if (!bounds.IsValid())
  {
    bounds = ezBoundingBoxSphere::MakeFromCenterExtents(ezVec3::MakeZero(), ezVec3(0.1f), 0.1f);
  }

  return bounds;
}

ezResult ezMeshBufferResourceDescriptor::RecomputeNormals()
{
  if (m_Topology != ezGALPrimitiveTopology::Triangles)
    return EZ_FAILURE; // normals not needed

  const ezUInt32 uiVertexSize = m_uiVertexSize;
  const ezUInt8* pPositions = nullptr;
  ezUInt8* pNormals = nullptr;
  ezGALResourceFormat::Enum normalsFormat = ezGALResourceFormat::XYZFloat;

  for (ezUInt32 i = 0; i < m_VertexDeclaration.m_VertexStreams.GetCount(); ++i)
  {
    if (m_VertexDeclaration.m_VertexStreams[i].m_Semantic == ezGALVertexAttributeSemantic::Position && m_VertexDeclaration.m_VertexStreams[i].m_Format == ezGALResourceFormat::XYZFloat)
    {
      pPositions = GetVertexData(i, 0).GetPtr();
    }

    if (m_VertexDeclaration.m_VertexStreams[i].m_Semantic == ezGALVertexAttributeSemantic::Normal)
    {
      normalsFormat = m_VertexDeclaration.m_VertexStreams[i].m_Format;
      pNormals = GetVertexData(i, 0).GetPtr();
    }
  }

  if (pPositions == nullptr || pNormals == nullptr)
    return EZ_FAILURE; // there are no normals that could be recomputed

  ezDynamicArray<ezVec3> newNormals;
  newNormals.SetCountUninitialized(m_uiVertexCount);

  for (auto& n : newNormals)
  {
    n.SetZero();
  }

  ezResult res = EZ_SUCCESS;

  const ezUInt16* pIndices16 = reinterpret_cast<const ezUInt16*>(m_IndexBufferData.GetData());
  const ezUInt32* pIndices32 = reinterpret_cast<const ezUInt32*>(m_IndexBufferData.GetData());
  const bool bUseIndices32 = Uses32BitIndices();

  // Compute unnormalized triangle normals and add them to all vertices.
  // This way large triangles have an higher influence on the vertex normal.
  for (ezUInt32 triIdx = 0; triIdx < GetPrimitiveCount(); ++triIdx)
  {
    const ezUInt32 v0 = bUseIndices32 ? pIndices32[triIdx * 3 + 0] : pIndices16[triIdx * 3 + 0];
    const ezUInt32 v1 = bUseIndices32 ? pIndices32[triIdx * 3 + 1] : pIndices16[triIdx * 3 + 1];
    const ezUInt32 v2 = bUseIndices32 ? pIndices32[triIdx * 3 + 2] : pIndices16[triIdx * 3 + 2];

    const ezVec3 p0 = *reinterpret_cast<const ezVec3*>(pPositions + ezMath::SafeMultiply64(uiVertexSize, v0));
    const ezVec3 p1 = *reinterpret_cast<const ezVec3*>(pPositions + ezMath::SafeMultiply64(uiVertexSize, v1));
    const ezVec3 p2 = *reinterpret_cast<const ezVec3*>(pPositions + ezMath::SafeMultiply64(uiVertexSize, v2));

    const ezVec3 d01 = p1 - p0;
    const ezVec3 d02 = p2 - p0;

    const ezVec3 triNormal = d01.CrossRH(d02);

    if (triNormal.IsValid())
    {
      newNormals[v0] += triNormal;
      newNormals[v1] += triNormal;
      newNormals[v2] += triNormal;
    }
  }

  for (ezUInt32 i = 0; i < newNormals.GetCount(); ++i)
  {
    // normalize the new normal
    if (newNormals[i].NormalizeIfNotZero(ezVec3::MakeAxisX()).Failed())
      res = EZ_FAILURE;

    // then encode it in the target format precision and write it back to the buffer
    EZ_SUCCEED_OR_RETURN(ezMeshBufferUtils::EncodeNormal(newNormals[i], ezByteArrayPtr(pNormals + ezMath::SafeMultiply64(uiVertexSize, i), sizeof(ezVec3)), normalsFormat));
  }

  return res;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezMeshBufferResource::ezMeshBufferResource()
  : ezResource(DoUpdate::OnGraphicsResourceThreads, 1)
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

  m_hVertexBuffer = pDevice->CreateVertexBuffer(descriptor.GetVertexDataSize(), descriptor.GetVertexCount(), descriptor.GetVertexBufferData().GetArrayPtr());

  ezStringBuilder sName;
  sName.SetFormat("{0} Vertex Buffer", GetResourceDescription());
  pDevice->GetBuffer(m_hVertexBuffer)->SetDebugName(sName);

  if (descriptor.HasIndexBuffer())
  {
    const ezUInt32 uiIndexCount = ezGALPrimitiveTopology::GetIndexCount(m_Topology, m_uiPrimitiveCount);
    m_hIndexBuffer = pDevice->CreateIndexBuffer(descriptor.Uses32BitIndices() ? ezGALIndexType::UInt : ezGALIndexType::UShort, uiIndexCount, descriptor.GetIndexBufferData());

    sName.SetFormat("{0} Index Buffer", GetResourceDescription());
    pDevice->GetBuffer(m_hIndexBuffer)->SetDebugName(sName);

    // we only know the memory usage here, so we write it back to the internal variable directly and then read it in UpdateMemoryUsage() again
    ModifyMemoryUsage().m_uiMemoryGPU = descriptor.GetVertexBufferData().GetCount() + descriptor.GetIndexBufferData().GetCount();
  }
  else
  {
    // we only know the memory usage here, so we write it back to the internal variable directly and then read it in UpdateMemoryUsage() again
    ModifyMemoryUsage().m_uiMemoryGPU = descriptor.GetVertexBufferData().GetCount();
  }


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
