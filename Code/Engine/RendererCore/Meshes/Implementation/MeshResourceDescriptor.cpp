#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

ezMeshResourceDescriptor::ezMeshResourceDescriptor()
{
  m_Bounds = ezBoundingBoxSphere::MakeInvalid();
}

void ezMeshResourceDescriptor::Clear()
{
  m_Bounds = ezBoundingBoxSphere::MakeInvalid();
  m_hMeshBuffer.Invalidate();
  m_Materials.Clear();
  m_MeshBufferDescriptor.Clear();
  m_SubMeshes.Clear();
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

ezArrayPtr<const ezMeshResourceDescriptor::Material> ezMeshResourceDescriptor::GetMaterials() const
{
  return m_Materials;
}

ezArrayPtr<const ezMeshResourceDescriptor::SubMesh> ezMeshResourceDescriptor::GetSubMeshes() const
{
  return m_SubMeshes;
}

void ezMeshResourceDescriptor::CollapseSubMeshes()
{
  for (ezUInt32 idx = 1; idx < m_SubMeshes.GetCount(); ++idx)
  {
    m_SubMeshes[0].m_uiFirstPrimitive = ezMath::Min(m_SubMeshes[0].m_uiFirstPrimitive, m_SubMeshes[idx].m_uiFirstPrimitive);
    m_SubMeshes[0].m_uiPrimitiveCount += m_SubMeshes[idx].m_uiPrimitiveCount;

    if (m_SubMeshes[0].m_Bounds.IsValid() && m_SubMeshes[idx].m_Bounds.IsValid())
    {
      m_SubMeshes[0].m_Bounds.ExpandToInclude(m_SubMeshes[idx].m_Bounds);
    }
  }

  m_SubMeshes.SetCount(1);
  m_SubMeshes[0].m_uiMaterialIndex = 0;

  m_Materials.SetCount(1);
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
  p.m_Bounds = ezBoundingBoxSphere::MakeInvalid();

  m_SubMeshes.PushBack(p);
}

void ezMeshResourceDescriptor::SetMaterial(ezUInt32 uiMaterialIndex, const char* szPathToMaterial)
{
  m_Materials.EnsureCount(uiMaterialIndex + 1);

  m_Materials[uiMaterialIndex].m_sPath = szPathToMaterial;
}

ezResult ezMeshResourceDescriptor::Save(const char* szFile)
{
  EZ_LOG_BLOCK("ezMeshResourceDescriptor::Save", szFile);

  ezFileWriter file;
  if (file.Open(szFile, 1024 * 1024).Failed())
  {
    ezLog::Error("Failed to open file '{0}'", szFile);
    return EZ_FAILURE;
  }

  Save(file);
  return EZ_SUCCESS;
}

void ezMeshResourceDescriptor::Save(ezStreamWriter& inout_stream)
{
  ezUInt8 uiVersion = 7;
  inout_stream << uiVersion;

  ezUInt8 uiCompressionMode = 0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  uiCompressionMode = 1;
  ezCompressedStreamWriterZstd compressor(&inout_stream, 0, ezCompressedStreamWriterZstd::Compression::Average);
  ezChunkStreamWriter chunk(compressor);
#else
  ezChunkStreamWriter chunk(stream);
#endif

  inout_stream << uiCompressionMode;

  chunk.BeginStream(1);

  {
    chunk.BeginChunk("Materials", 1);

    // number of materials
    chunk << m_Materials.GetCount();

    // each material
    for (ezUInt32 idx = 0; idx < m_Materials.GetCount(); ++idx)
    {
      chunk << idx;                      // Material Index
      chunk << m_Materials[idx].m_sPath; // Material Path (data directory relative)
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
      chunk << idx;                                // Sub-Mesh index
      chunk << m_SubMeshes[idx].m_uiMaterialIndex; // The material to use
      chunk << m_SubMeshes[idx].m_uiFirstPrimitive;
      chunk << m_SubMeshes[idx].m_uiPrimitiveCount;
    }

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("MeshInfo", 4);

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

    // Version 3: Topology
    chunk << (ezUInt8)m_MeshBufferDescriptor.GetTopology();

    for (ezUInt32 idx = 0; idx < m_MeshBufferDescriptor.GetVertexDeclaration().m_VertexStreams.GetCount(); ++idx)
    {
      const auto& vs = m_MeshBufferDescriptor.GetVertexDeclaration().m_VertexStreams[idx];

      chunk << idx;                // Vertex stream index
      chunk << (ezInt32)vs.m_Format;
      chunk << (ezInt32)vs.m_Semantic;
      chunk << vs.m_uiElementSize; // not needed, but can be used to check that memory layout has not changed
      chunk << vs.m_uiOffset;      // not needed, but can be used to check that memory layout has not changed
    }

    // Version 2
    if (!m_Bounds.IsValid())
    {
      ComputeBounds();
    }

    chunk << m_Bounds.m_vCenter;
    chunk << m_Bounds.m_vBoxHalfExtents;
    chunk << m_Bounds.m_fSphereRadius;
    // Version 4
    chunk << m_fMaxBoneVertexOffset;

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("VertexBuffer", 1);

    // size in bytes
    chunk << m_MeshBufferDescriptor.GetVertexBufferData().GetCount();

    if (!m_MeshBufferDescriptor.GetVertexBufferData().IsEmpty())
    {
      chunk.WriteBytes(m_MeshBufferDescriptor.GetVertexBufferData().GetData(), m_MeshBufferDescriptor.GetVertexBufferData().GetCount()).IgnoreResult();
    }

    chunk.EndChunk();
  }

  // always write the index buffer chunk, even if it is empty
  {
    chunk.BeginChunk("IndexBuffer", 1);

    // size in bytes
    chunk << m_MeshBufferDescriptor.GetIndexBufferData().GetCount();

    if (!m_MeshBufferDescriptor.GetIndexBufferData().IsEmpty())
    {
      chunk.WriteBytes(m_MeshBufferDescriptor.GetIndexBufferData().GetData(), m_MeshBufferDescriptor.GetIndexBufferData().GetCount()).IgnoreResult();
    }

    chunk.EndChunk();
  }

  if (!m_Bones.IsEmpty())
  {
    chunk.BeginChunk("BindPose", 1);

    chunk.WriteHashTable(m_Bones).IgnoreResult();

    chunk.EndChunk();
  }

  if (m_hDefaultSkeleton.IsValid())
  {
    chunk.BeginChunk("Skeleton", 1);

    chunk << m_hDefaultSkeleton;

    chunk.EndChunk();
  }

  chunk.EndStream();

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  compressor.FinishCompressedStream().IgnoreResult();

  ezLog::Dev("Compressed mesh data from {0} KB to {1} KB ({2}%%)", ezArgF((float)compressor.GetUncompressedSize() / 1024.0f, 1), ezArgF((float)compressor.GetCompressedSize() / 1024.0f, 1), ezArgF(100.0f * compressor.GetCompressedSize() / compressor.GetUncompressedSize(), 1));
#endif
}

ezResult ezMeshResourceDescriptor::Load(const char* szFile)
{
  EZ_LOG_BLOCK("ezMeshResourceDescriptor::Load", szFile);

  ezFileReader file;
  if (file.Open(szFile, 1024 * 1024).Failed())
  {
    ezLog::Error("Failed to open file '{0}'", szFile);
    return EZ_FAILURE;
  }

  // skip asset header
  ezAssetFileHeader assetHeader;
  EZ_SUCCEED_OR_RETURN(assetHeader.Read(file));

  return Load(file);
}

ezResult ezMeshResourceDescriptor::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  // version 4 and below is broken
  if (uiVersion <= 4)
    return EZ_FAILURE;

  ezUInt8 uiCompressionMode = 0;
  if (uiVersion >= 6)
  {
    inout_stream >> uiCompressionMode;
  }

  ezStreamReader* pCompressor = &inout_stream;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  ezCompressedStreamReaderZstd decompressorZstd;
#endif

  switch (uiCompressionMode)
  {
    case 0:
      break;

    case 1:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
      decompressorZstd.SetInputStream(&inout_stream);
      pCompressor = &decompressorZstd;
      break;
#else
      ezLog::Error("Mesh is compressed with zstandard, but support for this compressor is not compiled in.");
      return EZ_FAILURE;
#endif

    default:
      ezLog::Error("Mesh is compressed with an unknown algorithm.");
      return EZ_FAILURE;
  }

  ezChunkStreamReader chunk(*pCompressor);
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
        ezLog::Error("Version of chunk '{0}' is invalid ({1})", ci.m_sChunkName, ci.m_uiChunkVersion);
        return EZ_FAILURE;
      }

      // number of materials
      chunk >> count;
      m_Materials.SetCount(count);

      // each material
      for (ezUInt32 i = 0; i < m_Materials.GetCount(); ++i)
      {
        ezUInt32 idx;
        chunk >> idx;                      // Material Index
        chunk >> m_Materials[idx].m_sPath; // Material Path (data directory relative)
        /// \todo Material Path (relative to mesh file)
      }
    }

    if (chunk.GetCurrentChunk().m_sChunkName == "SubMeshes")
    {
      if (ci.m_uiChunkVersion != 1)
      {
        ezLog::Error("Version of chunk '{0}' is invalid ({1})", ci.m_sChunkName, ci.m_uiChunkVersion);
        return EZ_FAILURE;
      }

      // number of sub-meshes
      chunk >> count;
      m_SubMeshes.SetCount(count);

      for (ezUInt32 i = 0; i < m_SubMeshes.GetCount(); ++i)
      {
        ezUInt32 idx;
        chunk >> idx;                                // Sub-Mesh index
        chunk >> m_SubMeshes[idx].m_uiMaterialIndex; // The material to use
        chunk >> m_SubMeshes[idx].m_uiFirstPrimitive;
        chunk >> m_SubMeshes[idx].m_uiPrimitiveCount;

        /// \todo load from file
        m_SubMeshes[idx].m_Bounds = ezBoundingBoxSphere::MakeInvalid();
      }
    }

    if (ci.m_sChunkName == "MeshInfo")
    {
      if (ci.m_uiChunkVersion > 4)
      {
        ezLog::Error("Version of chunk '{0}' is invalid ({1})", ci.m_sChunkName, ci.m_uiChunkVersion);
        return EZ_FAILURE;
      }

      // Number of vertices
      ezUInt32 uiVertexCount = 0;
      chunk >> uiVertexCount;

      // Number of primitives
      ezUInt32 uiPrimitiveCount = 0;
      chunk >> uiPrimitiveCount;

      // Whether any index buffer is used
      chunk >> bHasIndexBuffer;

      // Whether the indices are 16 or 32 Bit, always false, if no index buffer is used
      chunk >> b32BitIndices;

      // Number of vertex streams
      ezUInt32 uiStreamCount = 0;
      chunk >> uiStreamCount;

      ezUInt8 uiTopology = ezGALPrimitiveTopology::Triangles;
      if (ci.m_uiChunkVersion >= 3)
      {
        chunk >> uiTopology;
      }

      for (ezUInt32 i = 0; i < uiStreamCount; ++i)
      {
        ezUInt32 idx;
        chunk >> idx; // Vertex stream index
        EZ_ASSERT_DEV(idx == i, "Invalid stream index ({0}) in file (should be {1})", idx, i);

        ezInt32 iFormat, iSemantic;
        ezUInt16 uiElementSize, uiOffset;

        chunk >> iFormat;
        chunk >> iSemantic;
        chunk >> uiElementSize; // not needed, but can be used to check that memory layout has not changed
        chunk >> uiOffset;      // not needed, but can be used to check that memory layout has not changed

        if (uiVersion < 7)
        {
          // ezGALVertexAttributeSemantic got new elements inserted
          // need to adjust old file formats accordingly

          if (iSemantic >= ezGALVertexAttributeSemantic::Color2) // should be ezGALVertexAttributeSemantic::TexCoord0 instead
          {
            iSemantic += 6;
          }
        }

        m_MeshBufferDescriptor.AddStream((ezGALVertexAttributeSemantic::Enum)iSemantic, (ezGALResourceFormat::Enum)iFormat);
      }

      m_MeshBufferDescriptor.AllocateStreams(uiVertexCount, (ezGALPrimitiveTopology::Enum)uiTopology, uiPrimitiveCount);

      // Version 2
      if (ci.m_uiChunkVersion >= 2)
      {
        chunk >> m_Bounds.m_vCenter;
        chunk >> m_Bounds.m_vBoxHalfExtents;
        chunk >> m_Bounds.m_fSphereRadius;
        bCalculateBounds = !m_Bounds.IsValid();
      }

      if (ci.m_uiChunkVersion >= 4)
      {
        chunk >> m_fMaxBoneVertexOffset;
      }
    }

    if (ci.m_sChunkName == "VertexBuffer")
    {
      if (ci.m_uiChunkVersion != 1)
      {
        ezLog::Error("Version of chunk '{0}' is invalid ({1})", ci.m_sChunkName, ci.m_uiChunkVersion);
        return EZ_FAILURE;
      }

      // size in bytes
      chunk >> count;
      m_MeshBufferDescriptor.GetVertexBufferData().SetCountUninitialized(count);

      if (!m_MeshBufferDescriptor.GetVertexBufferData().IsEmpty())
        chunk.ReadBytes(m_MeshBufferDescriptor.GetVertexBufferData().GetData(), m_MeshBufferDescriptor.GetVertexBufferData().GetCount());
    }

    if (ci.m_sChunkName == "IndexBuffer")
    {
      if (ci.m_uiChunkVersion != 1)
      {
        ezLog::Error("Version of chunk '{0}' is invalid ({1})", ci.m_sChunkName, ci.m_uiChunkVersion);
        return EZ_FAILURE;
      }

      // size in bytes
      chunk >> count;
      m_MeshBufferDescriptor.GetIndexBufferData().SetCountUninitialized(count);

      if (!m_MeshBufferDescriptor.GetIndexBufferData().IsEmpty())
        chunk.ReadBytes(m_MeshBufferDescriptor.GetIndexBufferData().GetData(), m_MeshBufferDescriptor.GetIndexBufferData().GetCount());
    }

    if (ci.m_sChunkName == "BindPose")
    {
      EZ_SUCCEED_OR_RETURN(chunk.ReadHashTable(m_Bones));
    }

    if (ci.m_sChunkName == "Skeleton")
    {
      chunk >> m_hDefaultSkeleton;
    }

    chunk.NextChunk();
  }

  chunk.EndStream();

  if (bCalculateBounds)
  {
    ComputeBounds();

    auto b = m_Bounds;
    ezLog::Info("Calculated Bounds: {0} | {1} | {2} - {3} | {4} | {5}", ezArgF(b.m_vCenter.x, 2), ezArgF(b.m_vCenter.y, 2), ezArgF(b.m_vCenter.z, 2), ezArgF(b.m_vBoxHalfExtents.x, 2), ezArgF(b.m_vBoxHalfExtents.y, 2), ezArgF(b.m_vBoxHalfExtents.z, 2));
  }

  return EZ_SUCCESS;
}

void ezMeshResourceDescriptor::ComputeBounds()
{
  if (m_hMeshBuffer.IsValid())
  {
    ezResourceLock<ezMeshBufferResource> pMeshBuffer(m_hMeshBuffer, ezResourceAcquireMode::AllowLoadingFallback);
    m_Bounds = pMeshBuffer->GetBounds();
  }
  else
  {
    m_Bounds = m_MeshBufferDescriptor.ComputeBounds();
  }

  if (!m_Bounds.IsValid())
  {
    m_Bounds = ezBoundingBoxSphere::MakeFromCenterExtents(ezVec3::MakeZero(), ezVec3(0.1f), 0.1f);
  }
}

ezResult ezMeshResourceDescriptor::BoneData::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream << m_GlobalInverseRestPoseMatrix;
  inout_stream << m_uiBoneIndex;

  return EZ_SUCCESS;
}

ezResult ezMeshResourceDescriptor::BoneData::Deserialize(ezStreamReader& inout_stream)
{
  inout_stream >> m_GlobalInverseRestPoseMatrix;
  inout_stream >> m_uiBoneIndex;

  return EZ_SUCCESS;
}
