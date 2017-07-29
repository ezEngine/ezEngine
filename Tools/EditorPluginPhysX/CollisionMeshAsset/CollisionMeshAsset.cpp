#include <PCH.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAsset.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAssetObjects.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAssetManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <Core/Graphics/Geometry.h>
#include <PhysXCooking/PhysXCooking.h>
#include <Foundation/IO/ChunkStream.h>
#include <ModelImporter/Material.h>
#include <ModelImporter/Mesh.h>
#include <ModelImporter/ModelImporter.h>
#include <ModelImporter/VertexData.h>
#include <Foundation/Time/Stopwatch.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCollisionMeshAssetDocument, 3, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

static ezVec3 GetBasisVector(ezBasisAxis::Enum basisAxis)
{
  switch (basisAxis)
  {
  case ezBasisAxis::PositiveX:
    return ezVec3(1.0f, 0.0f, 0.0f);

  case ezBasisAxis::NegativeX:
    return ezVec3(-1.0f, 0.0f, 0.0f);

  case ezBasisAxis::PositiveY:
    return ezVec3(0.0f, 1.0f, 0.0f);

  case ezBasisAxis::NegativeY:
    return ezVec3(0.0f, -1.0f, 0.0f);

  case ezBasisAxis::PositiveZ:
    return ezVec3(0.0f, 0.0f, 1.0f);

  case ezBasisAxis::NegativeZ:
    return ezVec3(0.0f, 0.0f, -1.0f);

  default:
    EZ_REPORT_FAILURE("Invalid basis dir {0}", basisAxis);
    return ezVec3::ZeroVector();
  }
}

static ezMat3 CalculateTransformationMatrix(const ezCollisionMeshAssetProperties* pProp)
{
  const float us = ezMath::Clamp(pProp->m_fUniformScaling, 0.0001f, 10000.0f);
  const float sx = ezMath::Clamp(pProp->m_vNonUniformScaling.x, 0.0001f, 10000.0f);
  const float sy = ezMath::Clamp(pProp->m_vNonUniformScaling.y, 0.0001f, 10000.0f);
  const float sz = ezMath::Clamp(pProp->m_vNonUniformScaling.z, 0.0001f, 10000.0f);

  ezMat3 mResult;
  mResult.SetColumn(0, GetBasisVector(pProp->m_ForwardDir) * us * sx);
  mResult.SetColumn(1, GetBasisVector(pProp->m_RightDir) * us * sy);
  mResult.SetColumn(2, GetBasisVector(pProp->m_UpDir) * us * sz);

  return mResult.GetTranspose();
}

ezCollisionMeshAssetDocument::ezCollisionMeshAssetDocument(const char* szDocumentPath)
  : ezSimpleAssetDocument<ezCollisionMeshAssetProperties>(szDocumentPath, true)
{
}

const char* ezCollisionMeshAssetDocument::QueryAssetType() const
{
  if (GetProperties()->m_MeshType == ezCollisionMeshType::TriangleMesh)
    return "Collision Mesh";

  return "Collision Mesh (Convex)";
}


//////////////////////////////////////////////////////////////////////////


ezStatus ezCollisionMeshAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  ezCollisionMeshAssetProperties* pProp = GetProperties();

  const ezMat3 mTransformation = CalculateTransformationMatrix(pProp);
  const bool bFlipTriangles = (mTransformation.GetColumn(0).Cross(mTransformation.GetColumn(1)).Dot(mTransformation.GetColumn(2)) < 0.0f);

  ezChunkStreamWriter chunk(stream);

  chunk.BeginStream();

  {
    ezPhysXCookingMesh xMesh;

    // TODO verify
    xMesh.m_bFlipNormals = (mTransformation.GetColumn(0).Cross(mTransformation.GetColumn(1)).Dot(mTransformation.GetColumn(2)) < 0.0f);

    if (pProp->m_MeshType == ezCollisionMeshType::ConvexHull || pProp->m_MeshType == ezCollisionMeshType::TriangleMesh)
    {
      EZ_SUCCEED_OR_RETURN(CreateMeshFromFile(mTransformation, xMesh));
    }
    else
    {
      ezGeometry geom;
      const ezMat4 mTrans(mTransformation, ezVec3::ZeroVector());

      if (pProp->m_MeshType == ezCollisionMeshType::Cylinder)
      {
        geom.AddCylinderOnePiece(pProp->m_fRadius, pProp->m_fRadius2, pProp->m_fHeight, ezMath::Clamp<ezUInt16>(pProp->m_uiDetail, 3, 32), ezColor::White, mTrans);
      }

      EZ_SUCCEED_OR_RETURN(CreateMeshFromGeom(geom, xMesh));
    }

    EZ_SUCCEED_OR_RETURN(WriteToStream(chunk, xMesh));
  }

  chunk.EndStream();

  return ezStatus(EZ_SUCCESS);
}

static ezStatus ImportMesh(const char* filename, const char* subMeshFilename, ezSharedPtr<ezModelImporter::Scene>& outScene, ezModelImporter::Mesh*& outMesh, ezString& outMeshFileAbs)
{
  ezStopwatch timer;

  outMeshFileAbs = filename;
  if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(outMeshFileAbs))
  {
    return ezStatus(ezFmt("Could not make path absolute: '{0};", outMeshFileAbs));
  }

  EZ_SUCCEED_OR_RETURN(ezModelImporter::Importer::GetSingleton()->ImportMesh(outMeshFileAbs, subMeshFilename, outScene, outMesh));

  ezLog::Debug("Mesh Import time: {0}s", ezArgF(timer.GetRunningTotal().GetSeconds(), 2));

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezCollisionMeshAssetDocument::CreateMeshFromFile(const ezMat3 &mTransformation, ezPhysXCookingMesh& outMesh)
{
  using namespace ezModelImporter;

  ezCollisionMeshAssetProperties* pProp = GetProperties();

  ezSharedPtr<Scene> scene;
  Mesh* mesh = nullptr;
  ezString sMeshFileAbs;
  EZ_SUCCEED_OR_RETURN(ImportMesh(pProp->m_sMeshFile, pProp->m_sSubMeshName, scene, mesh, sMeshFileAbs));

  const ezModelImporter::TypedVertexDataStreamView<ezVec3> positionStream(*mesh->GetDataStream(ezGALVertexAttributeSemantic::Position));

  const ezArrayPtr<Mesh::Triangle> triangles = mesh->GetTriangles();

  outMesh.m_PolygonSurfaceID.SetCountUninitialized(triangles.GetCount());
  outMesh.m_VerticesInPolygon.SetCountUninitialized(triangles.GetCount());
  for (ezUInt32 uiTriangle = 0; uiTriangle < triangles.GetCount(); ++uiTriangle)
  {
    outMesh.m_PolygonSurfaceID[uiTriangle] = 0; // default value, will be updated below when extracting materials.
    outMesh.m_VerticesInPolygon[uiTriangle] = 3;  // Triangles!
  }

  // Extract vertices and indices
  {
    const ezGALVertexAttributeSemantic::Enum streamSemantics[] = { ezGALVertexAttributeSemantic::Position };
    ezHashTable<Mesh::DataIndexBundle<1>, ezUInt32> dataIndicesToVertexIndices;
    mesh->GenerateInterleavedVertexMapping(streamSemantics, dataIndicesToVertexIndices, outMesh.m_PolygonIndices);

    // outMesh.m_PolygonIndices is now ready and we have a mapping from mesh data index to vertex index. Remains only to copy over the vertices to their correct places.

    outMesh.m_Vertices.SetCount(dataIndicesToVertexIndices.GetCount());
    for(auto it = dataIndicesToVertexIndices.GetIterator(); it.IsValid(); ++it)
    {
      ezVec3 vPosition = positionStream.GetValue(it.Key()[0]);
      outMesh.m_Vertices[it.Value()] = mTransformation.TransformDirection(vPosition);
    }
  }

  // Extract Material Information
  {
    ezStringBuilder sMatName;

    pProp->m_Slots.SetCount(mesh->GetNumSubMeshes());

    for (ezUInt32 subMeshIdx = 0; subMeshIdx < mesh->GetNumSubMeshes(); ++subMeshIdx)
    {
      const ezModelImporter::SubMesh& subMesh = mesh->GetSubMesh(subMeshIdx);
      const ezModelImporter::Material* material = scene->GetMaterial(subMesh.m_Material);

      // update the triangle material information
      for (ezUInt32 tri = 0; tri < subMesh.m_uiTriangleCount; ++tri)
      {
        outMesh.m_PolygonSurfaceID[subMesh.m_uiFirstTriangle + tri] = subMeshIdx;
      }

      pProp->m_Slots[subMeshIdx].m_sLabel = material->m_Name;
    }

    ApplyNativePropertyChangesToObjectManager();
    pProp = GetProperties();
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezCollisionMeshAssetDocument::CreateMeshFromGeom(ezGeometry& geom, ezPhysXCookingMesh& outMesh)
{
  ezCollisionMeshAssetProperties* pProp = GetProperties();

  // Material setup.
  {
    // Ensure there is just one slot.
    if (pProp->m_Slots.GetCount() != 1)
    {
      GetObjectAccessor()->StartTransaction("Update Mesh Material Info");

      pProp->m_Slots.SetCount(1);
      pProp->m_Slots[0].m_sLabel = "Default";

      ApplyNativePropertyChangesToObjectManager();
      GetObjectAccessor()->FinishTransaction();

      // Need to reacquire pProp pointer since it might be reallocated.
      pProp = GetProperties();
    }
  }

  // copy vertex positions
  {
    outMesh.m_Vertices.SetCountUninitialized(geom.GetVertices().GetCount());
    for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
    {
      outMesh.m_Vertices[v] = geom.GetVertices()[v].m_vPosition;
    }
  }

  // Copy Polygon Data
  {
    outMesh.m_PolygonSurfaceID.SetCountUninitialized(geom.GetPolygons().GetCount());
    outMesh.m_VerticesInPolygon.SetCountUninitialized(geom.GetPolygons().GetCount());
    outMesh.m_PolygonIndices.Reserve(geom.GetPolygons().GetCount() * 4);

    for (ezUInt32 p = 0; p < geom.GetPolygons().GetCount(); ++p)
    {
      const auto& poly = geom.GetPolygons()[p];
      outMesh.m_VerticesInPolygon[p] = poly.m_Vertices.GetCount();
      outMesh.m_PolygonSurfaceID[p] = 0;

      for (ezUInt32 posIdx : poly.m_Vertices)
      {
        outMesh.m_PolygonIndices.PushBack(posIdx);
      }
    }
  }

  return ezStatus(EZ_SUCCESS);
}

//ezStatus ezMeshAssetDocument::InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader)
//{
//  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(AssetHeader);
//  return status;
//}

ezStatus ezCollisionMeshAssetDocument::WriteToStream(ezChunkStreamWriter& stream, const ezPhysXCookingMesh& mesh)
{
  const ezCollisionMeshAssetProperties* pProp = GetProperties();

  ezResult resCooking = EZ_FAILURE;

  {
    stream.BeginChunk("Surfaces", 1);

    stream << pProp->m_Slots.GetCount();

    for (const auto& slot : pProp->m_Slots)
    {
      stream << slot.m_sResource;
    }

    stream.EndChunk();
  }

  if (pProp->m_MeshType == ezCollisionMeshType::TriangleMesh)
  {
    stream.BeginChunk("TriangleMesh", 1);

    ezStopwatch timer;
    resCooking = ezPhysXCooking::CookTriangleMesh(mesh, stream);
    ezLog::Dev("Triangle Mesh Cooking time: {0}s", ezArgF(timer.GetRunningTotal().GetSeconds(), 2));

    stream.EndChunk();
  }
  else
  {
    stream.BeginChunk("ConvexMesh", 1);

    ezStopwatch timer;
    resCooking = ezPhysXCooking::CookConvexMesh(mesh, stream);
    ezLog::Dev("Convex Mesh Cooking time: {0}s", ezArgF(timer.GetRunningTotal().GetSeconds(), 2));

    stream.EndChunk();
  }

  if (resCooking.Failed())
    return ezStatus("Cooking the collision mesh failed.");


  return ezStatus(EZ_SUCCESS);
}

ezStatus ezCollisionMeshAssetDocument::InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(AssetHeader);
  return status;
}
