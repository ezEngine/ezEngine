#include <ProcGenPluginPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/ChunkStream.h>
#include <GameEngine/Curves/ColorGradientResource.h>
#include <GameEngine/Prefabs/PrefabResource.h>
#include <GameEngine/Physics/SurfaceResource.h>
#include <ProcGenPlugin/Resources/ProcGenGraphResource.h>
#include <ProcGenPlugin/VM/ExpressionByteCode.h>

namespace ezProcGenInternal
{
  extern Pattern* GetPattern(ezTempHashedString sName);
}

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

const ezDynamicArray<ezSharedPtr<const ezProcGenInternal::PlacementOutput>>& ezProcGenGraphResource::GetPlacementOutputs() const
{
  return m_PlacementOutputs;
}

ezResourceLoadDesc ezProcGenGraphResource::UnloadData(Unload WhatToUnload)
{
  m_ByteCode.Clear();
  m_PlacementOutputs.Clear();

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

    ezHashTable<ezProcGenInternal::PlacementOutput*, ezUInt32> placementOutputToByteCodeIndex;

    // skip all chunks that we don't know
    while (chunk.GetCurrentChunk().m_bValid)
    {
      if (chunk.GetCurrentChunk().m_sChunkName == "ByteCode")
      {
        ezUInt32 uiNumByteCodes = 0;
        chunk >> uiNumByteCodes;

        m_ByteCode.SetCount(uiNumByteCodes);
        for (ezUInt32 uiByteCodeIndex = 0; uiByteCodeIndex < uiNumByteCodes; ++uiByteCodeIndex)
        {
          m_ByteCode[uiByteCodeIndex].Load(chunk);
        }
      }
      else if (chunk.GetCurrentChunk().m_sChunkName == "PlacementOutputs")
      {
        ezUInt32 uiNumOutputs = 0;
        chunk >> uiNumOutputs;

        m_PlacementOutputs.Reserve(uiNumOutputs);
        for (ezUInt32 uiIndex = 0; uiIndex < uiNumOutputs; ++uiIndex)
        {
          auto pOutput = EZ_DEFAULT_NEW(ezProcGenInternal::PlacementOutput);

          chunk >> sTemp;
          pOutput->m_sName.Assign(sTemp.GetData());

          ezUInt32 uiNumObjectsToPlace = 0;
          chunk >> uiNumObjectsToPlace;

          for (ezUInt32 uiObjectIndex = 0; uiObjectIndex < uiNumObjectsToPlace; ++uiObjectIndex)
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

          if (chunk.GetCurrentChunk().m_uiChunkVersion >= 2)
          {
            chunk >> pOutput->m_uiCollisionLayer;

            chunk >> sTemp;

            if (!sTemp.IsEmpty())
            {
              pOutput->m_hColorGradient = ezResourceManager::LoadResource<ezColorGradientResource>(sTemp);
            }
          }

          ezUInt32 uiByteCodeIndex = ezInvalidIndex;
          chunk >> uiByteCodeIndex;

          placementOutputToByteCodeIndex.Insert(pOutput, uiByteCodeIndex);

          if (chunk.GetCurrentChunk().m_uiChunkVersion >= 3)
          {
            chunk >> sTemp;

            if (!sTemp.IsEmpty())
            {
              pOutput->m_hSurface = ezResourceManager::LoadResource<ezSurfaceResource>(sTemp);
            }
          }

          m_PlacementOutputs.PushBack(pOutput);
        }
      }

      chunk.NextChunk();
    }

    chunk.EndStream();

    // link bytecode
    for (auto it = placementOutputToByteCodeIndex.GetIterator(); it.IsValid(); ++it)
    {
      ezUInt32 uiByteCodeIndex = it.Value();
      if (uiByteCodeIndex != ezInvalidIndex)
      {
        it.Key()->m_pByteCode = &m_ByteCode[uiByteCodeIndex];
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

  auto pOutput = EZ_DEFAULT_NEW(ezProcGenInternal::PlacementOutput);
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
