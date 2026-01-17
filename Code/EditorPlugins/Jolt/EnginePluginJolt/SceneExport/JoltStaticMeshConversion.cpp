#include <EnginePluginJolt/EnginePluginJoltPCH.h>

#include <EnginePluginJolt/SceneExport/JoltStaticMeshConversion.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <JoltPlugin/Actors/JoltStaticActorComponent.h>
#include <JoltPlugin/Resources/JoltMeshResourceWriter.h>

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

  ezJoltMeshDesc meshDesc;
  meshDesc.m_Type = ezJoltMeshDesc::Type::Triangle;
  meshDesc.m_Vertices.SetCountUninitialized(uiNumVertices);

  for (ezUInt32 i = 0; i < uiNumVertices; ++i)
  {
    meshDesc.m_Vertices[i] = desc.m_Vertices[i];
  }

  meshDesc.m_TriangleIndices.SetCountUninitialized(uiNumTriangles * 3);
  meshDesc.m_TriangleSurfaceID.SetCount(uiNumTriangles);

  for (ezUInt32 i = 0; i < uiNumTriangles; ++i)
  {
    meshDesc.m_TriangleIndices[i * 3 + 0] = desc.m_Triangles[i].m_uiVertexIndices[0];
    meshDesc.m_TriangleIndices[i * 3 + 1] = desc.m_Triangles[i].m_uiVertexIndices[1];
    meshDesc.m_TriangleIndices[i * 3 + 2] = desc.m_Triangles[i].m_uiVertexIndices[2];
  }

  // copy materials
  // we could collate identical materials here and merge meshes, but the mesh cooking will probably do the same already
  {
    for (ezUInt32 i = 0; i < desc.m_Surfaces.GetCount(); ++i)
    {
      meshDesc.m_Surfaces.PushBack(desc.m_Surfaces[i]);
    }

    for (ezUInt32 i = 0; i < uiNumSubMeshes; ++i)
    {
      const ezUInt32 uiLastTriangle = desc.m_SubMeshes[i].m_uiFirstTriangle + desc.m_SubMeshes[i].m_uiNumTriangles;
      const ezUInt16 uiSurface = desc.m_SubMeshes[i].m_uiSurfaceIndex;

      for (ezUInt32 t = desc.m_SubMeshes[i].m_uiFirstTriangle; t < uiLastTriangle; ++t)
      {
        meshDesc.m_TriangleSurfaceID[t] = uiSurface;
      }
    }
  }

  ezStringBuilder sDocGuid, sOutputFile;
  ezConversionUtils::ToString(documentGuid, sDocGuid);

  sOutputFile.SetFormat(":project/AssetCache/Generated/{0}.ezJoltMesh", sDocGuid);

  ezDeferredFileWriter file;
  file.SetOutput(sOutputFile);

  if (ezJoltMeshResourceWriter::WriteMeshResource(std::move(meshDesc), file).Failed())
  {
    ezLog::Error("Could not write to global collision mesh file");
    return;
  }

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
