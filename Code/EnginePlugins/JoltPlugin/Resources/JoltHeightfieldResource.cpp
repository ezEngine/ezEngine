#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <JoltPlugin/Resources/JoltHeightfieldResource.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/Utilities/JoltStreamUtils.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezJoltHeightfieldResource, 1, ezRTTIDefaultAllocator<ezJoltHeightfieldResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezJoltHeightfieldResource);
// clang-format on

ezJoltHeightfieldResource::ezJoltHeightfieldResource()
  : ezResource(DoUpdate::OnMainThread, 1)
{
  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(ezJoltHeightfieldResource);
}

ezJoltHeightfieldResource::~ezJoltHeightfieldResource() = default;

ezResourceLoadDesc ezJoltHeightfieldResource::UnloadData(Unload WhatToUnload)
{
  m_uiContentHash = 0;
  m_uiCollisionLayer = 0;
  m_Surfaces.Clear();
  m_ShapeData.Clear();
  m_ShapeData.Compact();

  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(ezJoltHeightfieldResource);

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;
  return res;
}

ezResourceLoadDesc ezJoltHeightfieldResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezJoltHeightfieldResource::UpdateContent", GetResourceIdOrDescription());

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // resource loader prepends the absolute file path
  ezStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  {
    ezAssetFileHeader header;
    header.Read(*Stream).IgnoreResult();
  }

  ezUInt8 uiVersion = 0;
  ezUInt8 uiCompressionMode = 0;
  *Stream >> uiVersion;
  *Stream >> uiCompressionMode;
  *Stream >> m_uiContentHash;

  ezStreamReader* pChunkSource = Stream;

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
      pChunkSource = &decompressorZstd;
      break;
#else
      ezLog::Error("Heightfield file '{}' uses zstd compression but support is not compiled in.", GetResourceID());
      res.m_State = ezResourceState::LoadedResourceMissing;
      return res;
#endif
    default:
      ezLog::Error("Heightfield file '{}' uses unknown compression mode {}.", GetResourceID(), uiCompressionMode);
      res.m_State = ezResourceState::LoadedResourceMissing;
      return res;
  }

  ezChunkStreamReader chunk(*pChunkSource);
  chunk.SetEndChunkFileMode(ezChunkStreamReader::EndChunkFileMode::JustClose);
  chunk.BeginStream();

  while (chunk.GetCurrentChunk().m_bValid)
  {
    if (chunk.GetCurrentChunk().m_sChunkName == "Surfaces")
    {
      ezUInt32 uiNumSurfaces = 0;
      chunk >> uiNumSurfaces;
      m_Surfaces.SetCount(uiNumSurfaces);
      ezStringBuilder sTemp;
      for (ezUInt32 i = 0; i < uiNumSurfaces; ++i)
      {
        chunk >> sTemp;
        m_Surfaces[i] = ezResourceManager::LoadResource<ezSurfaceResource>(sTemp);
      }
    }

    if (chunk.GetCurrentChunk().m_sChunkName == "Heightfield")
    {
      chunk >> m_uiCollisionLayer;

      ezUInt32 uiShapeDataSize = 0;
      chunk >> uiShapeDataSize;
      m_ShapeData.SetCountUninitialized(uiShapeDataSize);
      chunk.ReadBytes(m_ShapeData.GetData(), uiShapeDataSize);
    }

    chunk.NextChunk();
  }

  chunk.EndStream();

  if (m_ShapeData.IsEmpty())
  {
    ezLog::Error("No 'Heightfield' chunk found in heightfield file '{}'", GetResourceID());
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezJoltHeightfieldResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezJoltHeightfieldResource);
  out_NewMemoryUsage.m_uiMemoryCPU += m_Surfaces.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryCPU += m_ShapeData.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezJoltHeightfieldResource, ezJoltHeightfieldResourceDescriptor)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  const ezUInt32 N = descriptor.m_uiResolution;
  const float halfX = descriptor.m_vHalfExtents.x;
  const float halfY = descriptor.m_vHalfExtents.y;

  if (N < 4 || (N % 2) != 0 || descriptor.m_Heights.GetCount() != N * N)
  {
    ezLog::Error("ezJoltHeightfieldResource: invalid height data (N={}, count={}).", N, descriptor.m_Heights.GetCount());
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  const ezUInt32 uiCellCount = (N - 1) * (N - 1);
  const bool bHasMaterials = !descriptor.m_Surfaces.IsEmpty() && !descriptor.m_MaterialIndices.IsEmpty();

  if (bHasMaterials && descriptor.m_MaterialIndices.GetCount() != uiCellCount)
  {
    ezLog::Error("ezJoltHeightfieldResource: material indices count ({}) must be (N-1)^2 = {}.", descriptor.m_MaterialIndices.GetCount(), uiCellCount);
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  m_Surfaces = descriptor.m_Surfaces;
  m_uiCollisionLayer = descriptor.m_uiCollisionLayer;

  // Resolve surface handles to Jolt material pointers.
  ezDynamicArray<const ezJoltMaterial*> materialPtrs;
  materialPtrs.SetCount(descriptor.m_Surfaces.GetCount(), nullptr);
  for (ezUInt32 i = 0; i < descriptor.m_Surfaces.GetCount(); ++i)
  {
    if (descriptor.m_Surfaces[i].IsValid())
    {
      ezResourceLock<ezSurfaceResource> pSurface(descriptor.m_Surfaces[i], ezResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pSurface.GetAcquireResult() == ezResourceAcquireResult::Final && pSurface->m_pPhysicsMaterialJolt != nullptr)
        materialPtrs[i] = reinterpret_cast<const ezJoltMaterial*>(pSurface->m_pPhysicsMaterialJolt);
    }
  }

  // Row order must be flipped so that Jolt's row axis maps to +Y in ezEngine space after the +90° X rotation.
  ezDynamicArray<float> flippedSamples;
  flippedSamples.SetCountUninitialized(N * N);
  for (ezUInt32 row = 0; row < N; ++row)
  {
    const ezUInt32 srcRow = N - 1 - row;
    for (ezUInt32 col = 0; col < N; ++col)
      flippedSamples[row * N + col] = descriptor.m_Heights[srcRow * N + col];
  }

  JPH::PhysicsMaterialList joltMaterials;
  if (bHasMaterials)
  {
    for (const ezJoltMaterial* pMat : materialPtrs)
      joltMaterials.push_back(pMat != nullptr ? pMat : ezJoltCore::GetDefaultMaterial());
  }

  JPH::HeightFieldShapeSettings settings(
    flippedSamples.GetData(),
    JPH::Vec3(-halfX, 0.0f, -halfY),
    JPH::Vec3(2.0f * halfX / static_cast<float>(N - 1), 1.0f, 2.0f * halfY / static_cast<float>(N - 1)),
    N,
    bHasMaterials ? descriptor.m_MaterialIndices.GetData() : nullptr,
    joltMaterials);

  JPH::ShapeSettings::ShapeResult result = settings.Create();
  if (result.HasError())
  {
    ezLog::Error("ezJoltHeightfieldResource: failed to create JPH::HeightFieldShape: {}", result.GetError().c_str());
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // Serialize the shape to binary state so OnSimulationStarted can use sRestoreFromBinaryState,
  // the same path used for file-loaded resources.
  ezContiguousMemoryStreamStorage storage;
  ezMemoryStreamWriter memWriter(&storage);
  ezJoltStreamOut joltOut(&memWriter);
  result.Get()->SaveBinaryState(joltOut);

  m_ShapeData.SetCountUninitialized(storage.GetStorageSize32());
  ezMemoryUtils::Copy(m_ShapeData.GetData(), storage.GetData(), storage.GetStorageSize32());

  res.m_State = ezResourceState::Loaded;
  return res;
}

EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Resources_JoltHeightfieldResource);
