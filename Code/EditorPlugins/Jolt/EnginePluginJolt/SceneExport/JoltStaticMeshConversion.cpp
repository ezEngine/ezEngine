#include <EnginePluginJolt/EnginePluginJoltPCH.h>

#include <EnginePluginJolt/SceneExport/JoltStaticMeshConversion.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <JoltCooking/JoltCooking.h>
#include <JoltPlugin/Actors/JoltStaticActorComponent.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneExportModifier_JoltStaticMeshConversion, 1, ezRTTIDefaultAllocator<ezSceneExportModifier_JoltStaticMeshConversion>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

void ezSceneExportModifier_JoltStaticMeshConversion::ModifyWorld(ezWorld& ref_world, ezStringView sDocumentType, const ezUuid& documentGuid, bool bForExport)
{
  if (sDocumentType == "Prefab")
  {
    // the auto generated static meshes are needed in the prefab document, so that physical interactions for previewing purposes work
    // however, the scene also exports the static colmesh, including all the prefabs (with overridden materials)
    // in the final scene this would create double colmeshes in the same place, but the materials may differ
    // therefore we don't want to export the colmesh other than for preview purposes, so we ignore this, if 'bForExport' is true

    if (bForExport)
    {
      return;
    }
  }

  EZ_LOCK(ref_world.GetWriteMarker());

  ezSmcDescription desc;
  desc.m_Surfaces.PushBack(); // add a dummy empty material

  ezMsgBuildStaticMesh msg;
  msg.m_pStaticMeshDescription = &desc;

  for (auto it = ref_world.GetObjects(); it.IsValid(); ++it)
  {
    if (!it->IsStatic())
      continue;

    it->SendMessage(msg);
  }

  if (desc.m_SubMeshes.IsEmpty() || desc.m_Vertices.IsEmpty() || desc.m_Triangles.IsEmpty())
    return;

  const ezUInt32 uiNumVertices = desc.m_Vertices.GetCount();
  const ezUInt32 uiNumTriangles = desc.m_Triangles.GetCount();
  const ezUInt32 uiNumSubMeshes = desc.m_SubMeshes.GetCount();

  ezJoltCookingMesh xMesh;
  xMesh.m_Vertices.SetCountUninitialized(uiNumVertices);

  for (ezUInt32 i = 0; i < uiNumVertices; ++i)
  {
    xMesh.m_Vertices[i] = desc.m_Vertices[i];
  }

  xMesh.m_PolygonIndices.SetCountUninitialized(uiNumTriangles * 3);
  xMesh.m_VerticesInPolygon.SetCountUninitialized(uiNumTriangles);
  xMesh.m_PolygonSurfaceID.SetCount(uiNumTriangles);

  for (ezUInt32 i = 0; i < uiNumTriangles; ++i)
  {
    xMesh.m_VerticesInPolygon[i] = 3;

    xMesh.m_PolygonIndices[i * 3 + 0] = desc.m_Triangles[i].m_uiVertexIndices[0];
    xMesh.m_PolygonIndices[i * 3 + 1] = desc.m_Triangles[i].m_uiVertexIndices[1];
    xMesh.m_PolygonIndices[i * 3 + 2] = desc.m_Triangles[i].m_uiVertexIndices[2];
  }

  ezHybridArray<ezString, 32> surfaces;

  // copy materials
  // we could collate identical materials here and merge meshes, but the mesh cooking will probably do the same already
  {
    for (ezUInt32 i = 0; i < desc.m_Surfaces.GetCount(); ++i)
    {
      surfaces.PushBack(desc.m_Surfaces[i]);
    }

    for (ezUInt32 i = 0; i < uiNumSubMeshes; ++i)
    {
      const ezUInt32 uiLastTriangle = desc.m_SubMeshes[i].m_uiFirstTriangle + desc.m_SubMeshes[i].m_uiNumTriangles;
      const ezUInt16 uiSurface = desc.m_SubMeshes[i].m_uiSurfaceIndex;

      for (ezUInt32 t = desc.m_SubMeshes[i].m_uiFirstTriangle; t < uiLastTriangle; ++t)
      {
        xMesh.m_PolygonSurfaceID[t] = uiSurface;
      }
    }
  }

  ezStringBuilder sDocGuid, sOutputFile;
  ezConversionUtils::ToString(documentGuid, sDocGuid);

  sOutputFile.SetFormat(":project/AssetCache/Generated/{0}.ezJoltMesh", sDocGuid);

  ezDeferredFileWriter file;
  file.SetOutput(sOutputFile);

  ezAssetFileHeader header;
  header.SetFileHashAndVersion(0, 9); // ezGetStaticRTTI<ezJoltCollisionMeshAssetDocument>()->GetTypeVersion();
  header.Write(file).IgnoreResult();

  const ezUInt8 uiVersion = 3;
  ezUInt8 uiCompressionMode = 0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  uiCompressionMode = 1;
  ezCompressedStreamWriterZstd compressor(&file, 0, ezCompressedStreamWriterZstd::Compression::Average);
  ezChunkStreamWriter chunk(compressor);
#else
  ezChunkStreamWriter chunk(file);
#endif

  file << uiVersion;
  file << uiCompressionMode;

  chunk.BeginStream(1);

  if (ezJoltCooking::WriteResourceToStream(chunk, xMesh, surfaces, ezJoltCooking::MeshType::Triangle).LogFailure())
  {
    ezLog::Error("Could not write to global collision mesh file");
    return;
  }

  chunk.EndStream();

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  if (compressor.FinishCompressedStream().Failed())
  {
    ezLog::Error("Failed to finish compressing stream.");
    return;
  }

  ezLog::Dev("Compressed collision mesh data from {0} KB to {1} KB ({2}%%)", ezArgF((float)compressor.GetUncompressedSize() / 1024.0f, 1), ezArgF((float)compressor.GetCompressedSize() / 1024.0f, 1), ezArgF(100.0f * compressor.GetCompressedSize() / compressor.GetUncompressedSize(), 1));

#endif

  if (file.Close().Failed())
  {
    ezLog::Error("Could not write to global collision mesh file");
    return;
  }

  {
    ezGameObject* pGo;
    ezGameObjectDesc god;
    god.m_sName.Assign("Greybox Collision Mesh");
    ref_world.CreateObject(god, pGo);

    auto* pCompMan = ref_world.GetOrCreateComponentManager<ezJoltStaticActorComponentManager>();

    ezJoltStaticActorComponent* pComp;
    pCompMan->CreateComponent(pGo, pComp);

    if (!sOutputFile.IsEmpty())
    {
      ezJoltMeshResourceHandle hMesh = ezResourceManager::LoadResource<ezJoltMeshResource>(sOutputFile);
      pComp->SetMesh(hMesh);
    }
  }
}
