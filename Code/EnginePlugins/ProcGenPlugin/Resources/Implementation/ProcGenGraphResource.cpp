#include <ProcGenPluginPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/ChunkStream.h>
#include <GameEngine/Curves/ColorGradientResource.h>
#include <GameEngine/Physics/SurfaceResource.h>
#include <GameEngine/Prefabs/PrefabResource.h>
#include <ProcGenPlugin/Resources/ProcGenGraphResource.h>
#include <ProcGenPlugin/Resources/ProcGenGraphSharedData.h>
#include <ProcGenPlugin/VM/ExpressionByteCode.h>

namespace ezProcGenInternal
{
  extern Pattern* GetPattern(ezTempHashedString sName);
}

using namespace ezProcGenInternal;

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGenGraphResource, 1, ezRTTIDefaultAllocator<ezProcGenGraphResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezProcGenGraphResource);
// clang-format on

ezProcGenGraphResource::ezProcGenGraphResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
}

ezProcGenGraphResource::~ezProcGenGraphResource() {}

const ezDynamicArray<ezSharedPtr<const PlacementOutput>>& ezProcGenGraphResource::GetPlacementOutputs() const
{
  return m_PlacementOutputs;
}

const ezDynamicArray<ezSharedPtr<const VertexColorOutput>>& ezProcGenGraphResource::GetVertexColorOutputs() const
{
  return m_VertexColorOutputs;
}

ezResourceLoadDesc ezProcGenGraphResource::UnloadData(Unload WhatToUnload)
{
  m_PlacementOutputs.Clear();
  m_VertexColorOutputs.Clear();
  m_pSharedData = nullptr;

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezProcGenGraphResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezProcGenGraphResource::UpdateContent", GetResourceDescription().GetData());

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

  // load
  {
    ezChunkStreamReader chunk(*Stream);
    chunk.SetEndChunkFileMode(ezChunkStreamReader::EndChunkFileMode::JustClose);

    chunk.BeginStream();

    ezStringBuilder sTemp;

    // skip all chunks that we don't know
    while (chunk.GetCurrentChunk().m_bValid)
    {
      if (chunk.GetCurrentChunk().m_sChunkName == "SharedData")
      {
        ezSharedPtr<GraphSharedData> pSharedData = EZ_DEFAULT_NEW(GraphSharedData);
        if (pSharedData->Load(chunk).Succeeded())
        {
          m_pSharedData = pSharedData;
        }
      }
      else if (chunk.GetCurrentChunk().m_sChunkName == "PlacementOutputs")
      {
        if (chunk.GetCurrentChunk().m_uiChunkVersion < 4)
        {
          ezLog::Error("Invalid PlacementOutputs Chunk Version {0}. Expected >= 4", chunk.GetCurrentChunk().m_uiChunkVersion);
          chunk.NextChunk();
          continue;
        }

        ezUInt32 uiNumOutputs = 0;
        chunk >> uiNumOutputs;

        m_PlacementOutputs.Reserve(uiNumOutputs);
        for (ezUInt32 uiIndex = 0; uiIndex < uiNumOutputs; ++uiIndex)
        {
          ezUniquePtr<ezExpressionByteCode> pByteCode = EZ_DEFAULT_NEW(ezExpressionByteCode);
          if (pByteCode->Load(chunk).Failed())
          {
            break;
          }

          ezSharedPtr<PlacementOutput> pOutput = EZ_DEFAULT_NEW(PlacementOutput);
          pOutput->m_pByteCode = std::move(pByteCode);

          chunk >> pOutput->m_sName;
          chunk.ReadArray(pOutput->m_VolumeTagSetIndices);

          ezUInt64 uiNumObjectsToPlace = 0;
          chunk >> uiNumObjectsToPlace;

          for (ezUInt32 uiObjectIndex = 0; uiObjectIndex < static_cast<ezUInt32>(uiNumObjectsToPlace); ++uiObjectIndex)
          {
            chunk >> sTemp;
            pOutput->m_ObjectsToPlace.ExpandAndGetRef() = ezResourceManager::LoadResource<ezPrefabResource>(sTemp);
          }

          pOutput->m_pPattern = ezProcGenInternal::GetPattern("Bayer");

          chunk >> pOutput->m_fFootprint;

          chunk >> pOutput->m_vMinOffset;
          chunk >> pOutput->m_vMaxOffset;

          chunk >> pOutput->m_fAlignToNormal;

          chunk >> pOutput->m_vMinScale;
          chunk >> pOutput->m_vMaxScale;

          chunk >> pOutput->m_fCullDistance;

          chunk >> pOutput->m_uiCollisionLayer;

          chunk >> sTemp;
          if (!sTemp.IsEmpty())
          {
            pOutput->m_hColorGradient = ezResourceManager::LoadResource<ezColorGradientResource>(sTemp);
          }

          chunk >> sTemp;
          if (!sTemp.IsEmpty())
          {
            pOutput->m_hSurface = ezResourceManager::LoadResource<ezSurfaceResource>(sTemp);
          }

          m_PlacementOutputs.PushBack(pOutput);
        }
      }
      else if (chunk.GetCurrentChunk().m_sChunkName == "VertexColorOutputs")
      {
        if (chunk.GetCurrentChunk().m_uiChunkVersion < 2)
        {
          ezLog::Error("Invalid VertexColorOutputs Chunk Version {0}. Expected >= 2", chunk.GetCurrentChunk().m_uiChunkVersion);
          chunk.NextChunk();
          continue;
        }

        ezUInt32 uiNumOutputs = 0;
        chunk >> uiNumOutputs;

        m_VertexColorOutputs.Reserve(uiNumOutputs);
        for (ezUInt32 uiIndex = 0; uiIndex < uiNumOutputs; ++uiIndex)
        {
          ezUniquePtr<ezExpressionByteCode> pByteCode = EZ_DEFAULT_NEW(ezExpressionByteCode);
          if (pByteCode->Load(chunk).Failed())
          {
            break;
          }

          ezSharedPtr<VertexColorOutput> pOutput = EZ_DEFAULT_NEW(VertexColorOutput);
          pOutput->m_pByteCode = std::move(pByteCode);

          chunk >> pOutput->m_sName;
          chunk.ReadArray(pOutput->m_VolumeTagSetIndices);

          m_VertexColorOutputs.PushBack(pOutput);
        }
      }

      chunk.NextChunk();
    }

    chunk.EndStream();

    // link shared data
    if (m_pSharedData != nullptr)
    {
      for (auto& pPlacementOutput : m_PlacementOutputs)
      {
        const_cast<PlacementOutput*>(pPlacementOutput.Borrow())->m_pGraphSharedData = m_pSharedData;
      }

      for (auto& pVertexColorOutput : m_VertexColorOutputs)
      {
        const_cast<VertexColorOutput*>(pVertexColorOutput.Borrow())->m_pGraphSharedData = m_pSharedData;
      }
    }
  }

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezProcGenGraphResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = 0;
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezProcGenGraphResource, ezProcGenGraphResourceDescriptor)
{
  // EZ_REPORT_FAILURE("This resource type does not support creating data.");

  // Missing resource

  auto pOutput = EZ_DEFAULT_NEW(PlacementOutput);
  pOutput->m_sName.Assign("MissingPlacementOutput");
  pOutput->m_ObjectsToPlace.PushBack(ezResourceManager::GetResourceTypeMissingFallback<ezPrefabResource>());
  pOutput->m_pPattern = ezProcGenInternal::GetPattern("Bayer");
  pOutput->m_fFootprint = 3.0f;
  pOutput->m_vMinOffset.Set(-1.0f, -1.0f, -0.5f);
  pOutput->m_vMaxOffset.Set(1.0f, 1.0f, 0.0f);
  pOutput->m_vMinScale.Set(1.0f, 1.0f, 1.0f);
  pOutput->m_vMaxScale.Set(1.5f, 1.5f, 2.0f);

  m_PlacementOutputs.PushBack(pOutput);
  //

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}
