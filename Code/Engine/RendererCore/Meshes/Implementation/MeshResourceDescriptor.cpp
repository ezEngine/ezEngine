#include <RendererCore/PCH.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/Logging/Log.h>

#include <CoreUtils/Assets/AssetFileHeader.h>

ezMeshResourceDescriptor::ezMeshResourceDescriptor()
{
  m_Bounds.SetInvalid();
}

ezMeshBufferResourceDescriptor& ezMeshResourceDescriptor::MeshBufferDesc()
{
  return m_MeshBufferDescriptor;
}

const ezMeshBufferResourceDescriptor& ezMeshResourceDescriptor::MeshBufferDesc() const
{
  return m_MeshBufferDescriptor;
}

void ezMeshResourceDescriptor::UseExistingMeshBuffer(const ezMeshBufferResourceHandle& hBuffer)
{
  m_hMeshBuffer = hBuffer;
}

const ezMeshBufferResourceHandle& ezMeshResourceDescriptor::GetExistingMeshBuffer() const
{
  return m_hMeshBuffer;
}

const ezHybridArray<ezMeshResourceDescriptor::Material, 32>& ezMeshResourceDescriptor::GetMaterials() const
{
  return m_Materials;
}

const ezHybridArray<ezMeshResourceDescriptor::SubMesh, 32>& ezMeshResourceDescriptor::GetSubMeshes() const
{
  return m_SubMeshes;
}

const ezBoundingBoxSphere& ezMeshResourceDescriptor::GetBounds() const
{
  return m_Bounds;
}

void ezMeshResourceDescriptor::AddSubMesh(ezUInt32 uiPrimitiveCount, ezUInt32 uiFirstPrimitive, ezUInt32 uiMaterialIndex)
{
  SubMesh p;
  p.m_uiFirstPrimitive = uiFirstPrimitive;
  p.m_uiPrimitiveCount = uiPrimitiveCount;
  p.m_uiMaterialIndex = uiMaterialIndex;
  p.m_Bounds.SetInvalid();

  m_SubMeshes.PushBack(p);
}

void ezMeshResourceDescriptor::SetMaterial(ezUInt32 uiMaterialIndex, const char* szPathToMaterial)
{
  if (uiMaterialIndex >= m_Materials.GetCount())
    m_Materials.SetCount(uiMaterialIndex + 1);

  m_Materials[uiMaterialIndex].m_sPath = szPathToMaterial;
}

ezResult ezMeshResourceDescriptor::Save(const char* szFile)
{
  EZ_LOG_BLOCK("ezMeshResourceDescriptor::Save", szFile);

  ezFileWriter file;
  if (file.Open(szFile, 1024 * 1024).Failed())
  {
    ezLog::Error("Failed to open file '%s'", szFile);
    return EZ_FAILURE;
  }

  Save(file);
  return EZ_SUCCESS;
}

void ezMeshResourceDescriptor::Save(ezStreamWriterBase& stream)
{
  ezChunkStreamWriter chunk(stream);

  chunk.BeginStream();

  {
    chunk.BeginChunk("Materials", 1);

    // number of materials
    chunk << m_Materials.GetCount();

    // each material
    for (ezUInt32 idx = 0; idx < m_Materials.GetCount(); ++idx)
    {
      chunk << idx;                       // Material Index
      chunk << m_Materials[idx].m_sPath;  // Material Path (data directory relative)
      /// \todo Material Path (relative to mesh file)
    }

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("SubMeshes", 1);

    // number of sub-meshes
    chunk << m_SubMeshes.GetCount();

    for (ezUInt32 idx = 0; idx < m_SubMeshes.GetCount(); ++idx)
    {
      chunk << idx;                                   // Sub-Mesh index
      chunk << m_SubMeshes[idx].m_uiMaterialIndex;    // The material to use
      chunk << m_SubMeshes[idx].m_uiFirstPrimitive;
      chunk << m_SubMeshes[idx].m_uiPrimitiveCount;
    }

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("MeshInfo", 2);

    // Number of vertices
    chunk << m_MeshBufferDescriptor.GetVertexCount();

    // Number of triangles
    chunk << m_MeshBufferDescriptor.GetPrimitiveCount();

    // Whether any index buffer is used
    chunk << m_MeshBufferDescriptor.HasIndexBuffer();

    // Whether the indices are 16 or 32 Bit, always false, if no index buffer is used
    chunk << (m_MeshBufferDescriptor.HasIndexBuffer() && m_MeshBufferDescriptor.Uses32BitIndices());

    // Number of vertex streams
    chunk << m_MeshBufferDescriptor.GetVertexDeclaration().m_VertexStreams.GetCount();

    for (ezUInt32 idx = 0; idx < m_MeshBufferDescriptor.GetVertexDeclaration().m_VertexStreams.GetCount(); ++idx)
    {
      const auto& vs = m_MeshBufferDescriptor.GetVertexDeclaration().m_VertexStreams[idx];

      chunk << idx;                     // Vertex stream index
      chunk << (ezInt32) vs.m_Format;
      chunk << (ezInt32) vs.m_Semantic;
      chunk << vs.m_uiElementSize;      // not needed, but can be used to check that memory layout has not changed
      chunk << vs.m_uiOffset;           // not needed, but can be used to check that memory layout has not changed
    }

    // Version 2
    CalculateBounds();
    chunk << m_Bounds.m_vCenter;
    chunk << m_Bounds.m_vBoxHalfExtends;
    chunk << m_Bounds.m_fSphereRadius;

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("VertexBuffer", 1);

    // size in bytes
    chunk << m_MeshBufferDescriptor.GetVertexBufferData().GetCount();

    if (!m_MeshBufferDescriptor.GetVertexBufferData().IsEmpty())
      chunk.WriteBytes(m_MeshBufferDescriptor.GetVertexBufferData().GetData(), m_MeshBufferDescriptor.GetVertexBufferData().GetCount());

    chunk.EndChunk();
  }

  // always write the index buffer chunk, even if it is empty
  {
    chunk.BeginChunk("IndexBuffer", 1);

    // size in bytes
    chunk << m_MeshBufferDescriptor.GetIndexBufferData().GetCount();

    if (!m_MeshBufferDescriptor.GetIndexBufferData().IsEmpty())
      chunk.WriteBytes(m_MeshBufferDescriptor.GetIndexBufferData().GetData(), m_MeshBufferDescriptor.GetIndexBufferData().GetCount());

    chunk.EndChunk();
  }

  chunk.EndStream();
}

ezResult ezMeshResourceDescriptor::Load(const char* szFile)
{
  EZ_LOG_BLOCK("ezMeshResourceDescriptor::Load", szFile);

  ezFileReader file;
  if (file.Open(szFile, 1024 * 1024).Failed())
  {
    ezLog::Error("Failed to open file '%s'", szFile);
    return EZ_FAILURE;
  }

  // skip asset header
  ezAssetFileHeader assetHeader;
  assetHeader.Read(file);

  return Load(file);
}

ezResult ezMeshResourceDescriptor::Load(ezStreamReaderBase& stream)
{
  ezChunkStreamReader chunk(stream);

  chunk.BeginStream();

  ezUInt32 count;
  bool bHasIndexBuffer = false;
  bool b32BitIndices = false;
  bool bCalculateBounds = true;

  while (chunk.GetCurrentChunk().m_bValid)
  {
    const auto& ci = chunk.GetCurrentChunk();

    if (ci.m_sChunkName == "Materials")
    {
      if (ci.m_uiChunkVersion != 1)
      {
        ezLog::Error("Version of chunk '%s' is invalid (%u)", ci.m_sChunkName.GetData(), ci.m_uiChunkVersion);
        return EZ_FAILURE;
      }

      // number of materials
      chunk >> count;
      m_Materials.SetCount(count);

      // each material
      for (ezUInt32 i = 0; i < m_Materials.GetCount(); ++i)
      {
        ezUInt32 idx;
        chunk >> idx;                       // Material Index
        chunk >> m_Materials[idx].m_sPath;  // Material Path (data directory relative)
        /// \todo Material Path (relative to mesh file)
      }
    }

    if (chunk.GetCurrentChunk().m_sChunkName == "SubMeshes")
    {
      if (ci.m_uiChunkVersion != 1)
      {
        ezLog::Error("Version of chunk '%s' is invalid (%u)", ci.m_sChunkName.GetData(), ci.m_uiChunkVersion);
        return EZ_FAILURE;
      }

      // number of sub-meshes
      chunk >> count;
      m_SubMeshes.SetCount(count);

      for (ezUInt32 i = 0; i < m_SubMeshes.GetCount(); ++i)
      {
        ezUInt32 idx;
        chunk >> idx;                                   // Sub-Mesh index
        chunk >> m_SubMeshes[idx].m_uiMaterialIndex;    // The material to use
        chunk >> m_SubMeshes[idx].m_uiFirstPrimitive;
        chunk >> m_SubMeshes[idx].m_uiPrimitiveCount;

        /// \todo load from file
        m_SubMeshes[idx].m_Bounds.SetInvalid();
      }
    }

    if (ci.m_sChunkName == "MeshInfo")
    {
      if (ci.m_uiChunkVersion > 2)
      {
        ezLog::Error("Version of chunk '%s' is invalid (%u)", ci.m_sChunkName.GetData(), ci.m_uiChunkVersion);
        return EZ_FAILURE;
      }

      // Number of vertices
      ezUInt32 uiVertexCount = 0;
      chunk >> uiVertexCount;

      // Number of triangles
      ezUInt32 uiTriangleCount = 0;
      chunk >> uiTriangleCount;

      // Whether any index buffer is used
      chunk >> bHasIndexBuffer;

      // Whether the indices are 16 or 32 Bit, always false, if no index buffer is used
      chunk >> b32BitIndices;

      // Number of vertex streams
      ezUInt32 uiStreamCount = 0;
      chunk >> uiStreamCount;

      for (ezUInt32 i = 0; i < uiStreamCount; ++i)
      {
        ezUInt32 idx;
        chunk >> idx;                     // Vertex stream index
        EZ_ASSERT_DEV(idx == i, "Invalid stream index (%u) in file (should be %u)", idx, i);

        ezInt32 iFormat, iSemantic;
        ezUInt16 uiElementSize, uiOffset;

        chunk >> iFormat;
        chunk >> iSemantic;
        chunk >> uiElementSize;      // not needed, but can be used to check that memory layout has not changed
        chunk >> uiOffset;           // not needed, but can be used to check that memory layout has not changed

        m_MeshBufferDescriptor.AddStream((ezGALVertexAttributeSemantic::Enum) iSemantic, (ezGALResourceFormat::Enum) iFormat);
      }

      m_MeshBufferDescriptor.AllocateStreams(uiVertexCount, uiTriangleCount);

      // Version 2
      if (ci.m_uiChunkVersion >= 2)
      {
        bCalculateBounds = false;
        chunk >> m_Bounds.m_vCenter;
        chunk >> m_Bounds.m_vBoxHalfExtends;
        chunk >> m_Bounds.m_fSphereRadius;
      }
    }

    if (ci.m_sChunkName == "VertexBuffer")
    {
      if (ci.m_uiChunkVersion != 1)
      {
        ezLog::Error("Version of chunk '%s' is invalid (%u)", ci.m_sChunkName.GetData(), ci.m_uiChunkVersion);
        return EZ_FAILURE;
      }

      // size in bytes
      chunk >> count;
      m_MeshBufferDescriptor.GetVertexBufferData().SetCount(count);

      if (!m_MeshBufferDescriptor.GetVertexBufferData().IsEmpty())
        chunk.ReadBytes(m_MeshBufferDescriptor.GetVertexBufferData().GetData(), m_MeshBufferDescriptor.GetVertexBufferData().GetCount());
    }

    if (ci.m_sChunkName == "IndexBuffer")
    {
      if (ci.m_uiChunkVersion != 1)
      {
        ezLog::Error("Version of chunk '%s' is invalid (%u)", ci.m_sChunkName.GetData(), ci.m_uiChunkVersion);
        return EZ_FAILURE;
      }

      // size in bytes
      chunk >> count;
      m_MeshBufferDescriptor.GetIndexBufferData().SetCount(count);

      if (!m_MeshBufferDescriptor.GetIndexBufferData().IsEmpty())
        chunk.ReadBytes(m_MeshBufferDescriptor.GetIndexBufferData().GetData(), m_MeshBufferDescriptor.GetIndexBufferData().GetCount());
    }

    chunk.NextChunk();
  }

  chunk.EndStream();

  if (bCalculateBounds)
  {
    CalculateBounds();

    auto b = m_Bounds;
    ezLog::Info("Calculated Bounds: %.2f | %.2f | %.2f - %.2f | %.2f | %.2f", b.m_vCenter.x, b.m_vCenter.y, b.m_vCenter.z, b.m_vBoxHalfExtends.x, b.m_vBoxHalfExtends.y, b.m_vBoxHalfExtends.z);
  }

  return EZ_SUCCESS;
}

void ezMeshResourceDescriptor::CalculateBounds()
{
  m_Bounds.SetInvalid();

  const ezVertexStreamInfo* pPositionStreamInfo = nullptr;
  for (auto& streamInfo : m_MeshBufferDescriptor.GetVertexDeclaration().m_VertexStreams)
  {
    if (streamInfo.m_Semantic == ezGALVertexAttributeSemantic::Position)
    {
      pPositionStreamInfo = &streamInfo;
      break;
    }
  }

  if (pPositionStreamInfo == nullptr)
    return;

  const ezVec3* pPositionData = reinterpret_cast<const ezVec3*>(m_MeshBufferDescriptor.GetVertexBufferData().GetData() + pPositionStreamInfo->m_uiOffset);
  const ezUInt32 uiStride = m_MeshBufferDescriptor.GetVertexDataSize();

  /// \todo submesh bounds
  m_Bounds.SetFromPoints(pPositionData, m_MeshBufferDescriptor.GetVertexCount(), uiStride);
}

