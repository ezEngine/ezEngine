#include <PCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/Utils/WorldGeoExtractionUtil.h>
#include <Foundation/IO/ChunkStream.h>
#include <PhysXPlugin/Resources/PxMeshResource.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPxMeshResource, 1, ezRTTIDefaultAllocator<ezPxMeshResource>)
  ;
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezPxMeshResource::ezPxMeshResource()
    : ezResource<ezPxMeshResource, ezPxMeshResourceDescriptor>(DoUpdate::OnMainThread, 1)
{
  m_pPxTriangleMesh = nullptr;
  m_pPxConvexMesh = nullptr;
  m_Bounds = ezBoundingBoxSphere(ezVec3::ZeroVector(), ezVec3::ZeroVector(), 0);

  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(ezPxMeshResource);
}

ezPxMeshResource::~ezPxMeshResource()
{
  EZ_ASSERT_DEBUG(m_pPxTriangleMesh == nullptr, "Collision mesh was not unloaded correctly");
  EZ_ASSERT_DEBUG(m_pPxConvexMesh == nullptr, "Collision mesh was not unloaded correctly");
}

ezResourceLoadDesc ezPxMeshResource::UnloadData(Unload WhatToUnload)
{
  if (m_pPxTriangleMesh)
  {
    // since it is ref-counted, it may still be in use by the SDK, but we don't bother about that here
    m_pPxTriangleMesh->release();
    m_pPxTriangleMesh = nullptr;
  }

  if (m_pPxConvexMesh)
  {
    // since it is ref-counted, it may still be in use by the SDK, but we don't bother about that here
    m_pPxConvexMesh->release();
    m_pPxConvexMesh = nullptr;
  }

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
    ezString sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream);

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

        m_pPxConvexMesh = ezPhysX::GetSingleton()->GetPhysXAPI()->createConvexMesh(PassThroughStream);
      }

      chunk.NextChunk();
    }

    if (m_pPxTriangleMesh == nullptr && m_pPxConvexMesh == nullptr)
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

ezResourceLoadDesc ezPxMeshResource::CreateResource(ezPxMeshResourceDescriptor&& descriptor)
{
  // EZ_REPORT_FAILURE("This resource type does not support creating data.");

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

void ezPxMeshResource::ExtractGeometry(const ezTransform& transform, ezMsgExtractGeometry& msg) const
{
  if (msg.m_Mode != ezWorldGeoExtractionUtil::ExtractionMode::CollisionMesh &&
      msg.m_Mode != ezWorldGeoExtractionUtil::ExtractionMode::NavMeshGeneration)
    return;

  if (GetConvexMesh() != nullptr)
  {
    const auto pConvex = GetConvexMesh();
    const ezUInt32 uiFirstVertexIdx = msg.m_pWorldGeometry->m_Vertices.GetCount();

    for (ezUInt32 v = 0; v < pConvex->getNbVertices(); ++v)
    {
      auto& vertex = msg.m_pWorldGeometry->m_Vertices.ExpandAndGetRef();
      vertex.m_vPosition = transform * reinterpret_cast<const ezVec3&>(pConvex->getVertices()[v]);
    }

    const auto pIndices = pConvex->getIndexBuffer();

    for (ezUInt32 p = 0; p < pConvex->getNbPolygons(); ++p)
    {
      physx::PxHullPolygon poly;
      pConvex->getPolygonData(p, poly);

      const auto pLocalIdx = &pIndices[poly.mIndexBase];

      for (ezUInt32 tri = 2; tri < poly.mNbVerts; ++tri)
      {
        auto& triangle = msg.m_pWorldGeometry->m_Triangles.ExpandAndGetRef();
        triangle.m_uiVertexIndices[0] = uiFirstVertexIdx + pLocalIdx[0];
        triangle.m_uiVertexIndices[2] = uiFirstVertexIdx + pLocalIdx[tri - 1];
        triangle.m_uiVertexIndices[1] = uiFirstVertexIdx + pLocalIdx[tri];
      }
    }
  }
  else if (GetTriangleMesh() != nullptr)
  {
    const auto pTriMesh = GetTriangleMesh();
    const ezUInt32 uiFirstVertexIdx = msg.m_pWorldGeometry->m_Vertices.GetCount();

    for (ezUInt32 vtx = 0; vtx < pTriMesh->getNbVertices(); ++vtx)
    {
      auto& vertex = msg.m_pWorldGeometry->m_Vertices.ExpandAndGetRef();
      vertex.m_vPosition = transform * reinterpret_cast<const ezVec3&>(pTriMesh->getVertices()[vtx]);
    }

    if (pTriMesh->getTriangleMeshFlags().isSet(PxTriangleMeshFlag::e16_BIT_INDICES))
    {
      const ezUInt16* pIndices = reinterpret_cast<const ezUInt16*>(pTriMesh->getTriangles());

      for (ezUInt32 tri = 0; tri < pTriMesh->getNbTriangles(); ++tri)
      {
        auto& triangle = msg.m_pWorldGeometry->m_Triangles.ExpandAndGetRef();
        triangle.m_uiVertexIndices[0] = uiFirstVertexIdx + pIndices[tri * 3 + 0];
        triangle.m_uiVertexIndices[2] = uiFirstVertexIdx + pIndices[tri * 3 + 1];
        triangle.m_uiVertexIndices[1] = uiFirstVertexIdx + pIndices[tri * 3 + 2];
      }
    }
    else
    {
      const ezUInt32* pIndices = reinterpret_cast<const ezUInt32*>(pTriMesh->getTriangles());

      for (ezUInt32 tri = 0; tri < pTriMesh->getNbTriangles(); ++tri)
      {
        auto& triangle = msg.m_pWorldGeometry->m_Triangles.ExpandAndGetRef();
        triangle.m_uiVertexIndices[0] = uiFirstVertexIdx + pIndices[tri * 3 + 0];
        triangle.m_uiVertexIndices[2] = uiFirstVertexIdx + pIndices[tri * 3 + 1];
        triangle.m_uiVertexIndices[1] = uiFirstVertexIdx + pIndices[tri * 3 + 2];
      }
    }
  }
}

EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Resources_Implementation_PxMeshResource);
