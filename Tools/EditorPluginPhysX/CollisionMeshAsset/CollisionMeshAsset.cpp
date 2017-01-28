#include <PCH.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAsset.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAssetObjects.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAssetManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <../ThirdParty/AssImp/include/scene.h>
#include <../ThirdParty/AssImp/include/Importer.hpp>
#include <../ThirdParty/AssImp/include/postprocess.h>
#include <../ThirdParty/AssImp/include/Logger.hpp>
#include <../ThirdParty/AssImp/include/LogStream.hpp>
#include <../ThirdParty/AssImp/include/DefaultLogger.hpp>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <Core/Graphics/Geometry.h>
#include <PhysXCooking/PhysXCooking.h>
#include <Foundation/IO/ChunkStream.h>

using namespace Assimp;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCollisionMeshAssetDocument, 1, ezRTTINoAllocator);
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
  ezMat3 mResult;
  mResult.SetColumn(0, GetBasisVector(pProp->m_ForwardDir) * pProp->m_fUniformScaling * pProp->m_vNonUniformScaling.x);
  mResult.SetColumn(1, GetBasisVector(pProp->m_RightDir) * pProp->m_fUniformScaling * pProp->m_vNonUniformScaling.y);
  mResult.SetColumn(2, GetBasisVector(pProp->m_UpDir) * pProp->m_fUniformScaling * pProp->m_vNonUniformScaling.z);

  return mResult.GetTranspose();
}

class aiLogStream : public LogStream
{
public:
  void write(const char* message)
  {
    ezLog::Dev("AssImp: {0}", message);
  }
};

ezCollisionMeshAssetDocument::ezCollisionMeshAssetDocument(const char* szDocumentPath) : ezSimpleAssetDocument<ezCollisionMeshAssetProperties>(szDocumentPath)
{
}


const char* ezCollisionMeshAssetDocument::QueryAssetType() const
{
  if (GetProperties()->m_MeshType == ezCollisionMeshType::ConvexHull)
    return "Collision Mesh (Convex)";

  return "Collision Mesh";
}

ezStatus ezCollisionMeshAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  ezCollisionMeshAssetProperties* pProp = GetProperties();

  const ezMat3 mTransformation = CalculateTransformationMatrix(pProp);
  const bool bFlipTriangles = (mTransformation.GetColumn(0).Cross(mTransformation.GetColumn(1)).Dot(mTransformation.GetColumn(2)) < 0.0f);

  ezChunkStreamWriter chunk(stream);

  chunk.BeginStream();

  {
    auto ret = CreateMeshFromFile(pProp, bFlipTriangles, mTransformation, chunk);

    if (ret.m_Result.Failed())
      return ret;
  }

  chunk.EndStream();

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezCollisionMeshAssetDocument::CreateMeshFromFile(ezCollisionMeshAssetProperties* pProp, bool bFlipTriangles, const ezMat3 &mTransformation, ezChunkStreamWriter& stream)
{
  ezPhysXCooking::Mesh xMesh;
  xMesh.m_bFlipNormals = false;// bFlipTriangles;

  ezString sMeshFileAbs = pProp->m_sMeshFile;
  if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sMeshFileAbs))
  {
    ezLog::Error("Collision Mesh Asset Transform failed: Input Path '{0}' is not in any data directory", sMeshFileAbs.GetData());
    return ezStatus(ezFmt("Could not make path absolute: '{0};", sMeshFileAbs.GetData()));
  }

  Importer importer;

  DefaultLogger::create("", Logger::NORMAL);

  const unsigned int severity = Logger::Debugging | Logger::Info | Logger::Err | Logger::Warn;
  DefaultLogger::get()->attachStream(new aiLogStream(), severity);

  const aiScene* scene = nullptr;

  {
    EZ_LOG_BLOCK("Importing Mesh", sMeshFileAbs.GetData());

    scene = importer.ReadFile(sMeshFileAbs.GetData(), aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_PreTransformVertices);

    if (!scene)
    {
      ezLog::Error("Could not import file '{0}'", sMeshFileAbs.GetData());
      return ezStatus("Collision Mesh Asset input file could not be imported");
    }
  }

  ezLog::Success("Mesh has been imported: '{0}'", sMeshFileAbs);

  ezLog::Info("Number of unique Meshes: {0}", scene->mNumMeshes);

  ezUInt32 uiVertices = 0;
  ezUInt32 uiTriangles = 0;

  for (ezUInt32 meshIdx = 0; meshIdx < scene->mNumMeshes; ++meshIdx)
  {
    uiVertices += scene->mMeshes[meshIdx]->mNumVertices;
    uiTriangles += scene->mMeshes[meshIdx]->mNumFaces;
  }

  ezLog::Info("Number of Vertices: {0}", uiVertices);
  ezLog::Info("Number of Triangles: {0}", uiTriangles);


  {
    // Update materials.
    {
      aiString name;
      ezStringBuilder sMatName;

      pProp->m_Slots.SetCount(scene->mNumMeshes);
      for (ezUInt32 i = 0; i < scene->mNumMeshes; ++i)
      {
        aiMaterial* mat = scene->mMaterials[scene->mMeshes[i]->mMaterialIndex];

        mat->Get(AI_MATKEY_NAME, name);

        pProp->m_Slots[i].m_sLabel = name.C_Str();
      }
    }

    // Set changes.
    {
      ezAbstractObjectGraph graph;
      ezRttiConverterContext context;
      ezRttiConverterWriter rttiConverter(&graph, &context, true, true);

      ezDocumentObject* pPropObj = GetPropertyObject();
      context.RegisterObject(pPropObj->GetGuid(), pPropObj->GetTypeAccessor().GetType(), pProp);
      auto* pNode = rttiConverter.AddObjectToGraph(pProp, "Object");

      ezDocumentObjectConverterReader objectConverter(&graph, GetObjectManager(), ezDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
      objectConverter.ApplyPropertiesToObject(pNode, pPropObj);

      pProp = GetProperties();
    }
  }

  xMesh.m_PolygonIndices.Reserve(uiTriangles * 3);
  xMesh.m_Vertices.SetCount(uiVertices);
  xMesh.m_VerticesInPolygon.SetCount(uiTriangles);
  xMesh.m_PolygonSurfaceID.Reserve(uiTriangles);

  ezUInt32 uiCurVertex = 0;
  ezUInt32 uiCurTriangle = 0;

  aiString name;
  ezStringBuilder sMatName;

  const ezUInt32 uiMaxMaterial = ezMath::Max<ezUInt32>(0, pProp->m_Slots.GetCount());

  for (ezUInt32 meshIdx = 0; meshIdx < scene->mNumMeshes; ++meshIdx)
  {
    aiMesh* mesh = scene->mMeshes[meshIdx];
    //aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
    //mat->Get(AI_MATKEY_NAME, name);

    for (ezUInt32 faceIdx = 0; faceIdx < mesh->mNumFaces; ++faceIdx, ++uiCurTriangle)
    {
      EZ_ASSERT_DEV(mesh->mFaces[faceIdx].mNumIndices == 3, "");

      xMesh.m_VerticesInPolygon[uiCurTriangle] = 3;
      xMesh.m_PolygonIndices.PushBack(uiCurVertex + mesh->mFaces[faceIdx].mIndices[bFlipTriangles ? 2 : 0]);
      xMesh.m_PolygonIndices.PushBack(uiCurVertex + mesh->mFaces[faceIdx].mIndices[1]);
      xMesh.m_PolygonIndices.PushBack(uiCurVertex + mesh->mFaces[faceIdx].mIndices[bFlipTriangles ? 0 : 2]);

      xMesh.m_PolygonSurfaceID.PushBack(meshIdx > uiMaxMaterial ? 0 : meshIdx);
    }

    for (ezUInt32 vertexIdx = 0; vertexIdx < mesh->mNumVertices; ++vertexIdx, ++uiCurVertex)
    {
      xMesh.m_Vertices[uiCurVertex] = mTransformation.TransformDirection(ezVec3(mesh->mVertices[vertexIdx].x, mesh->mVertices[vertexIdx].y, mesh->mVertices[vertexIdx].z));
    }
  }

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

    resCooking = ezPhysXCooking::CookTriangleMesh(xMesh, stream);

    stream.EndChunk();
  }

  if (pProp->m_MeshType == ezCollisionMeshType::ConvexHull)
  {
    stream.BeginChunk("ConvexMesh", 1);

    resCooking = ezPhysXCooking::CookConvexMesh(xMesh, stream);

    stream.EndChunk();
  }

  if (resCooking.Failed())
    return ezStatus("Cooking the triangle mesh failed.");

  return ezStatus(EZ_SUCCESS);
}
