#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/Physics/SurfaceResource.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/MemoryStream.h>
#include <Jolt/Core/StreamIn.h>
#include <Jolt/Physics/Collision/Shape/CompoundShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>
#include <JoltPlugin/Shapes/Implementation/JoltCustomShapeInfo.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <RendererCore/Meshes/CpuMeshResource.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

class ezJoltStreamIn : public JPH::StreamIn
{
public:
  ezStreamReader* m_pStream = nullptr;
  bool m_bEOF = false;

  virtual void ReadBytes(void* outData, size_t inNumBytes) override
  {
    if (m_pStream->ReadBytes(outData, inNumBytes) < inNumBytes)
      m_bEOF = true;
  }

  virtual bool IsEOF() const override
  {
    return m_bEOF;
  }

  virtual bool IsFailed() const override
  {
    return false;
  }
};

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezJoltMeshResource, 1, ezRTTIDefaultAllocator<ezJoltMeshResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezJoltMeshResource);
// clang-format on

ezJoltMeshResource::ezJoltMeshResource()
  : ezResource(DoUpdate::OnMainThread, 1)
{
  m_Bounds = ezBoundingBoxSphere(ezVec3::ZeroVector(), ezVec3::ZeroVector(), 0);

  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(ezJoltMeshResource);
}

ezJoltMeshResource::~ezJoltMeshResource() = default;

ezResourceLoadDesc ezJoltMeshResource::UnloadData(Unload WhatToUnload)
{
  for (auto pMesh : m_CookedConvexMeshes)
  {
    if (pMesh != nullptr)
    {
      EZ_DEFAULT_DELETE(pMesh);
    }
  }

  for (auto pMesh : m_CookedConvexMeshInstances)
  {
    if (pMesh != nullptr)
    {
      pMesh->Release();
    }
  }

  if (m_CookedTriangleMeshInstance)
  {
    m_CookedTriangleMeshInstance->Release();
    m_CookedTriangleMeshInstance = nullptr;
  }

  m_CookedConvexMeshes.Clear();
  m_CookedConvexMeshInstances.Clear();

  m_CookedTriangleMesh.Clear();
  m_CookedTriangleMesh.Compact();

  m_uiNumTriangles = 0;
  m_uiNumVertices = 0;

  // we cannot compute this in UpdateMemoryUsage(), so we only read the data there, therefore we need to update this information here
  /// \todo Compute memory usage
  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(ezJoltMeshResource);

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

EZ_DEFINE_AS_POD_TYPE(JPH::Vec3);

static void ReadConvexMesh(ezStreamReader& stream, ezDataBuffer* pBuffer)
{
  ezUInt32 uiSize = 0;

  stream >> uiSize;
  pBuffer->SetCountUninitialized(uiSize);
  EZ_VERIFY(stream.ReadBytes(pBuffer->GetData(), uiSize) == uiSize, "Reading cooked convex mesh data failed.");
}

static void AddStats(ezStreamReader& stream, ezUInt32& uiVertices, ezUInt32& uiTriangles)
{
  ezUInt32 verts = 0, tris = 0;

  stream >> verts;
  stream >> tris;

  uiVertices += verts;
  uiTriangles += tris;
}

ezResourceLoadDesc ezJoltMeshResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezJoltMeshResource::UpdateContent", GetResourceDescription().GetData());

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  m_uiNumTriangles = 0;
  m_uiNumVertices = 0;

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

  // load and create the Jolt mesh
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
        ezUInt32 uiBufferSize = 0;
        chunk >> uiBufferSize;

        m_CookedTriangleMesh.SetCountUninitialized(uiBufferSize);
        chunk.ReadBytes(m_CookedTriangleMesh.GetData(), uiBufferSize);
        AddStats(chunk, m_uiNumVertices, m_uiNumTriangles);
      }

      if (chunk.GetCurrentChunk().m_sChunkName == "ConvexMesh")
      {
        m_CookedConvexMeshes.PushBack(EZ_DEFAULT_NEW(ezDataBuffer));
        m_CookedConvexMeshInstances.SetCount(1);
        ReadConvexMesh(chunk, m_CookedConvexMeshes.PeekBack());
        AddStats(chunk, m_uiNumVertices, m_uiNumTriangles);
      }

      if (chunk.GetCurrentChunk().m_sChunkName == "ConvexDecompositionMesh")
      {
        ezUInt16 uiNumParts = 0;
        chunk >> uiNumParts;

        m_CookedConvexMeshes.Reserve(uiNumParts);
        m_CookedConvexMeshInstances.SetCount(uiNumParts);

        for (ezUInt32 i = 0; i < uiNumParts; ++i)
        {
          m_CookedConvexMeshes.PushBack(EZ_DEFAULT_NEW(ezDataBuffer));
          ReadConvexMesh(chunk, m_CookedConvexMeshes.PeekBack());
          AddStats(chunk, m_uiNumVertices, m_uiNumTriangles);
        }
      }

      chunk.NextChunk();
    }

    if (m_CookedTriangleMesh.IsEmpty() && m_CookedConvexMeshes.IsEmpty())
    {
      ezLog::Error("Could neither find a 'TriangleMesh' chunk, nor a 'ConvexMesh' chunk in the JoltMesh file '{0}'", GetResourceID());
    }

    chunk.EndStream();
  }

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezJoltMeshResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezJoltMeshResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;

  out_NewMemoryUsage.m_uiMemoryCPU += m_Surfaces.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryCPU += m_CookedTriangleMesh.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryCPU += m_CookedConvexMeshes.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryCPU += m_CookedConvexMeshInstances.GetHeapMemoryUsage();

  for (const auto pConvex : m_CookedConvexMeshes)
  {
    out_NewMemoryUsage.m_uiMemoryCPU += pConvex->GetHeapMemoryUsage();
  }
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezJoltMeshResource, ezJoltMeshResourceDescriptor)
{
  // creates just an empty mesh

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

void RetrieveShapeTriangles(const JPH::Shape* pShape, ezDynamicArray<ezVec3>& positions)
{
  const int iMaxTris = 256;

  ezDynamicArray<ezVec3> positionsTmp;
  positionsTmp.SetCount(iMaxTris * 3);

  JPH::Shape::GetTrianglesContext ctxt;

  pShape->GetTrianglesStart(ctxt, JPH::AABox::sBiggest(), pShape->GetCenterOfMass(), JPH::Quat::sIdentity(), JPH::Vec3(1, 1, 1));

  while (true)
  {
    int found = pShape->GetTrianglesNext(ctxt, iMaxTris, reinterpret_cast<JPH::Float3*>(positionsTmp.GetData()), nullptr);

    positions.PushBackRange(positionsTmp.GetArrayPtr().GetSubArray(0, found * 3));

    if (found == 0)
      return;
  }
}

void RetrieveShapeTriangles(JPH::ShapeSettings* pShapeOpt, ezDynamicArray<ezVec3>& positions)
{
  auto res = pShapeOpt->Create();

  if (res.HasError())
    return;

  RetrieveShapeTriangles(res.Get(), positions);
}

ezCpuMeshResourceHandle ezJoltMeshResource::ConvertToCpuMesh() const
{
  ezCpuMeshResourceHandle hCpuMesh = ezResourceManager::GetExistingResource<ezCpuMeshResource>(GetResourceID());
  if (hCpuMesh.IsValid())
    return hCpuMesh;

  ezMeshResourceDescriptor desc;
  desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);

  ezDynamicArray<ezVec3> positions;
  positions.Reserve(256);

  if (!m_CookedConvexMeshes.IsEmpty())
  {
    for (ezUInt32 i = 0; i < m_CookedConvexMeshes.GetCount(); ++i)
    {
      auto pShape = InstantiateConvexPart(i, 0, nullptr, 1);
      RetrieveShapeTriangles(pShape, positions);
      pShape->Release();
    }
  }

  if (!m_CookedTriangleMesh.IsEmpty())
  {
    auto pShape = InstantiateTriangleMesh(0, {});
    RetrieveShapeTriangles(pShape, positions);
    pShape->Release();
  }

  if (positions.IsEmpty())
    return {};

  desc.MeshBufferDesc().AllocateStreams(positions.GetCount(), ezGALPrimitiveTopology::Triangles);
  desc.MeshBufferDesc().GetVertexBufferData().GetArrayPtr().CopyFrom(positions.GetByteArrayPtr());

  desc.AddSubMesh(desc.MeshBufferDesc().GetPrimitiveCount(), 0, 0);
  desc.ComputeBounds();

  return ezResourceManager::GetOrCreateResource<ezCpuMeshResource>(GetResourceID(), std::move(desc), GetResourceDescription());
}

JPH::Shape* ezJoltMeshResource::InstantiateTriangleMesh(ezUInt64 uiUserData, const ezDynamicArray<const ezJoltMaterial*>& materials) const
{
  EZ_ASSERT_DEV(!m_CookedTriangleMesh.IsEmpty(), "Jolt mesh resource doesn't contain a triangle mesh.");

  if (m_CookedTriangleMeshInstance == nullptr)
  {
    ezRawMemoryStreamReader memReader(m_CookedTriangleMesh);

    ezJoltStreamIn jStream;
    jStream.m_pStream = &memReader;

    auto shapeRes = JPH::Shape::sRestoreFromBinaryState(jStream);

    if (shapeRes.HasError())
    {
      EZ_REPORT_FAILURE("Failed to instantiate Jolt triangle mesh: {}", shapeRes.GetError().c_str());
      return nullptr;
    }

    shapeRes.Get()->AddRef();

    ezHybridArray<JPH::PhysicsMaterialRefC, 32> materials;
    materials.SetCount(m_Surfaces.GetCount());

    for (ezUInt32 i = 0; i < m_Surfaces.GetCount(); ++i)
    {
      if (!m_Surfaces[i].IsValid())
        continue;

      ezResourceLock pSurf(m_Surfaces[i], ezResourceAcquireMode::BlockTillLoaded);
      const ezJoltMaterial* pMat = static_cast<const ezJoltMaterial*>(pSurf->m_pPhysicsMaterialJolt);

      materials[i] = pMat;
    }

    shapeRes.Get()->RestoreMaterialState(materials.GetData(), materials.GetCount());

    m_CookedTriangleMeshInstance = shapeRes.Get();
    m_CookedTriangleMeshInstance->AddRef();
  }

  {
    ezJoltCustomShapeInfo* pShapeDeco = new ezJoltCustomShapeInfo(m_CookedTriangleMeshInstance);
    pShapeDeco->SetUserData(uiUserData);

    if (materials.GetCount() == m_Surfaces.GetCount())
    {
      pShapeDeco->m_CustomMaterials.SetCount(materials.GetCount());
      for (ezUInt32 i = 0; i < materials.GetCount(); ++i)
      {
        pShapeDeco->m_CustomMaterials[i] = materials[i];
      }
    }

    pShapeDeco->AddRef();
    return pShapeDeco;
  }
}

JPH::Shape* ezJoltMeshResource::InstantiateConvexPart(ezUInt32 uiPartIdx, ezUInt64 uiUserData, const ezJoltMaterial* pMaterial, float fDensity) const
{
  EZ_ASSERT_DEV(!m_CookedConvexMeshes.IsEmpty(), "Jolt mesh resource doesn't contain any convex mesh.");

  if (m_CookedConvexMeshInstances[uiPartIdx] == nullptr)
  {
    ezRawMemoryStreamReader memReader(*m_CookedConvexMeshes[uiPartIdx]);

    ezJoltStreamIn jStream;
    jStream.m_pStream = &memReader;

    auto shapeRes = JPH::Shape::sRestoreFromBinaryState(jStream);

    if (shapeRes.HasError())
    {
      EZ_REPORT_FAILURE("Failed to instantiate Jolt triangle mesh: {}", shapeRes.GetError().c_str());
      return nullptr;
    }

    JPH::ConvexShape* pConvexShape = static_cast<JPH::ConvexShape*>(shapeRes.Get().GetPtr());
    pConvexShape->SetDensity(1.0f); // density will be multiplied by the decoration shape, so set the base value to 1

    ezHybridArray<JPH::PhysicsMaterialRefC, 1> materials;
    materials.SetCount(m_Surfaces.GetCount());

    for (ezUInt32 i = 0; i < m_Surfaces.GetCount(); ++i)
    {
      if (!m_Surfaces[i].IsValid())
        continue;

      ezResourceLock pSurf(m_Surfaces[i], ezResourceAcquireMode::BlockTillLoaded);
      const ezJoltMaterial* pMat = static_cast<const ezJoltMaterial*>(pSurf->m_pPhysicsMaterialJolt);

      materials[i] = pMat;
    }


    EZ_ASSERT_DEBUG(materials.GetCount() <= 1, "Convex meshes should only have a single material. '{}' has {}", GetResourceDescription(), materials.GetCount());
    shapeRes.Get()->RestoreMaterialState(materials.GetData(), materials.GetCount());


    m_CookedConvexMeshInstances[uiPartIdx] = shapeRes.Get();
    m_CookedConvexMeshInstances[uiPartIdx]->AddRef();
  }

  {
    ezJoltCustomShapeInfo* pShapeDeco = new ezJoltCustomShapeInfo(m_CookedConvexMeshInstances[uiPartIdx]);
    pShapeDeco->SetUserData(uiUserData);
    pShapeDeco->m_fDensity = fDensity;

    if (pMaterial && pMaterial->m_pSurface != nullptr)
    {
      pShapeDeco->m_CustomMaterials.SetCount(1);
      pShapeDeco->m_CustomMaterials[0] = pMaterial;
    }

    pShapeDeco->AddRef();
    return pShapeDeco;
  }
}
