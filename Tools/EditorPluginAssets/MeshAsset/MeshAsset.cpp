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
#include <CoreUtils/Geometry/GeomUtils.h>

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
    ezLog::Dev("AssImp: %s", message);
  }
};

ezMeshAssetDocument::ezMeshAssetDocument(const char* szDocumentPath) : ezSimpleAssetDocument<ezMeshAssetProperties>(szDocumentPath)
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

  ezMeshResourceDescriptor desc;

  const ezMat3 mTransformation = CalculateTransformationMatrix(pProp);
  const bool bFlipTriangles = (mTransformation.GetColumn(0).Cross(mTransformation.GetColumn(1)).Dot(mTransformation.GetColumn(2)) < 0.0f);

  if (pProp->m_PrimitiveType == ezMeshPrimitive::File)
  {
    auto ret = CreateMeshFromFile(pProp, desc, bFlipTriangles, mTransformation);

    if (ret.m_Result.Failed())
      return ret;
  }
  else
  {
    ezGeometry geom;
    const ezMat4 mTrans(mTransformation, ezVec3::ZeroVector());

    if (pProp->m_PrimitiveType == ezMeshPrimitive::Box)
    {
      geom.AddTexturedBox(ezVec3(1.0f), ezColor::White, mTrans);
    }
    else if (pProp->m_PrimitiveType == ezMeshPrimitive::Capsule)
    {
      geom.AddCapsule(pProp->m_fRadius, ezMath::Max(0.0f, pProp->m_fHeight), ezMath::Max<ezUInt16>(3, pProp->m_uiDetail), ezMath::Max<ezUInt16>(1, pProp->m_uiDetail2), ezColor::White, mTrans);
    }
    else if (pProp->m_PrimitiveType == ezMeshPrimitive::Cone)
    {
      geom.AddCone(pProp->m_fRadius, pProp->m_fHeight, pProp->m_bCap, ezMath::Max<ezUInt16>(3, pProp->m_uiDetail), ezColor::White, mTrans);
    }
    else if (pProp->m_PrimitiveType == ezMeshPrimitive::Cylinder)
    {
      geom.AddCylinder(pProp->m_fRadius, pProp->m_fRadius2, pProp->m_fHeight, pProp->m_bCap, pProp->m_bCap2, ezMath::Max<ezUInt16>(3, pProp->m_uiDetail), ezColor::White, mTrans, 0, ezAngle::Degree(ezMath::Clamp(pProp->m_fAngle, 0.0f, 360.0f)));
    }
    else if (pProp->m_PrimitiveType == ezMeshPrimitive::GeodesicSphere)
    {
      geom.AddGeodesicSphere(pProp->m_fRadius, pProp->m_uiDetail, ezColor::White, mTrans);
    }
    else if (pProp->m_PrimitiveType == ezMeshPrimitive::HalfSphere)
    {
      geom.AddHalfSphere(pProp->m_fRadius, ezMath::Max<ezUInt16>(3, pProp->m_uiDetail), ezMath::Max<ezUInt16>(1, pProp->m_uiDetail2), pProp->m_bCap, ezColor::White, mTrans);
    }
    else if (pProp->m_PrimitiveType == ezMeshPrimitive::Pyramid)
    {
      geom.AddPyramid(ezVec3(1.0f), pProp->m_bCap, ezColor::White, mTrans);
    }
    else if (pProp->m_PrimitiveType == ezMeshPrimitive::Rect)
    {
      geom.AddRectXY(ezVec2(1.0f), ezColor::White, mTrans);
    }
    else if (pProp->m_PrimitiveType == ezMeshPrimitive::Sphere)
    {
      geom.AddSphere(pProp->m_fRadius, ezMath::Max<ezUInt16>(3, pProp->m_uiDetail), ezMath::Max<ezUInt16>(2, pProp->m_uiDetail2), ezColor::White, mTrans);
    }
    else if (pProp->m_PrimitiveType == ezMeshPrimitive::Torus)
    {
      geom.AddTorus(pProp->m_fRadius, ezMath::Max(pProp->m_fRadius + 0.01f, pProp->m_fRadius2), ezMath::Max<ezUInt16>(3, pProp->m_uiDetail), ezMath::Max<ezUInt16>(3, pProp->m_uiDetail2), ezColor::White, mTrans);
    }

    CreateMeshFromGeom(pProp, geom, bFlipTriangles, desc);
  }

  desc.Save(stream);

  return ezStatus(EZ_SUCCESS);
}


void ezMeshAssetDocument::CreateMeshFromGeom(const ezMeshAssetProperties* pProp, ezGeometry& geom, const bool bFlipTriangles, ezMeshResourceDescriptor& desc)
{
  ezDynamicArray<ezUInt16> Indices;
  Indices.Reserve(geom.GetPolygons().GetCount() * 6);

  for (ezUInt32 p = 0; p < geom.GetPolygons().GetCount(); ++p)
  {
    for (ezUInt32 v = 0; v < geom.GetPolygons()[p].m_Vertices.GetCount() - 2; ++v)
    {
      Indices.PushBack(geom.GetPolygons()[p].m_Vertices[bFlipTriangles ? v + 2 : 0]);
      Indices.PushBack(geom.GetPolygons()[p].m_Vertices[v + 1]);
      Indices.PushBack(geom.GetPolygons()[p].m_Vertices[bFlipTriangles ? 0 : v + 2]);
    }
  }

  if (!pProp->m_Slots.IsEmpty())
    desc.SetMaterial(0, pProp->m_Slots[0].m_sResource);
  else
    desc.SetMaterial(0, "");

  desc.AddSubMesh(Indices.GetCount() / 3, 0, 0);

  desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
  desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::XYFloat);
  desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);

  desc.MeshBufferDesc().AllocateStreams(geom.GetVertices().GetCount(), Indices.GetCount() / 3);

  for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
  {
    desc.MeshBufferDesc().SetVertexData<ezVec3>(0, v, geom.GetVertices()[v].m_vPosition);
    desc.MeshBufferDesc().SetVertexData<ezVec2>(1, v, geom.GetVertices()[v].m_vTexCoord);
    desc.MeshBufferDesc().SetVertexData<ezVec3>(2, v, geom.GetVertices()[v].m_vNormal);
  }

  for (ezUInt32 t = 0; t < Indices.GetCount(); t += 3)
  {
    desc.MeshBufferDesc().SetTriangleIndices(t / 3, Indices[t], Indices[t + 1], Indices[t + 2]);
  }
}

ezStatus ezMeshAssetDocument::CreateMeshFromFile(const ezMeshAssetProperties* pProp, ezMeshResourceDescriptor &desc, bool bFlipTriangles, const ezMat3 &mTransformation)
{
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

  aiString name;
  ezStringBuilder sMatName;

  for (ezUInt32 i = 0; i < scene->mNumMeshes; ++i)
  {
    aiMesh* mesh = scene->mMeshes[i];
    aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];

    desc.AddSubMesh(mesh->mNumFaces, uiCurTriangle, i);

    mat->Get(AI_MATKEY_NAME, name);

    sMatName = pProp->GetResourceSlotProperty(i);

    desc.SetMaterial(i, sMatName);

    for (ezUInt32 f = 0; f < mesh->mNumFaces; ++f, ++uiCurTriangle)
    {
      EZ_ASSERT_DEV(mesh->mFaces[f].mNumIndices == 3, "");

      desc.MeshBufferDesc().SetTriangleIndices(uiCurTriangle, uiCurVertex + mesh->mFaces[f].mIndices[bFlipTriangles ? 2 : 0],
                                               uiCurVertex + mesh->mFaces[f].mIndices[1], uiCurVertex + mesh->mFaces[f].mIndices[bFlipTriangles ? 0 : 2]);
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
        /// \todo Transform the normals correctly
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

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezMeshAssetDocument::InternalRetrieveAssetInfo(const char * szPlatform)
{
  ezMeshAssetProperties* pProp = GetProperties();
  ezDocumentObjectBase* pPropObj = GetPropertyObject();

  if (pProp->m_PrimitiveType == ezMeshPrimitive::File)
  {

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
  }
  else
  {
    pProp->m_Slots.SetCount(1);
    pProp->m_Slots[0].m_sLabel = "Default";
  }

  {
    ezAbstractObjectGraph graph;
    ezRttiConverterContext context;
    ezRttiConverterWriter rttiConverter(&graph, &context, true, true);
    context.RegisterObject(pPropObj->GetGuid(), pPropObj->GetTypeAccessor().GetType(), pProp);
    auto* pNode = rttiConverter.AddObjectToGraph(pProp, "Object");

    ezDocumentObjectConverterReader objectConverter(&graph, GetObjectManager(), ezDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
    objectConverter.ApplyPropertiesToObject(pNode, pPropObj);
  }

  GetSelectionManager()->Clear();
  GetSelectionManager()->AddObject(pPropObj);


  return ezStatus(EZ_SUCCESS);
}

