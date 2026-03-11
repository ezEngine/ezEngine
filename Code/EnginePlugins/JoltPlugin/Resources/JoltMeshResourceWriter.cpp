#include <JoltPlugin/JoltPluginPCH.h>

#include <JoltPlugin/Resources/JoltMeshResourceWriter.h>

#include <Core/Graphics/ConvexHull.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/Utilities/Progress.h>

#define ENABLE_VHACD_IMPLEMENTATION 1
#include <VHACD/VHACD.h>
using namespace VHACD;

namespace
{
  class ezJoltStreamOut : public JPH::StreamOut
  {
  public:
    ezJoltStreamOut(ezStreamWriter* pPassThrough)
    {
      m_pWriter = pPassThrough;
    }

    virtual void WriteBytes(const void* pInData, size_t uiInNumBytes) override
    {
      if (m_pWriter->WriteBytes(pInData, uiInNumBytes).Failed())
        m_bFailed = true;
    }

    virtual bool IsFailed() const override
    {
      return m_bFailed;
    }


  private:
    ezStreamWriter* m_pWriter = nullptr;
    bool m_bFailed = false;
  };
} // namespace

//////////////////////////////////////////////////////////////////////////

// static
ezResult ezJoltMeshResourceWriter::WriteMeshResource(const ezJoltMeshDesc& meshDesc, ezStreamWriter& inout_stream, bool bWriteAssetHeader /*= true*/, ezUInt64 uiAssetHash /*= 0*/)
{
  if (bWriteAssetHeader)
  {
    ezAssetFileHeader header;
    header.SetFileHashAndVersion(uiAssetHash, 10); // ezGetStaticRTTI<ezJoltCollisionMeshAssetDocument>()->GetTypeVersion();
    EZ_SUCCEED_OR_RETURN(header.Write(inout_stream));
  }

  const ezUInt8 uiVersion = 3;
  ezUInt8 uiCompressionMode = 0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  uiCompressionMode = 1;
  ezCompressedStreamWriterZstd compressor(&inout_stream, 0, ezCompressedStreamWriterZstd::Compression::Average);
  ezChunkStreamWriter chunk(compressor);
#else
  ezChunkStreamWriter chunk(inout_stream);
#endif

  inout_stream << uiVersion;
  inout_stream << uiCompressionMode;

  chunk.BeginStream(1);

  // Write chunks
  {
    {
      chunk.BeginChunk("Surfaces", 1);

      chunk << meshDesc.m_Surfaces.GetCount();

      for (const ezString& sSurface : meshDesc.m_Surfaces)
      {
        chunk << sSurface;
      }

      chunk.EndChunk();
    }

    {
      chunk.BeginChunk("Details", 1);

      ezBoundingBoxSphere aabb = ezBoundingBoxSphere::MakeFromPoints(meshDesc.m_Vertices.GetData(), meshDesc.m_Vertices.GetCount());

      chunk << aabb;

      chunk.EndChunk();
    }

    ezResult resCooking = EZ_FAILURE;

    if (meshDesc.m_Type == ezJoltMeshDesc::Type::Triangle)
    {
      chunk.BeginChunk("TriangleMesh", 1);

      ezStopwatch timer;
      resCooking = CookTriangleMesh(meshDesc, chunk);
      ezLog::Dev("Triangle Mesh Cooking time: {0}s", ezArgF(timer.GetRunningTotal().GetSeconds(), 2));

      chunk.EndChunk();
    }
    else if (meshDesc.m_Type == ezJoltMeshDesc::Type::ConvexHull)
    {
      chunk.BeginChunk("ConvexMesh", 1);

      ezStopwatch timer;
      resCooking = CookConvexMesh(meshDesc, chunk);
      ezLog::Dev("Convex Mesh Cooking time: {0}s", ezArgF(timer.GetRunningTotal().GetSeconds(), 2));

      chunk.EndChunk();
    }
    else if (meshDesc.m_Type == ezJoltMeshDesc::Type::ConvexDecomposition)
    {
      chunk.BeginChunk("ConvexDecompositionMesh", 1);

      ezStopwatch timer;
      resCooking = CookDecomposedConvexMesh(meshDesc, chunk);
      ezLog::Dev("Decomposed Convex Mesh Cooking time: {0}s", ezArgF(timer.GetRunningTotal().GetSeconds(), 2));

      chunk.EndChunk();
    }
    else if (meshDesc.m_Type == ezJoltMeshDesc::Type::ConvexHullGroup)
    {
      chunk.BeginChunk("ConvexDecompositionMesh", 1);

      ezStopwatch timer;
      resCooking = CookConvexHullGroup(meshDesc, chunk);
      ezLog::Dev("Decomposed Convex Mesh Cooking time: {0}s", ezArgF(timer.GetRunningTotal().GetSeconds(), 2));

      chunk.EndChunk();
    }

    if (resCooking.Failed())
    {
      ezLog::Error("Cooking the collision mesh failed.");
      return EZ_FAILURE;
    }
  }

  chunk.EndStream();

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  if (compressor.FinishCompressedStream().Failed())
  {
    ezLog::Error("Failed to finish compressing stream.");
    return EZ_FAILURE;
  }

  ezLog::Dev("Compressed collision mesh data from {0} to {1} ({2}%%)", ezArgFileSize(compressor.GetUncompressedSize()), ezArgFileSize(compressor.GetCompressedSize()), ezArgF(100.0f * compressor.GetCompressedSize() / compressor.GetUncompressedSize(), 1));

#endif

  return EZ_SUCCESS;
}

ezResult ezJoltMeshResourceWriter::ComputeConvexHull(const ezDynamicArray<ezVec3>& vertices, ezDynamicArray<ezVec3>& out_hullVertices)
{
  ezStopwatch timer;

  ezConvexHullGenerator gen;
  if (gen.Build(vertices).Failed())
  {
    ezLog::Error("Computing the convex hull failed.");
    return EZ_FAILURE;
  }

  ezDynamicArray<ezConvexHullGenerator::Face> faces;
  gen.Retrieve(out_hullVertices, faces);

  if (faces.GetCount() >= 255)
  {
    ezConvexHullGenerator gen2;
    gen2.SetSimplificationMinTriangleAngle(ezAngle::MakeFromDegree(30));
    gen2.SetSimplificationFlatVertexNormalThreshold(ezAngle::MakeFromDegree(10));
    gen2.SetSimplificationMinTriangleEdgeLength(0.08f);

    if (gen2.Build(out_hullVertices).Failed())
    {
      ezLog::Error("Computing the convex hull failed (second try).");
      return EZ_FAILURE;
    }

    gen2.Retrieve(out_hullVertices, faces);
  }

  ezLog::Dev("Computed the convex hull in {0} milliseconds", ezArgF(timer.GetRunningTotal().GetMilliseconds(), 1));
  return EZ_SUCCESS;
}

ezResult ezJoltMeshResourceWriter::CookSingleConvexJoltMesh(const ezDynamicArray<ezVec3>& vertices, ezStreamWriter& inout_stream)
{
  if (JPH::Allocate == nullptr)
  {
    // make sure an allocator exists
    JPH::RegisterDefaultAllocator();
  }

  ezTempHybridArray<JPH::Vec3, 256> verts;
  verts.SetCountUninitialized(vertices.GetCount());

  for (ezUInt32 i = 0; i < verts.GetCount(); ++i)
  {
    verts[i] = ezJoltConversionUtils::ToVec3(vertices[i]);
  }

  JPH::ConvexHullShapeSettings shapeSettings(verts.GetData(), (int)verts.GetCount());

  auto shapeRes = shapeSettings.Create();

  if (shapeRes.HasError())
  {
    ezLog::Error("Cooking convex Jolt mesh failed: {}", shapeRes.GetError().c_str());
    return EZ_FAILURE;
  }

  ezDefaultMemoryStreamStorage storage;
  ezMemoryStreamWriter memWriter(&storage);

  ezJoltStreamOut jOut(&memWriter);
  shapeRes.Get()->SaveBinaryState(jOut);

  inout_stream << storage.GetStorageSize32();
  storage.CopyToStream(inout_stream).AssertSuccess();

  const ezUInt32 uiNumVertices = verts.GetCount();
  inout_stream << uiNumVertices;

  const ezUInt32 uiNumTriangles = shapeRes.Get()->GetStats().mNumTriangles;
  inout_stream << uiNumTriangles;

  return EZ_SUCCESS;
}

ezResult ezJoltMeshResourceWriter::CookTriangleMesh(const ezJoltMeshDesc& meshDesc, ezStreamWriter& inout_stream)
{
  if (JPH::Allocate == nullptr)
  {
    // make sure an allocator exists
    JPH::RegisterDefaultAllocator();
  }

  JPH::VertexList vertexList;
  JPH::IndexedTriangleList triangleList;

  // copy vertices
  {
    vertexList.resize(meshDesc.m_Vertices.GetCount());
    for (ezUInt32 i = 0; i < meshDesc.m_Vertices.GetCount(); ++i)
    {
      vertexList[i] = ezJoltConversionUtils::ToFloat3(meshDesc.m_Vertices[i]);
    }
  }

  ezUInt32 uiMaxMaterialIndex = 0;

  // copy triangles
  {
    triangleList.reserve(meshDesc.m_TriangleSurfaceID.GetCount());
    for (ezUInt32 i = 0; i < meshDesc.m_TriangleSurfaceID.GetCount(); ++i)
    {
      const ezUInt32 uiMaterialID = meshDesc.m_TriangleSurfaceID[i];
      if (uiMaterialID == 0xFFFF)
        continue;

      uiMaxMaterialIndex = ezMath::Max(uiMaxMaterialIndex, uiMaterialID);

      const ezUInt32 idx0 = meshDesc.m_TriangleIndices[i * 3 + 0];
      const ezUInt32 idx1 = meshDesc.m_TriangleIndices[i * 3 + 1];
      const ezUInt32 idx2 = meshDesc.m_TriangleIndices[i * 3 + 2];

      if (idx0 == idx1 || idx0 == idx2 || idx1 == idx2)
      {
        // triangle is degenerate, skip it
        continue;
      }

      const ezVec3 v0 = ezJoltConversionUtils::ToVec3(vertexList[idx0]);
      const ezVec3 v1 = ezJoltConversionUtils::ToVec3(vertexList[idx1]);
      const ezVec3 v2 = ezJoltConversionUtils::ToVec3(vertexList[idx2]);

      if (v0.IsEqual(v1, 0.001f) || v0.IsEqual(v2, 0.001f) || v1.IsEqual(v2, 0.001f))
      {
        // triangle is degenerate, skip it
        continue;
      }

      auto& triangle = triangleList.emplace_back();
      triangle.mMaterialIndex = uiMaterialID;
      triangle.mIdx[0] = idx0;
      triangle.mIdx[1] = idx1;
      triangle.mIdx[2] = idx2;
    }
  }

  // cook mesh (create Jolt shape, then save to binary stream)
  {
    JPH::MeshShapeSettings meshSettings(vertexList, triangleList);
    meshSettings.mMaterials.resize(uiMaxMaterialIndex + 1);

    auto shapeRes = meshSettings.Create();

    if (shapeRes.HasError())
    {
      ezLog::Error("Cooking Jolt triangle mesh failed: {}", shapeRes.GetError().c_str());
      return EZ_FAILURE;
    }

    ezDefaultMemoryStreamStorage storage;
    ezMemoryStreamWriter memWriter(&storage);

    ezJoltStreamOut jOut(&memWriter);
    shapeRes.Get()->SaveBinaryState(jOut);

    inout_stream << storage.GetStorageSize32();
    storage.CopyToStream(inout_stream).AssertSuccess();

    const ezUInt32 uiNumVertices = static_cast<ezUInt32>(vertexList.size());
    inout_stream << uiNumVertices;

    const ezUInt32 uiNumTriangles = shapeRes.Get()->GetStats().mNumTriangles;
    inout_stream << uiNumTriangles;
  }

  return EZ_SUCCESS;
}

ezResult ezJoltMeshResourceWriter::CookConvexMesh(const ezJoltMeshDesc& meshDesc, ezStreamWriter& inout_stream)
{
  ezProgressRange range("Cooking Convex Mesh", 2, false);

  range.BeginNextStep("Computing Convex Hull");

  ezTempHybridArray<ezVec3, 256> hullVertices;
  EZ_SUCCEED_OR_RETURN(ComputeConvexHull(meshDesc.m_Vertices, hullVertices));

  range.BeginNextStep("Cooking Convex Hull");

  EZ_SUCCEED_OR_RETURN(CookSingleConvexJoltMesh(hullVertices, inout_stream));

  return EZ_SUCCESS;
}

ezResult ezJoltMeshResourceWriter::CookDecomposedConvexMesh(const ezJoltMeshDesc& meshDesc, ezStreamWriter& inout_stream)
{
  EZ_LOG_BLOCK("Decomposing Mesh");

  IVHACD* pConDec = CreateVHACD();
  IVHACD::Parameters params;
  params.m_maxConvexHulls = ezMath::Max(1u, meshDesc.m_uiMaxConvexPieces);

  if (meshDesc.m_uiMaxConvexPieces <= 2)
  {
    params.m_resolution = 10 * 10 * 10;
  }
  else if (meshDesc.m_uiMaxConvexPieces <= 5)
  {
    params.m_resolution = 20 * 20 * 20;
  }
  else if (meshDesc.m_uiMaxConvexPieces <= 10)
  {
    params.m_resolution = 40 * 40 * 40;
  }
  else if (meshDesc.m_uiMaxConvexPieces <= 25)
  {
    params.m_resolution = 60 * 60 * 60;
  }
  else if (meshDesc.m_uiMaxConvexPieces <= 50)
  {
    params.m_resolution = 80 * 80 * 80;
  }
  else
  {
    params.m_resolution = 100 * 100 * 100;
  }

  if (!pConDec->Compute(meshDesc.m_Vertices.GetData()->GetData(), meshDesc.m_Vertices.GetCount(), meshDesc.m_TriangleIndices.GetData(), meshDesc.m_TriangleSurfaceID.GetCount(), params))
  {
    ezLog::Error("Failed to compute convex decomposition");
    return EZ_FAILURE;
  }

  ezUInt16 uiNumParts = 0;

  for (ezUInt32 i = 0; i < pConDec->GetNConvexHulls(); ++i)
  {
    IVHACD::ConvexHull ch;
    pConDec->GetConvexHull(i, ch);

    if (ch.m_triangles.empty())
      continue;

    ++uiNumParts;
  }

  ezLog::Dev("Convex mesh parts: {}", uiNumParts);

  inout_stream << uiNumParts;

  ezTempHybridArray<ezVec3, 256> hullVertices;
  for (ezUInt32 i = 0; i < pConDec->GetNConvexHulls(); ++i)
  {
    IVHACD::ConvexHull ch;
    pConDec->GetConvexHull(i, ch);

    if (ch.m_triangles.empty())
      continue;

    hullVertices.SetCount((ezUInt32)ch.m_points.size());

    for (ezUInt32 v = 0; v < (ezUInt32)ch.m_points.size(); ++v)
    {
      hullVertices[v].Set((float)ch.m_points[v].mX, (float)ch.m_points[v].mY, (float)ch.m_points[v].mZ);
    }

    EZ_SUCCEED_OR_RETURN(CookSingleConvexJoltMesh(hullVertices, inout_stream));
  }

  return EZ_SUCCESS;
}

ezResult ezJoltMeshResourceWriter::CookConvexHullGroup(const ezJoltMeshDesc& meshDesc, ezStreamWriter& inout_stream)
{
  ezMap<ezUInt16, ezDynamicArray<ezVec3>> parts;

  ezUInt32 uiVertexIdx = 0;
  for (ezUInt32 faceIdx = 0; faceIdx < meshDesc.m_TriangleSurfaceID.GetCount(); ++faceIdx)
  {
    const ezUInt16 materialID = meshDesc.m_TriangleSurfaceID[faceIdx];

    if (materialID == 0xFFFF)
      continue;

    auto& meshPartVertices = parts[materialID];

    for (ezUInt8 v = 0; v < 3; ++v)
    {
      const ezUInt32 vtxIdx = meshDesc.m_TriangleIndices[uiVertexIdx++];

      meshPartVertices.PushBack(meshDesc.m_Vertices[vtxIdx]);
    }
  }

  ezUInt16 uiNumParts = parts.GetCount();
  inout_stream << uiNumParts;

  for (auto it : parts)
  {
    const auto& meshPartVertices = it.Value();
    ezTempHybridArray<ezVec3, 256> hullVertices;
    EZ_SUCCEED_OR_RETURN(ComputeConvexHull(meshPartVertices, hullVertices));

    EZ_SUCCEED_OR_RETURN(CookSingleConvexJoltMesh(hullVertices, inout_stream));
  }

  return EZ_SUCCESS;
}
