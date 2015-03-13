#include <RendererCore/PCH.h>
#include <RendererCore/Meshes/MeshFormat.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/IO/ChunkStream.h>

ezMeshBufferResourceDescriptor& ezMeshFormatBuilder::MeshBufferDesc()
{
  return m_MeshBufferDescriptor;
}

void ezMeshFormatBuilder::AddSubMesh(ezUInt32 uiPrimitiveCount, ezUInt32 uiFirstPrimitive, ezUInt32 uiMaterialIndex)
{
  SubMesh p;
  p.m_uiFirstPrimitive = uiFirstPrimitive;
  p.m_uiMaterialIndex = uiMaterialIndex;
  p.m_uiPrimitiveCount = uiPrimitiveCount;

  m_SubMeshes.PushBack(p);
}

void ezMeshFormatBuilder::SetMaterial(ezUInt32 uiMaterialIndex, const char* szPathToMaterial)
{
  if (uiMaterialIndex >= m_Materials.GetCount())
    m_Materials.SetCount(uiMaterialIndex + 1);

  m_Materials[uiMaterialIndex].m_sPath = szPathToMaterial;
}

ezResult ezMeshFormatBuilder::WriteMeshFormat(const char* szFile)
{
  EZ_LOG_BLOCK("ezMeshFormatBuilder::WriteMeshFormat", szFile);

  ezFileWriter file;
  if (file.Open(szFile, 1024 * 1024).Failed())
  {
    ezLog::Error("Failed to open file '%s'", szFile);
    return EZ_FAILURE;
  }

  WriteMeshFormat(file);
  return EZ_SUCCESS;
}

void ezMeshFormatBuilder::WriteMeshFormat(ezStreamWriterBase& stream)
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
    chunk.BeginChunk("MeshInfo", 1);

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

