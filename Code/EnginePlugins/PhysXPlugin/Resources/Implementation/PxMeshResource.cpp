#include <PhysXPlugin/PhysXPluginPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/ChunkStream.h>
#include <PhysXPlugin/Resources/PxMeshResource.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <RendererCore/Meshes/CpuMeshResource.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPxMeshResource, 1, ezRTTIDefaultAllocator<ezPxMeshResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezPxMeshResource);
// clang-format on

ezPxMeshResource::ezPxMeshResource()
  : ezResource(DoUpdate::OnMainThread, 1)
{
  m_Bounds = ezBoundingBoxSphere(ezVec3::ZeroVector(), ezVec3::ZeroVector(), 0);

  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(ezPxMeshResource);
}

ezPxMeshResource::~ezPxMeshResource()
{
  EZ_ASSERT_DEBUG(m_pPxTriangleMesh == nullptr, "Collision mesh was not unloaded correctly");
}

ezResourceLoadDesc ezPxMeshResource::UnloadData(Unload WhatToUnload)
{
  if (m_pPxTriangleMesh)
  {
    // since it is ref-counted, it may still be in use by the SDK, but we don't bother about that here
    m_pPxTriangleMesh->release();
    m_pPxTriangleMesh = nullptr;
  }

  for (auto pMesh : m_PxConvexParts)
  {
    if (pMesh != nullptr)
    {
      pMesh->release();
    }
  }
  m_PxConvexParts.Clear();


  // we cannot compute this in UpdateMemoryUsage(), so we only read the data there, therefore we need to update this information here
  /// \todo Compute memory usage
  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(ezPxMeshResource);

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

class ezPxInputStream : public PxInputStream
{
public:
  ezPxInputStream(ezStreamReader* pStream)
    : m_pStream(pStream)
  {
  }

  virtual PxU32 read(void* dest, PxU32 count) override { return (PxU32)m_pStream->ReadBytes(dest, count); }

  ezStreamReader* m_pStream;
};

ezResourceLoadDesc ezPxMeshResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezPhysXMeshResource::UpdateContent", GetResourceDescription().GetData());

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    ezStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  ezUInt8 uiVersion = 1;
  ezUInt8 uiCompressionMode = 0;

  if (AssetHash.GetFileVersion() >= 6) // asset document version, in version 6 the 'resource file format version' was added
  {
    *Stream >> uiVersion;
    *Stream >> uiCompressionMode;
  }

  ezStreamReader* pCompressor = Stream;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  ezCompressedStreamReaderZstd decompressorZstd;
#endif

  switch (uiCompressionMode)
  {
    case 0:
      break;

    case 1:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
      decompressorZstd.SetInputStream(Stream);
      pCompressor = &decompressorZstd;
      break;
#else
      ezLog::Error("Collision mesh is compressed with zstandard, but support for this compressor is not compiled in.");
      res.m_State = ezResourceState::LoadedResourceMissing;
      return res;
#endif

    default:
      ezLog::Error("Collision mesh is compressed with an unknown algorithm.");
      res.m_State = ezResourceState::LoadedResourceMissing;
      return res;
  }

  // load and create the PhysX mesh
  {
    ezChunkStreamReader chunk(*pCompressor);
    chunk.SetEndChunkFileMode(ezChunkStreamReader::EndChunkFileMode::JustClose);

    chunk.BeginStream();

    // skip all chunks that we don't know
    while (chunk.GetCurrentChunk().m_bValid)
    {
      if (chunk.GetCurrentChunk().m_sChunkName == "Surfaces")
      {
        ezUInt32 uiNumSurfaces = 0;
        chunk >> uiNumSurfaces;

        m_Surfaces.SetCount(uiNumSurfaces);
        ezStringBuilder sTemp;

        for (ezUInt32 surf = 0; surf < uiNumSurfaces; ++surf)
        {
          chunk >> sTemp;

          m_Surfaces[surf] = ezResourceManager::LoadResource<ezSurfaceResource>(sTemp);
        }
      }

      if (chunk.GetCurrentChunk().m_sChunkName == "Details")
      {
        chunk >> m_Bounds;
      }

      if (chunk.GetCurrentChunk().m_sChunkName == "TriangleMesh")
      {
        ezPxInputStream PassThroughStream(&chunk);

        m_pPxTriangleMesh = ezPhysX::GetSingleton()->GetPhysXAPI()->createTriangleMesh(PassThroughStream);
      }

      if (chunk.GetCurrentChunk().m_sChunkName == "ConvexMesh")
      {
        ezPxInputStream PassThroughStream(&chunk);

        m_PxConvexParts.Clear();
        m_PxConvexParts.PushBack(ezPhysX::GetSingleton()->GetPhysXAPI()->createConvexMesh(PassThroughStream));
      }

      if (chunk.GetCurrentChunk().m_sChunkName == "ConvexDecompositionMesh")
      {
        ezUInt16 uiNumParts = 0;
        chunk >> uiNumParts;

        m_PxConvexParts.SetCount(uiNumParts);

        for (ezUInt32 i = 0; i < uiNumParts; ++i)
        {
          ezPxInputStream PassThroughStream(&chunk);

          m_PxConvexParts[i] = ezPhysX::GetSingleton()->GetPhysXAPI()->createConvexMesh(PassThroughStream);
        }
      }

      chunk.NextChunk();
    }

    if (m_pPxTriangleMesh == nullptr && m_PxConvexParts.IsEmpty())
    {
      ezLog::Error("Could neither find a 'TriangleMesh' chunk, nor a 'ConvexMesh' chunk in the PhysXMesh file '{0}'", GetResourceID());
    }


    chunk.EndStream();
  }

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezPxMeshResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  // we cannot compute this data here, so we update it wherever we know the memory usage

  out_NewMemoryUsage.m_uiMemoryCPU = ModifyMemoryUsage().m_uiMemoryCPU;
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezPxMeshResource, ezPxMeshResourceDescriptor)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

ezCpuMeshResourceHandle ezPxMeshResource::ConvertToCpuMesh() const
{
  ezCpuMeshResourceHandle hCpuMesh = ezResourceManager::GetExistingResource<ezCpuMeshResource>(GetResourceID());
  if (!hCpuMesh.IsValid())
  {
    ezMeshResourceDescriptor desc;
    desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);

    if (!m_PxConvexParts.IsEmpty())
    {
      ezDynamicArray<ezVec3> positions;
      ezDynamicArray<ezUInt16> indices;
      positions.Reserve(256);
      indices.Reserve(512);

      for (auto pConvex : m_PxConvexParts)
      {
        const ezUInt16 uiFirstVertexIdx = static_cast<ezUInt16>(positions.GetCount());

        for (ezUInt32 v = 0; v < pConvex->getNbVertices(); ++v)
        {
          positions.PushBack(reinterpret_cast<const ezVec3&>(pConvex->getVertices()[v]));
        }

        const auto pIndices = pConvex->getIndexBuffer();

        for (ezUInt32 p = 0; p < pConvex->getNbPolygons(); ++p)
        {
          physx::PxHullPolygon poly;
          pConvex->getPolygonData(p, poly);

          const auto pLocalIdx = &pIndices[poly.mIndexBase];

          for (ezUInt32 tri = 2; tri < poly.mNbVerts; ++tri)
          {
            indices.PushBack(uiFirstVertexIdx + pLocalIdx[0]);
            indices.PushBack(uiFirstVertexIdx + pLocalIdx[tri - 1]);
            indices.PushBack(uiFirstVertexIdx + pLocalIdx[tri]);
          }
        }
      }

      desc.MeshBufferDesc().AllocateStreams(positions.GetCount(), ezGALPrimitiveTopology::Triangles, indices.GetCount() / 3);
      desc.MeshBufferDesc().GetVertexBufferData().GetArrayPtr().CopyFrom(positions.GetByteArrayPtr());
      desc.MeshBufferDesc().GetIndexBufferData().GetArrayPtr().CopyFrom(indices.GetByteArrayPtr());
    }
    else if (GetTriangleMesh() != nullptr)
    {
      const auto pTriMesh = GetTriangleMesh();

      desc.MeshBufferDesc().AllocateStreams(pTriMesh->getNbVertices(), ezGALPrimitiveTopology::Triangles, pTriMesh->getNbTriangles());

      auto positions = ezMakeArrayPtr(reinterpret_cast<const ezVec3*>(pTriMesh->getVertices()), pTriMesh->getNbVertices()).ToByteArray();
      desc.MeshBufferDesc().GetVertexBufferData().GetArrayPtr().CopyFrom(positions);

      const bool uses16BitIndices = pTriMesh->getTriangleMeshFlags().isSet(PxTriangleMeshFlag::e16_BIT_INDICES);
      EZ_ASSERT_DEV(uses16BitIndices == !desc.MeshBufferDesc().Uses32BitIndices(), "Index format mismatch");
      const ezUInt32 indexSize = uses16BitIndices ? sizeof(ezUInt16) : sizeof(ezUInt32);
      auto indices = ezMakeArrayPtr(static_cast<const ezUInt8*>(pTriMesh->getTriangles()), pTriMesh->getNbTriangles() * 3 * indexSize);
      desc.MeshBufferDesc().GetIndexBufferData().GetArrayPtr().CopyFrom(indices);
    }

    desc.AddSubMesh(desc.MeshBufferDesc().GetPrimitiveCount(), 0, 0);
    desc.ComputeBounds();

    hCpuMesh = ezResourceManager::GetOrCreateResource<ezCpuMeshResource>(GetResourceID(), std::move(desc), GetResourceDescription());
  }

  return hCpuMesh;
}

ezUInt32 ezPxMeshResource::GetNumPolygons() const
{
  if (!m_PxConvexParts.IsEmpty())
  {
    ezUInt32 num = 0;

    for (auto pShape : m_PxConvexParts)
    {
      num += pShape->getNbPolygons();
    }

    return num;
  }
  else if (const auto pTriangleMesh = GetTriangleMesh())
  {
    return pTriangleMesh->getNbTriangles();
  }

  return 0;
}

ezUInt32 ezPxMeshResource::GetNumVertices() const
{
  if (!m_PxConvexParts.IsEmpty())
  {
    ezUInt32 num = 0;

    for (auto pShape : m_PxConvexParts)
    {
      num += pShape->getNbVertices();
    }

    return num;
  }
  else if (const auto pTriangleMesh = GetTriangleMesh())
  {
    return pTriangleMesh->getNbVertices();
  }

  return 0;
}

EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Resources_Implementation_PxMeshResource);
