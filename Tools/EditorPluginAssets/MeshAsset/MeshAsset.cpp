#include <PCH.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <CoreUtils/Image/Image.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>
#include <../ThirdParty/AssImp/include/scene.h>
#include <../ThirdParty/AssImp/include/Importer.hpp>
#include <../ThirdParty/AssImp/include/postprocess.h>
#include <../ThirdParty/AssImp/include/Logger.hpp>
#include <../ThirdParty/AssImp/include/LogStream.hpp>
#include <../ThirdParty/AssImp/include/DefaultLogger.hpp>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

using namespace Assimp;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshAssetDocument, ezAssetDocument, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

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
    EZ_REPORT_FAILURE("Invalid basis dir %d", basisAxis);
    return ezVec3::ZeroVector();
  }
}

static ezMat3 CalculateTransformationMatrix(const ezMeshAssetProperties* pProp)
{
  ezMat3 mResult;
  mResult.SetColumn(0, GetBasisVector(pProp->m_ForwardDir) * pProp->m_fMeshScaling);
  mResult.SetColumn(1, GetBasisVector(pProp->m_RightDir) * pProp->m_fMeshScaling);
  mResult.SetColumn(2, GetBasisVector(pProp->m_UpDir) * pProp->m_fMeshScaling);

  return mResult.GetTranspose();
}

class aiLogStream : public LogStream
{
public:
  void write(const char* message)
  {
    ezLog::Dev("AssImp: %s", message);
  }
};

ezMeshAssetDocument::ezMeshAssetDocument(const char* szDocumentPath) : ezSimpleAssetDocument<ezMeshAssetProperties, ezMeshAssetObjectManager>(szDocumentPath)
{
}

void ezMeshAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo)
{
  const ezMeshAssetProperties* pProp = GetProperties();

  if (!pProp->m_sMeshFile.IsEmpty())
    pInfo->m_FileDependencies.PushBack(pProp->m_sMeshFile);

}

ezStatus ezMeshAssetDocument::InternalTransformAsset(ezStreamWriterBase& stream, const char* szPlatform)
{
  const ezMeshAssetProperties* pProp = GetProperties();

  ezString sMeshFileAbs = pProp->m_sMeshFile;
  if (!ezEditorApp::GetInstance()->MakeDataDirectoryRelativePathAbsolute(sMeshFileAbs))
  {
    ezLog::Error("Mesh Asset Transform failed: Input Path '%s' is not in any data directory", sMeshFileAbs.GetData());
    return ezStatus("Could not make path absolute: '%s;", sMeshFileAbs.GetData());
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
      ezLog::Error("Could not import file '%s'", sMeshFileAbs.GetData());
      return ezStatus("Mesh Asset input file could not be imported");
    }
  }

  ezLog::Success("Mesh has been imported", sMeshFileAbs.GetData());

  ezLog::Info("Number of unique Meshes: %u", scene->mNumMeshes);

  ezMeshResourceDescriptor desc;

  desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
  desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::UVFloat);
  desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);

  ezUInt32 uiVertices = 0;
  ezUInt32 uiTriangles = 0;

  for (ezUInt32 i = 0; i < scene->mNumMeshes; ++i)
  {
    uiVertices += scene->mMeshes[i]->mNumVertices;
    uiTriangles += scene->mMeshes[i]->mNumFaces;
  }

  ezLog::Info("Number of Vertices: %u", uiVertices);
  ezLog::Info("Number of Triangles: %u", uiTriangles);

  desc.MeshBufferDesc().AllocateStreams(uiVertices, uiTriangles);

  ezUInt32 uiCurVertex = 0;
  ezUInt32 uiCurTriangle = 0;

  desc.SetMaterial(0, "");

  const ezMat3 mTransformation = CalculateTransformationMatrix(pProp);
  const bool bFlipTriangles = (mTransformation.GetColumn(0).Cross(mTransformation.GetColumn(1)).Dot(mTransformation.GetColumn(2)) < 0.0f);

  const ezUInt32 uiFirstIndex = bFlipTriangles ? 2 : 0;
  const ezUInt32 uiMiddleIndex = 1;
  const ezUInt32 uiLastIndex = bFlipTriangles ? 0 : 2;

  aiString name;
  ezStringBuilder sMatName;

  for (ezUInt32 i = 0; i < scene->mNumMeshes; ++i)
  {
    aiMesh* mesh = scene->mMeshes[i];
    aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];

    desc.AddSubMesh(mesh->mNumFaces, uiCurTriangle, i);

    mat->Get(AI_MATKEY_NAME, name);
    //pProp->GetResourceSlotProperty(i).m_sSlotName = name.C_Str();

    sMatName = pProp->GetResourceSlotProperty(i);
    
    desc.SetMaterial(i, sMatName);

    for (ezUInt32 f = 0; f < mesh->mNumFaces; ++f, ++uiCurTriangle)
    {
      EZ_ASSERT_DEV(mesh->mFaces[f].mNumIndices == 3, "");

      desc.MeshBufferDesc().SetTriangleIndices(uiCurTriangle, uiCurVertex + mesh->mFaces[f].mIndices[uiFirstIndex], 
        uiCurVertex + mesh->mFaces[f].mIndices[uiMiddleIndex], uiCurVertex + mesh->mFaces[f].mIndices[uiLastIndex]);
    }

    const ezUInt32 uiBaseVertex = uiCurVertex;
    
    ezUInt32 uiThisVertex = uiBaseVertex;
    for (ezUInt32 v = 0; v < mesh->mNumVertices; ++v, ++uiThisVertex)
    {
      desc.MeshBufferDesc().SetVertexData(0, uiThisVertex, mTransformation.TransformDirection(ezVec3(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z)));
    }
    uiCurVertex = uiThisVertex;

    uiThisVertex = uiBaseVertex;
    if (mesh->mTextureCoords[0])
    {
      for (ezUInt32 v = 0; v < mesh->mNumVertices; ++v, ++uiThisVertex)
      {
        desc.MeshBufferDesc().SetVertexData(1, uiThisVertex, ezVec2(mesh->mTextureCoords[0][v].x, 1.0f - mesh->mTextureCoords[0][v].y));
      }
    }
    else
    {
      for (ezUInt32 v = 0; v < mesh->mNumVertices; ++v, ++uiThisVertex)
      {
        desc.MeshBufferDesc().SetVertexData(1, uiThisVertex, ezVec2(0.0f, 0.0f));
      }
    }

    uiThisVertex = uiBaseVertex;
    if (mesh->mNormals)
    {
      for (ezUInt32 v = 0; v < mesh->mNumVertices; ++v, ++uiThisVertex)
      {
        desc.MeshBufferDesc().SetVertexData(2, uiThisVertex, ezVec3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z));
      }
    }
    else
    {
      for (ezUInt32 v = 0; v < mesh->mNumVertices; ++v, ++uiThisVertex)
      {
        desc.MeshBufferDesc().SetVertexData(2, uiThisVertex, ezVec3(0.0f, 1.0f, 0.0f));
      }

    }
  }

  desc.Save(stream);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezMeshAssetDocument::InternalRetrieveAssetInfo(const char * szPlatform)
{
  ezMeshAssetProperties* pProp = GetProperties();
  ezDocumentObjectBase* pPropObj = GetPropertyObject();

  ezString sMeshFileAbs = pProp->m_sMeshFile;
  if (!ezEditorApp::GetInstance()->MakeDataDirectoryRelativePathAbsolute(sMeshFileAbs))
  {
    ezLog::Error("Mesh Asset Transform failed: Input Path '%s' is not in any data directory", sMeshFileAbs.GetData());
    return ezStatus("Could not make path absolute: '%s;", sMeshFileAbs.GetData());
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
      ezLog::Error("Could not import file '%s'", sMeshFileAbs.GetData());
      return ezStatus("Mesh Asset input file could not be imported");
    }
  }

  ezLog::Success("Mesh has been imported", sMeshFileAbs.GetData());

  ezLog::Info("Number of unique Meshes: %u", scene->mNumMeshes);

  ezUInt32 uiVertices = 0;
  ezUInt32 uiTriangles = 0;

  for (ezUInt32 i = 0; i < scene->mNumMeshes; ++i)
  {
    uiVertices += scene->mMeshes[i]->mNumVertices;
    uiTriangles += scene->mMeshes[i]->mNumFaces;
  }

  ezLog::Info("Number of Vertices: %u", uiVertices);
  ezLog::Info("Number of Triangles: %u", uiTriangles);

  //pProp->m_uiVertices = uiVertices;
  //pProp->m_uiTriangles = uiTriangles;
  //pProp->m_SlotNames.SetCount(scene->mNumMeshes);
 
 
  aiString name;
  ezStringBuilder sMatName;

  pProp->m_Slots.SetCount(scene->mNumMeshes);
  for (ezUInt32 i = 0; i < scene->mNumMeshes; ++i)
  {
    aiMaterial* mat = scene->mMaterials[scene->mMeshes[i]->mMaterialIndex];

    mat->Get(AI_MATKEY_NAME, name);

    pProp->m_Slots[i].m_sLabel = name.C_Str();
  }


  {
    ezAbstractObjectGraph graph;
    ezRttiConverterContext context;
    ezRttiConverterWriter rttiConverter(&graph, &context, true, true);
    context.RegisterObject(pPropObj->GetGuid(), pPropObj->GetTypeAccessor().GetType(), pProp);
    auto* pNode = rttiConverter.AddObjectToGraph(pProp, "Object");

    ezDocumentObjectConverterReader objectConverter(&graph, GetObjectManager());
    objectConverter.ApplyPropertiesToObject(pNode, pPropObj);
  }

  GetSelectionManager()->Clear();
  GetSelectionManager()->AddObject(pPropObj);

  return ezStatus(EZ_SUCCESS);
}

