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

void ezSceneExportModifier_StaticMeshConversion::ModifyWorld(ezWorld& world)
{
  EZ_LOCK(world.GetWriteMarker());

  ezTag tagColMesh = ezTagRegistry::GetGlobalRegistry().RegisterTag("AutoColMesh");

  ezSmcDescription desc;
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

  ezPhysXCookingMesh xMesh;
  xMesh.m_Vertices.SetCountUninitialized(uiNumVertices);

  for (ezUInt32 i = 0; i < uiNumVertices; ++i)
  {
    xMesh.m_Vertices[i] = desc.m_Vertices[i];
  }

  xMesh.m_PolygonIndices.SetCountUninitialized(uiNumTriangles * 3);
  xMesh.m_VerticesInPolygon.SetCountUninitialized(uiNumTriangles);
  xMesh.m_PolygonSurfaceID.SetCountUninitialized(uiNumTriangles);

  for (ezUInt32 i = 0; i < uiNumTriangles; ++i)
  {
    xMesh.m_PolygonSurfaceID[i] = 0; // TODO material
    xMesh.m_VerticesInPolygon[i] = 3;

    xMesh.m_PolygonIndices[i * 3 + 0] = desc.m_Triangles[i].m_uiVertexIndices[0];
    xMesh.m_PolygonIndices[i * 3 + 1] = desc.m_Triangles[i].m_uiVertexIndices[1];
    xMesh.m_PolygonIndices[i * 3 + 2] = desc.m_Triangles[i].m_uiVertexIndices[2];
  }

  // TODO: hacky file path
  ezDeferredFileWriter file;
  file.SetOutput(":project/GlobalColMesh.ezPhysXMesh");

  ezAssetFileHeader header;
  header.Write(file);

  ezChunkStreamWriter chunk(file);
  chunk.BeginStream();

  ezHybridArray<ezString, 32> surfaces;

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

    pComp->SetMeshFile("GlobalColMesh.ezPhysXMesh");
  }
}
