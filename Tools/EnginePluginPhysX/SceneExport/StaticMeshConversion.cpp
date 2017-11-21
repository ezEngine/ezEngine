#include <PCH.h>
#include <EnginePluginPhysX/SceneExport/StaticMeshConversion.h>
#include <Core/World/World.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <PhysXCooking/PhysXCooking.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <PhysXPlugin/Components/PxStaticActorComponent.h>
#include <Core/Assets/AssetFileHeader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneExportModifier_StaticMeshConversion, 1, ezRTTIDefaultAllocator<ezSceneExportModifier_StaticMeshConversion>)
EZ_END_DYNAMIC_REFLECTED_TYPE

void ezSceneExportModifier_StaticMeshConversion::ModifyWorld(ezWorld& world, const ezUuid& documentGuid)
{
  EZ_LOCK(world.GetWriteMarker());

  ezTag tagColMesh = ezTagRegistry::GetGlobalRegistry().RegisterTag("AutoColMesh");

  ezSmcDescription desc;
  desc.m_Surfaces.PushBack(); // add a dummy empty material

  ezBuildStaticMeshMessage msg;
  msg.m_pStaticMeshDescription = &desc;

  for (auto it = world.GetObjects(); it.IsValid(); ++it)
  {
    if (!it->GetTags().IsSet(tagColMesh))
      continue;

    it->SendMessage(msg);
  }

  if (desc.m_SubMeshes.IsEmpty() || desc.m_Vertices.IsEmpty() || desc.m_Triangles.IsEmpty())
    return;

  const ezUInt32 uiNumVertices = desc.m_Vertices.GetCount();
  const ezUInt32 uiNumTriangles = desc.m_Triangles.GetCount();
  const ezUInt32 uiNumSubMeshes = desc.m_SubMeshes.GetCount();

  ezPhysXCookingMesh xMesh;
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

  sOutputFile.Format(":project/AssetCache/Generated/{0}.ezPhysXMesh", sDocGuid);
  
  ezDeferredFileWriter file;
  file.SetOutput(sOutputFile);

  ezAssetFileHeader header;
  header.Write(file);

  ezChunkStreamWriter chunk(file);
  chunk.BeginStream();

  ezPhysXCooking::WriteResourceToStream(chunk, xMesh, surfaces, false);

  chunk.EndStream();

  if (file.Close().Failed())
  {
    ezLog::Error("Could not write to global collision mesh file");
    return;
  }

  {
    ezGameObject* pGo;
    ezGameObjectDesc god;
    god.m_sName.Assign("Global Colmesh");
    world.CreateObject(god, pGo);

    auto* pCompMan = world.GetOrCreateComponentManager<ezPxStaticActorComponentManager>();

    ezPxStaticActorComponent* pComp;
    pCompMan->CreateComponent(pGo, pComp);

    pComp->SetMeshFile(sOutputFile);
  }
}
