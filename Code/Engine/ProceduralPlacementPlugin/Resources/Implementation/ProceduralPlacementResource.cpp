#include <PCH.h>
#include <ProceduralPlacementPlugin/Resources/ProceduralPlacementResource.h>
#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/ChunkStream.h>

namespace ezPPInternal
{
  extern Pattern* GetPattern(ezTempHashedString sName);
}

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProceduralPlacementResource, 1, ezRTTIDefaultAllocator<ezProceduralPlacementResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezProceduralPlacementResource::ezProceduralPlacementResource()
  : ezResource<ezProceduralPlacementResource, ezProceduralPlacementResourceDescriptor>(DoUpdate::OnAnyThread, 1)
{

}

ezProceduralPlacementResource::~ezProceduralPlacementResource()
{

}

ezArrayPtr<const ezPPInternal::Layer> ezProceduralPlacementResource::GetLayers() const
{
  return m_Layers;
}

ezResourceLoadDesc ezProceduralPlacementResource::UnloadData(Unload WhatToUnload)
{


  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezProceduralPlacementResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezProceduralPlacementResource::UpdateContent", GetResourceDescription().GetData());

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
#if 0
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
      ezLog::Error("Could neither find a 'TriangleMesh' chunk, nor a 'ConvexMesh' chunk in the PhysXMesh file '{0}'", GetResourceID());
    }
#endif

    chunk.EndStream();
  }

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezProceduralPlacementResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = 0;
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

ezResourceLoadDesc ezProceduralPlacementResource::CreateResource(const ezProceduralPlacementResourceDescriptor& descriptor)
{
  //EZ_REPORT_FAILURE("This resource type does not support creating data.");

  // temp

  auto& layer = m_Layers.ExpandAndGetRef();
  layer.m_sName.Assign("TestLayer");
  layer.m_ObjectsToPlace.PushBack(ezMakeHashedString("{ 40af5788-38f5-42a2-97e2-ab541b163519 }"));
  layer.m_pPattern = ezPPInternal::GetPattern("Bayer");
  layer.m_fFootprint = 3.0f;
  layer.m_vMinOffset.Set(-1.0f, -1.0f, -1.0f);
  layer.m_vMaxOffset.Set(1.0f, 1.0f, -0.5f);
  layer.m_vMaxScale.Set(1.5f, 1.5f, 2.0f);

  //

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

