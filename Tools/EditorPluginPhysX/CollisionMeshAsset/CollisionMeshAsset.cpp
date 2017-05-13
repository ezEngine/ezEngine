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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCollisionMeshAssetDocument, 2, ezRTTINoAllocator);
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
  : ezSimpleAssetDocument<ezCollisionMeshAssetProperties>(szDocumentPath)
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
        /// \todo Merged vertices
        geom.AddCylinder(pProp->m_fRadius, pProp->m_fRadius2, pProp->m_fHeight, true, true, ezMath::Max<ezUInt16>(3, pProp->m_uiDetail), ezColor::White, mTrans);
      }

      //CreateMeshFromGeom(pProp, geom, desc);
      EZ_ASSERT_NOT_IMPLEMENTED;

    }

    pProp = GetProperties();
    EZ_SUCCEED_OR_RETURN(WriteToStream(chunk, pProp, xMesh));
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
    return ezStatus(ezFmt("Could not make path absolute: '{0};", outMeshFileAbs.GetData()));
  }

  EZ_SUCCEED_OR_RETURN(ezModelImporter::Importer::GetSingleton()->ImportMesh(outMeshFileAbs, subMeshFilename, outScene, outMesh));

  ezLog::Debug("Mesh Import time: {0}s", ezArgF(timer.GetRunningTotal().GetSeconds(), 2));

  return ezStatus(EZ_SUCCESS);
}

template<>
struct ezHashHelper<ezModelImporter::VertexDataIndex>
{
  typedef ezModelImporter::VertexDataIndex ValueType;

  static ezUInt32 Hash(const ValueType& value)
  {
    return ezHashing::MurmurHash(&value, sizeof(ValueType));
  }

  static bool Equal(const ValueType& a, const ValueType& b)
  {
    return a == b;
  }
};

ezStatus ezCollisionMeshAssetDocument::CreateMeshFromFile(const ezMat3 &mTransformation, ezPhysXCookingMesh& outMesh)
{
  using namespace ezModelImporter;

  ezCollisionMeshAssetProperties* pProp = GetProperties();

  const bool bFlipTriangles = (mTransformation.GetColumn(0).Cross(mTransformation.GetColumn(1)).Dot(mTransformation.GetColumn(2)) < 0.0f);
  outMesh.m_bFlipNormals = bFlipTriangles; // TODO verify

  ezSharedPtr<Scene> scene;
  Mesh* mesh = nullptr;
  ezString sMeshFileAbs;
  EZ_SUCCEED_OR_RETURN(ImportMesh(pProp->m_sMeshFile, pProp->m_sSubMeshName, scene, mesh, sMeshFileAbs));

  const ezModelImporter::TypedVertexDataStreamView<ezVec3> positionStream(*mesh->GetDataStream(ezGALVertexAttributeSemantic::Position));
  auto pStream = mesh->GetDataStream(ezGALVertexAttributeSemantic::Position);

  const ezArrayPtr<Mesh::Triangle> triangles = mesh->GetTriangles();

  outMesh.m_PolygonSurfaceID.SetCountUninitialized(triangles.GetCount());
  outMesh.m_VerticesInPolygon.SetCountUninitialized(triangles.GetCount());
  outMesh.m_PolygonIndices.Reserve(triangles.GetCount() * 3);
  outMesh.m_Vertices.Reserve(triangles.GetCount() * 3);

  ezHashTable<VertexDataIndex, ezUInt32> vertexIndices;

  for (ezUInt32 uiTriangle = 0; uiTriangle < triangles.GetCount(); ++uiTriangle)
  {
    const Mesh::Triangle& triangle = triangles[uiTriangle];

    outMesh.m_PolygonSurfaceID[uiTriangle] = 0; // default value, will be updated below
    outMesh.m_VerticesInPolygon[uiTriangle] = 3;

    for (ezUInt32 v = 0; v < 3; ++v)
    {
      auto dataIdx = pStream->GetDataIndex(triangle.m_Vertices[v]);

      ezUInt32 uiVertexIndex = outMesh.m_Vertices.GetCount(); // next index value, if not a known vertex yet
      if (!vertexIndices.TryGetValue(dataIdx, uiVertexIndex))
      {
        // vertex wasn't used before, add it to the hashmap and the vertex data array
        vertexIndices.Insert(dataIdx, uiVertexIndex);

        ezVec3 vPosition = positionStream.GetValue(dataIdx);

        outMesh.m_Vertices.PushBack(mTransformation.TransformDirection(vPosition));
      }

      outMesh.m_PolygonIndices.PushBack(uiVertexIndex);
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

/*
void ezMeshAssetDocument::CreateMeshFromGeom(ezMeshAssetProperties* pProp, ezGeometry& geom, ezMeshResourceDescriptor& desc)
{
//// Material setup.
//{
//  // Ensure there is just one slot.
//  if (pProp->m_Slots.GetCount() != 1)
//  {
//    GetObjectAccessor()->StartTransaction("Update Mesh Material Info");

//    pProp->m_Slots.SetCount(1);
//    pProp->m_Slots[0].m_sLabel = "Default";

//    ApplyNativePropertyChangesToObjectManager();
//    GetObjectAccessor()->FinishTransaction();

//    // Need to reacquire pProp pointer since it might be reallocated.
//    pProp = GetProperties();
//  }

//  // Set material for mesh.
//  if (!pProp->m_Slots.IsEmpty())
//    desc.SetMaterial(0, pProp->m_Slots[0].m_sResource);
//  else
//    desc.SetMaterial(0, "");
//}

desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
desc.MeshBufferDesc().AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

desc.AddSubMesh(desc.MeshBufferDesc().GetPrimitiveCount(), 0, 0);
}
*/

//ezStatus ezMeshAssetDocument::InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader)
//{
//  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(AssetHeader);
//  return status;
//}

ezStatus ezCollisionMeshAssetDocument::WriteToStream(ezChunkStreamWriter& stream, const ezCollisionMeshAssetProperties* pProp, const ezPhysXCookingMesh& mesh)
{
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