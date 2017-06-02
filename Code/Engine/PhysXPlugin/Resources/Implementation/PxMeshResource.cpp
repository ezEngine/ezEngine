#include <PCH.h>
#include <PhysXPlugin/Resources/PxMeshResource.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/ChunkStream.h>
#include <GameEngine/Messages/BuildNavMeshMessage.h>
#include <GameEngine/AI/NavMesh/NavMeshDescription.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPxMeshResource, 1, ezRTTIDefaultAllocator<ezPxMeshResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezPxMeshResource::ezPxMeshResource() : ezResource<ezPxMeshResource, ezPhysXMeshResourceDescriptor>(DoUpdate::OnMainThread, 1)
{
  m_pPxTriangleMesh = nullptr;
  m_pPxConvexMesh = nullptr;

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
  ezPxInputStream(ezStreamReader* pStream) : m_pStream(pStream) {}

  virtual PxU32 read(void* dest, PxU32 count) override
  {
    return (PxU32)m_pStream->ReadBytes(dest, count);
  }

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

  // load and create the PhysX mesh
  {
    ezChunkStreamReader chunk(*Stream);
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
      ezLog::Error("Could neither find a 'TriangleMesh' chunk, nor a 'ConvexMesh' chunk in the PhysXMesh file '{0}'", GetResourceID().GetData());
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

ezResourceLoadDesc ezPxMeshResource::CreateResource(const ezPhysXMeshResourceDescriptor& descriptor)
{
  EZ_REPORT_FAILURE("This resource type does not support creating data.");

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

void ezPxMeshResource::AddToNavMesh(const ezTransform& transform, ezBuildNavMeshMessage& msg) const
{
  if (GetConvexMesh() != nullptr)
  {
    const auto pConvex = GetConvexMesh();
    const ezUInt32 uiFirstVertexIdx = msg.m_pNavMeshDescription->m_Vertices.GetCount();

    for (ezUInt32 v = 0; v < pConvex->getNbVertices(); ++v)
    {
      ezVec3 pos = reinterpret_cast<const ezVec3&>(pConvex->getVertices()[v]);
      pos = transform * pos;
      ezMath::Swap(pos.y, pos.z);
      msg.m_pNavMeshDescription->m_Vertices.PushBack(pos);
    }

    const auto pIndices = pConvex->getIndexBuffer();

    for (ezUInt32 p = 0; p < pConvex->getNbPolygons(); ++p)
    {
      physx::PxHullPolygon poly;
      pConvex->getPolygonData(p, poly);

      const auto pLocalIdx = &pIndices[poly.mIndexBase];

      for (ezUInt32 tri = 2; tri < poly.mNbVerts; ++tri)
      {
        auto& triangle = msg.m_pNavMeshDescription->m_Triangles.ExpandAndGetRef();
        triangle.m_uiVertexIndices[0] = uiFirstVertexIdx + pLocalIdx[0];
        triangle.m_uiVertexIndices[2] = uiFirstVertexIdx + pLocalIdx[tri - 1];
        triangle.m_uiVertexIndices[1] = uiFirstVertexIdx + pLocalIdx[tri];
      }
    }
  }
  else if (GetTriangleMesh() != nullptr)
  {
    const auto pTriMesh = GetTriangleMesh();
    const ezUInt32 uiFirstVertexIdx = msg.m_pNavMeshDescription->m_Vertices.GetCount();

    for (ezUInt32 vtx = 0; vtx < pTriMesh->getNbVertices(); ++vtx)
    {
      ezVec3 pos = reinterpret_cast<const ezVec3&>(pTriMesh->getVertices()[vtx]);
      pos = transform * pos;
      ezMath::Swap(pos.y, pos.z);
      msg.m_pNavMeshDescription->m_Vertices.PushBack(pos);
    }

    if (pTriMesh->getTriangleMeshFlags().isSet(PxTriangleMeshFlag::e16_BIT_INDICES))
    {
      const ezUInt16* pIndices = reinterpret_cast<const ezUInt16*>(pTriMesh->getTriangles());

      for (ezUInt32 tri = 0; tri < pTriMesh->getNbTriangles(); ++tri)
      {
        auto& triangle = msg.m_pNavMeshDescription->m_Triangles.ExpandAndGetRef();
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
        auto& triangle = msg.m_pNavMeshDescription->m_Triangles.ExpandAndGetRef();
        triangle.m_uiVertexIndices[0] = uiFirstVertexIdx + pIndices[tri * 3 + 0];
        triangle.m_uiVertexIndices[2] = uiFirstVertexIdx + pIndices[tri * 3 + 1];
        triangle.m_uiVertexIndices[1] = uiFirstVertexIdx + pIndices[tri * 3 + 2];
      }
    }
  }
}

EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Resources_Implementation_PxMeshResource);

