#pragma once

#include <EnginePluginJolt/EnginePluginJoltDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <JoltCooking/JoltCooking.h>
#include <JoltPlugin/Resources/JoltMeshResourceWriter.h>

class ezJoltCollisionMeshWriter : public ezJoltMeshResourceWriterInterface
{
  EZ_DECLARE_SINGLETON_OF_INTERFACE(ezJoltCollisionMeshWriter, ezJoltMeshResourceWriterInterface);

public:
  ezJoltCollisionMeshWriter();
  ~ezJoltCollisionMeshWriter();

  static ezJoltCooking::MeshType ConvertMeshType(ezJoltMeshDesc::Type meshType)
  {
    switch (meshType)
    {
      case ezJoltMeshDesc::Type::Triangle:
        return ezJoltCooking::MeshType::Triangle;

      case ezJoltMeshDesc::Type::ConvexHull:
        return ezJoltCooking::MeshType::ConvexHull;

      case ezJoltMeshDesc::Type::ConvexDecomposition:
        return ezJoltCooking::MeshType::ConvexDecomposition;

      case ezJoltMeshDesc::Type::ConvexHullGroup:
        return ezJoltCooking::MeshType::ConvexHullGroup;

        EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    return ezJoltCooking::MeshType::Triangle;
  }

  // all defined in header so it can be used in the editor plugin without linking against the engine plugin
  static ezResult WriteMeshResource(ezJoltMeshDesc&& meshDesc, ezStreamWriter& inout_stream, ezUInt64 uiAssetHash = 0)
  {
    if (meshDesc.m_bWriteAssetHeader)
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

    ezJoltCookingMesh xMesh;
    xMesh.m_Vertices = std::move(meshDesc.m_Vertices);
    xMesh.m_TriangleIndices = std::move(meshDesc.m_TriangleIndices);
    xMesh.m_TriangleSurfaceID = std::move(meshDesc.m_TriangleSurfaceID);

    if (ezJoltCooking::WriteResourceToStream(chunk, xMesh, meshDesc.m_Surfaces, ConvertMeshType(meshDesc.m_Type), meshDesc.m_uiMaxConvexPieces).LogFailure())
    {
      return EZ_FAILURE;
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

  virtual ezResult WriteMeshResourceInternal(ezJoltMeshDesc&& meshDesc, ezStreamWriter& inout_stream, ezUInt64 uiAssetHash) override
  {
    return WriteMeshResource(std::move(meshDesc), inout_stream, uiAssetHash);
  }
};
