#include <PCH.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetManager.h>
#include <EditorPluginAssets/ModelImporter/ModelImporter.h>
#include <EditorPluginAssets/ModelImporter/Scene.h>
#include <EditorPluginAssets/ModelImporter/Mesh.h>
#include <EditorPluginAssets/ModelImporter/Material.h>

#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>

#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Time/Stopwatch.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <CoreUtils/Image/Image.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <CoreUtils/Geometry/GeomUtils.h>


#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshAssetDocument, 2, ezRTTINoAllocator);
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

namespace ImportHelper
{
  using namespace ezModelImporter;

  template<int NumStreams>
  struct DataIndexBundle
  {
    EZ_DECLARE_POD_TYPE();
    /*      for (int i = 0; i < numStreams; ++i)
      {
        if (a[i != b[i])
          return false;
      }*/

    bool operator == (const DataIndexBundle& dataIndex) const
    {
      for (int i = 0; i < NumStreams; ++i)
      {
        if (m_indices[i] != dataIndex.m_indices[i])
          return false;
      }
      return true;
    }

    ezModelImporter::VertexDataIndex operator [] (int i) const { return m_indices[i]; }
    ezModelImporter::VertexDataIndex& operator [] (int i) { return m_indices[i]; }

    ezModelImporter::VertexDataIndex m_indices[NumStreams];
  };

  template <int NumStreams>
  struct ezHashHelper<typename DataIndexBundle<NumStreams>>
  {
    typedef ImportHelper::DataIndexBundle<NumStreams> ValueType;

    static ezUInt32 Hash(const ValueType& value)
    {
      return ezHashing::MurmurHash(&value, sizeof(ValueType));
    }

    static bool Equal(const ValueType& a, const ValueType& b)
    {
      return a == b;
    }
  };


  template<int NumStreams>
  static void GenerateInterleavedVertexMapping(ezArrayPtr<const Mesh::Triangle> triangles, const VertexDataStream* (&dataStreams)[NumStreams],
                                            ezHashTable<DataIndexBundle<NumStreams>, ezUInt32>& outDataIndices_to_InterleavedVertexIndices, ezDynamicArray<ezUInt32>& outTriangleVertexIndices)
  {
    outTriangleVertexIndices.SetCountUninitialized(triangles.GetCount() * 3);

    ezUInt32 nextVertexIndex = 0;
    DataIndexBundle<NumStreams> dataIndices;
    for (ezUInt32 t = 0; t < triangles.GetCount(); ++t)
    {
      for (int v = 0; v < 3; ++v)
      {
        for (int stream = 0; stream < NumStreams; ++stream)
        {
          dataIndices[stream] = dataStreams[stream] ? dataStreams[stream]->GetDataIndex(triangles[t].m_Vertices[v]) : 0;
        }

        ezUInt32 gpuVertexIndex = nextVertexIndex;
        if (outDataIndices_to_InterleavedVertexIndices.TryGetValue(dataIndices, gpuVertexIndex) == false)
        {
          outDataIndices_to_InterleavedVertexIndices.Insert(dataIndices, nextVertexIndex);
          ++nextVertexIndex;
        }

        outTriangleVertexIndices[t * 3 + v] = gpuVertexIndex;
      }
    }
  }
}

ezMeshAssetDocument::ezMeshAssetDocument(const char* szDocumentPath)
  : ezSimpleAssetDocument<ezMeshAssetProperties>(szDocumentPath, true)
{
}

ezStatus ezMeshAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform, const ezAssetFileHeader& AssetHeader)
{
  ezMeshAssetProperties* pProp = GetProperties();

  ezMeshResourceDescriptor desc;

  const ezMat3 mTransformation = CalculateTransformationMatrix(pProp);
  //const bool bFlipTriangles = mTransformation.GetDeterminant() < 0.0f; //(mTransformation.GetColumn(0).Cross(mTransformation.GetColumn(1)).Dot(mTransformation.GetColumn(2)) < 0.0f);

  if (pProp->m_PrimitiveType == ezMeshPrimitive::File)
  {
    auto ret = CreateMeshFromFile(pProp, desc, mTransformation);

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

    geom.ComputeTangents();
    CreateMeshFromGeom(pProp, geom, desc);
  }

  desc.Save(stream);

  return ezStatus(EZ_SUCCESS);
}


void ezMeshAssetDocument::CreateMeshFromGeom(const ezMeshAssetProperties* pProp, ezGeometry& geom, ezMeshResourceDescriptor& desc)
{
  if (!pProp->m_Slots.IsEmpty())
    desc.SetMaterial(0, pProp->m_Slots[0].m_sResource);
  else
    desc.SetMaterial(0, "");

  desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
  desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::XYFloat);
  desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);
  desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Tangent, ezGALResourceFormat::XYZFloat);
  desc.MeshBufferDesc().AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

  desc.AddSubMesh(desc.MeshBufferDesc().GetPrimitiveCount(), 0, 0);

}

ezStatus ezMeshAssetDocument::CreateMeshFromFile(ezMeshAssetProperties* pProp, ezMeshResourceDescriptor &desc, const ezMat3 &mTransformation)
{
  const bool bFlipTriangles = (mTransformation.GetColumn(0).Cross(mTransformation.GetColumn(1)).Dot(mTransformation.GetColumn(2)) < 0.0f);

  ezString sMeshFileAbs = pProp->m_sMeshFile;
  if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sMeshFileAbs))
  {
    ezLog::Error("Mesh Asset Transform failed: Input Path '%s' is not in any data directory", sMeshFileAbs.GetData());
    return ezStatus("Could not make path absolute: '%s;", sMeshFileAbs.GetData());
  }

  using namespace ezModelImporter;

  ezUniquePtr<Scene> scene = Importer::GetSingleton()->ImportScene(sMeshFileAbs);
  if (!scene)
  {
    ezLog::Error("Could not import file '%s'", sMeshFileAbs.GetData());
    return ezStatus("Mesh Asset input file '%s' could not be imported", sMeshFileAbs.GetData());
  }

  if (scene->GetMeshes().GetCount() == 0)
  {
    return ezStatus("Scene does not contain any meshes.");
  }

  // Want a single mesh so let's all merge it together.
  // Todo: Later we might want to point to a specific mesh inside the scene!
  Mesh* mesh = nullptr;
  {
    ezStopwatch timer;
    mesh = scene->MergeAllMeshes();
    mesh->ComputeNormals();
    mesh->ComputeTangents();
    ezLog::Success("Merged mesh, computed normals and tangents (time %.2fs)", timer.GetRunningTotal());
  }

  // Create vertex & index buffer.
  {
    ezStopwatch timer;
    // Prepare streams.
    const static int maxNumMeshStreams = 5;
    enum Streams
    {
      Position,
      Texcoord0,
      Normal,
      Tangent,
      BiTangent
    };

    const ezModelImporter::VertexDataStream* dataStreams[maxNumMeshStreams];
    dataStreams[Position] = mesh->GetDataStream(ezGALVertexAttributeSemantic::Position);
    if (dataStreams[Position] == nullptr)
    {
      ezLog::Error("Mesh '%s' from '%s' has no position vertex data stream.", mesh->m_Name.GetData(), sMeshFileAbs.GetData());
      return ezStatus("Mesh '%s' from '%s' is missing a required vertex data stream.", mesh->m_Name.GetData(), sMeshFileAbs.GetData());
    }
    dataStreams[Texcoord0] = mesh->GetDataStream(ezGALVertexAttributeSemantic::TexCoord0);
    dataStreams[Normal] = mesh->GetDataStream(ezGALVertexAttributeSemantic::Normal);
    if (dataStreams[Normal] == nullptr)
    {
      ezLog::Error("Mesh '%s' from '%s' has no normal vertex data stream. Something went wrong during normal generation.", mesh->m_Name.GetData(), sMeshFileAbs.GetData());
      return ezStatus("Mesh '%s' from '%s' has no normal vertex data stream. Something went wrong during normal generation.", mesh->m_Name.GetData(), sMeshFileAbs.GetData());
    }
    dataStreams[Tangent] = mesh->GetDataStream(ezGALVertexAttributeSemantic::Tangent);
    dataStreams[BiTangent] = mesh->GetDataStream(ezGALVertexAttributeSemantic::BiTangent);

    // Compute indices for interleaved data.
    ezHashTable<ImportHelper::DataIndexBundle<maxNumMeshStreams>, ezUInt32> dataIndices_to_InterleavedVertexIndices;
    ezDynamicArray<ezUInt32> triangleVertexIndices;
    auto triangles = mesh->GetTriangles();
    ImportHelper::GenerateInterleavedVertexMapping<maxNumMeshStreams>(triangles, dataStreams, dataIndices_to_InterleavedVertexIndices, triangleVertexIndices);
    ezTime mappingTime = timer.Checkpoint();


    ezUInt32 uiNumTriangles = mesh->GetNumTriangles();
    ezUInt32 uiNumVertices = dataIndices_to_InterleavedVertexIndices.GetCount();
    EZ_ASSERT_DEBUG(triangleVertexIndices.GetCount() == uiNumTriangles * 3, "Number of indices for index buffer is not triangles times 3");

    ezLog::Info("Number of Vertices: %u", uiNumVertices);
    ezLog::Info("Number of Triangles: %u", uiNumTriangles);

    // Seems to be necessary with current rendering pipeline.
#define GENERATE_FAKE_DATA

  // Allocate buffer.
    const ezUInt32 uiPosStream = desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    const ezUInt32 uiNormalStream = desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);
    ezUInt32 uiTangentStream = 0, uiTexStream = 0;
#ifndef GENERATE_FAKE_DATA
    if (dataStreams[Tangent] && dataStreams[BiTangent])
#endif
      uiTangentStream = desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Tangent, ezGALResourceFormat::XYZFloat);
#ifndef GENERATE_FAKE_DATA
    if (dataStreams[Texcoord0])
#endif
      uiTexStream = desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::UVFloat);
    desc.MeshBufferDesc().AllocateStreams(uiNumVertices, ezGALPrimitiveTopology::Triangles, uiNumTriangles);

    // Read in vertices.
    // Set positions and normals (should always be there.
    for (auto it = dataIndices_to_InterleavedVertexIndices.GetIterator(); it.IsValid(); ++it)
    {
      ImportHelper::DataIndexBundle<maxNumMeshStreams> dataIndices = it.Key();
      ezUInt32 uiVertexIndex = it.Value();

      ezVec3 vPosition = dataStreams[Position]->GetValueVec3(dataIndices[Position]);
      vPosition = mTransformation * vPosition;

      ezVec3 vNormal = dataStreams[Normal]->GetValueVec3(dataIndices[Normal]);
      vNormal = mTransformation.TransformDirection(vNormal);
      vNormal.NormalizeIfNotZero();

      desc.MeshBufferDesc().SetVertexData(uiPosStream, uiVertexIndex, vPosition);
      desc.MeshBufferDesc().SetVertexData(uiNormalStream, uiVertexIndex, vNormal);
    }
    // Set Tangents.
    if (dataStreams[Tangent] && dataStreams[BiTangent])
    {
      for (auto it = dataIndices_to_InterleavedVertexIndices.GetIterator(); it.IsValid(); ++it)
      {
        ImportHelper::DataIndexBundle<maxNumMeshStreams> dataIndices = it.Key();
        ezUInt32 uiVertexIndex = it.Value();

        ezVec3 vTangent = dataStreams[Tangent]->GetValueVec3(dataIndices[Tangent]);
        vTangent = mTransformation.TransformDirection(vTangent);
        vTangent.NormalizeIfNotZero();

        ezVec3 vBiTangent = dataStreams[BiTangent]->GetValueVec3(dataIndices[BiTangent]);
        vBiTangent = mTransformation.TransformDirection(vBiTangent);
        vBiTangent.NormalizeIfNotZero();

        float biTangentSign = vBiTangent.Dot(vTangent);
        biTangentSign = bFlipTriangles ? -biTangentSign : biTangentSign;

        // We encode the handedness of the tangent space in the length of the tangent.
        if (biTangentSign < 0.0f)
        {
          vTangent *= 1.7320508075688772935274463415059f; //ezMath::Root(3, 2)
        }

        desc.MeshBufferDesc().SetVertexData(uiTangentStream, uiVertexIndex, vTangent);
      }
    }
#ifdef GENERATE_FAKE_DATA
    else
    {
      for (auto it = dataIndices_to_InterleavedVertexIndices.GetIterator(); it.IsValid(); ++it)
      {
        ezUInt32 uiVertexIndex = it.Value();
        desc.MeshBufferDesc().SetVertexData(uiTangentStream, uiVertexIndex, ezVec3(0.0f));
      }
    }
#endif

    // Set Texcoords.
    if (dataStreams[Texcoord0])
    {
      for (auto it = dataIndices_to_InterleavedVertexIndices.GetIterator(); it.IsValid(); ++it)
      {
        ImportHelper::DataIndexBundle<maxNumMeshStreams> dataIndices = it.Key();
        ezUInt32 uiVertexIndex = it.Value();

        ezVec2 vTexcoord = dataStreams[Texcoord0]->GetValueVec2(dataIndices[Texcoord0]);
        desc.MeshBufferDesc().SetVertexData(uiTexStream, uiVertexIndex, vTexcoord);
      }
    }
#ifdef GENERATE_FAKE_DATA
    else
    {
      for (auto it = dataIndices_to_InterleavedVertexIndices.GetIterator(); it.IsValid(); ++it)
      {
        ezUInt32 uiVertexIndex = it.Value();
        desc.MeshBufferDesc().SetVertexData(uiTexStream, uiVertexIndex, ezVec2(0.0f));
      }
    }
#endif

    // Read in indices.
    if (bFlipTriangles)
    {
      for (ezUInt32 i = 0; i < triangleVertexIndices.GetCount(); i += 3)
        desc.MeshBufferDesc().SetTriangleIndices(i / 3, triangleVertexIndices[i + 2], triangleVertexIndices[i + 1], triangleVertexIndices[i + 0]);
    }
    else
    {
      for (ezUInt32 i = 0; i < triangleVertexIndices.GetCount(); i += 3)
        desc.MeshBufferDesc().SetTriangleIndices(i / 3, triangleVertexIndices[i + 0], triangleVertexIndices[i + 1], triangleVertexIndices[i + 2]);
    }

    ezLog::Success("Generated Vertex and Index Buffer (total time %.2fs, vertex mapping %.2fs)", timer.GetRunningTotal().GetSeconds(), mappingTime.GetSeconds());
  }

  // Materials/Submeshes.
  static const char* defaultMaterialAssetPath = "Materials/BaseMaterials/Lit.ezMaterialAsset";
  ezString defaultMaterialAssetId = ezConversionUtils::ToString(ezAssetCurator::GetSingleton()->FindAssetInfo(defaultMaterialAssetPath)->m_Info.m_DocumentID);

  for (ezUInt32 subMeshIdx = 0; subMeshIdx < mesh->GetNumSubMeshes(); ++subMeshIdx)
  {
    const ezModelImporter::SubMesh& subMesh = mesh->GetSubMesh(subMeshIdx);
    const ezModelImporter::Material* material = scene->GetMaterial(subMesh.m_Material);

    desc.AddSubMesh(subMesh.m_uiTriangleCount, subMesh.m_uiFirstTriangle, subMeshIdx);

    // Try to find material in property list.
    int assetMaterialIndex = -1;
    if (material)
    {
      for (ezUInt32 i = 0; i < pProp->m_Slots.GetCount(); ++i)
      {
        if (pProp->m_Slots[i].m_sLabel == material->m_Name)
        {
          assetMaterialIndex = i;
          break;
        }
      }
    }

    if(assetMaterialIndex >=0)
      desc.SetMaterial(subMeshIdx, pProp->GetResourceSlotProperty(assetMaterialIndex));
    else
      desc.SetMaterial(subMeshIdx, defaultMaterialAssetId);
  }

  return ezStatus(EZ_SUCCESS);
}

ezString ezMeshAssetDocument::ImportOrResolveTexture(const char* meshFileDirectory, const ezModelImporter::TextureReference* texture)
{
  ezStringBuilder absTexturePath = meshFileDirectory;
  absTexturePath.AppendPath(texture->m_FileName);
  ezStringBuilder absAssetSearchPath = absTexturePath;
  absAssetSearchPath.ChangeFileExtension("ezTextureAsset");
  absAssetSearchPath.MakeCleanPath();

  // Try to resolve.
  auto textureAssetInfo = ezAssetCurator::GetSingleton()->FindAssetInfo(absAssetSearchPath);
  if (textureAssetInfo)
    return ezConversionUtils::ToString(textureAssetInfo->m_Info.m_DocumentID);

  // Import otherwise.
  else
  {
    ezStringBuilder newAssetPathAbs;
    newAssetPathAbs.Format("%s_data/%s.ezTextureAsset", GetDocumentPath(), ezStringBuilder(texture->m_FileName).GetFileName().GetData());

    ezTextureAssetDocument* textureDocument = ezDynamicCast<ezTextureAssetDocument*>(ezQtEditorApp::GetSingleton()->CreateOrOpenDocument(true, newAssetPathAbs, false, false));
    if (!textureDocument)
    {
      ezLog::Error("Failed to create new texture asset '%s'", absTexturePath.GetData());
      return absTexturePath;
    }

    ezCommandHistory* history = textureDocument->GetCommandHistory();
    history->StartTransaction("Import Texture");
    ezUuid propertySetterTarget = textureDocument->GetPropertyObject()->GetGuid();

    auto setProperty = [&absTexturePath, textureDocument, history, &propertySetterTarget](const char* szProperty, const ezVariant& newValue) -> ezResult
    {
      ezSetObjectPropertyCommand cmd;
      cmd.m_Object = propertySetterTarget;
      cmd.m_NewValue = newValue;
      cmd.m_Index = 0;
      cmd.m_sProperty = szProperty;
      ezStatus status = history->AddCommand(cmd);
      if (status.m_Result.Failed())
      {
        ezLog::Error("Texture import '%s' failed: %s", absTexturePath.GetData(), status.m_sMessage.GetData());
        history->CancelTransaction();
        return EZ_FAILURE;
      }
      return EZ_SUCCESS;
    };

    // Set filename.
    if (setProperty("Input 1", absTexturePath.GetData()).Failed())
      return absTexturePath;

    // Try to map usage.
    ezEnum<ezTextureUsageEnum> usage;
    switch (texture->m_SemanticHint)
    {
    case ezModelImporter::SemanticHint::DIFFUSE:
      usage = ezTextureUsageEnum::Diffuse;
      break;
    case ezModelImporter::SemanticHint::AMBIENT: // Making wild guesses here.
    case ezModelImporter::SemanticHint::EMISSIVE:
      usage = ezTextureUsageEnum::Other_sRGB;
      break;
    case ezModelImporter::SemanticHint::ROUGHNESS:
    case ezModelImporter::SemanticHint::METALLIC:
    case ezModelImporter::SemanticHint::LIGHTMAP: // Lightmap linear? Modern ones likely.
      usage = ezTextureUsageEnum::Other_Linear;
      break;
    case ezModelImporter::SemanticHint::NORMAL:
      usage = ezTextureUsageEnum::NormalMap;
      break;
    case ezModelImporter::SemanticHint::DISPLACEMENT:
      usage = ezTextureUsageEnum::Height;
      break;
    default:
      usage = ezTextureUsageEnum::Unknown;
    }
    if (setProperty("Usage", usage.GetValue()).Failed())
      return absTexturePath;

    // Set... something else? Todo.

    history->FinishTransaction();
    textureDocument->SaveDocument();
    ezAssetCurator::GetSingleton()->TransformAsset(textureDocument->GetGuid());

    ezString guid = ezConversionUtils::ToString(textureDocument->GetGuid());
    textureDocument->GetDocumentManager()->CloseDocument(textureDocument);

    return guid;
  }
};

void ezMeshAssetDocument::ImportMaterials(const ezModelImporter::Scene& scene, const ezModelImporter::Mesh& mesh, ezMeshAssetProperties* pProp, const char* sMeshFileAbs)
{
  static const char* litMaterialAssetPath = "Materials/BaseMaterials/Lit.ezMaterialAsset";
  ezString litMaterialAssetId = "";
  {
    auto litMaterialAssetInfo = ezAssetCurator::GetSingleton()->FindAssetInfo(litMaterialAssetPath);
    if (litMaterialAssetInfo)
      litMaterialAssetId = ezConversionUtils::ToString(litMaterialAssetInfo->m_Info.m_DocumentID);
    else
      ezLog::SeriousWarning("Can't find default lit material %s", litMaterialAssetPath);
  }
  static const char* litAlphaTestMaterialAssetPath = "Materials/BaseMaterials/LitAlphaTest.ezMaterialAsset";
  ezString litAlphaTestMaterialAssetId = "";
  {
    auto litAlphaTestMaterialAssetInfo = ezAssetCurator::GetSingleton()->FindAssetInfo(litAlphaTestMaterialAssetPath);
    if (litAlphaTestMaterialAssetInfo)
      litAlphaTestMaterialAssetId = ezConversionUtils::ToString(litAlphaTestMaterialAssetInfo->m_Info.m_DocumentID);
    else
      ezLog::SeriousWarning("Can't find default lit alpha test material %s", litMaterialAssetPath);
  }

  ezHashTable<const ezModelImporter::Material*, ezString> importMatToMaterialGuid;
  for (ezUInt32 subMeshIdx = 0; subMeshIdx < mesh.GetNumSubMeshes(); ++subMeshIdx)
  {
    const ezModelImporter::SubMesh& subMesh = mesh.GetSubMesh(subMeshIdx);
    const ezModelImporter::Material* material = scene.GetMaterial(subMesh.m_Material);
    if (!material) // No material? Leave default or user set.
      continue;

    // Didn't find currently set resource, create new imported material.
    if (!ezAssetCurator::GetSingleton()->FindAssetInfo(pProp->m_Slots[subMeshIdx].m_sResource))
    {
      // Check first if we already imported this material.
      if (importMatToMaterialGuid.TryGetValue(material, pProp->m_Slots[subMeshIdx].m_sResource))
        continue;


      ezStringBuilder materialName = material->m_Name;
      // Might have not a name.
      if (materialName.IsEmpty())
      {
        materialName = "Unnamed";
        materialName.Append(ezConversionUtils::ToString(subMeshIdx));
      }

      // Put the new asset in the data folder.
      ezStringBuilder newResourcePathAbs;
      newResourcePathAbs.Format("%s_data/%s.ezMaterialAsset", GetDocumentPath(), materialName.GetData());

      // Does the generated path already exist? Use it.
      {
        const auto assetInfo = ezAssetCurator::GetSingleton()->FindAssetInfo(newResourcePathAbs);
        if (assetInfo != nullptr)
        {
          pProp->m_Slots[subMeshIdx].m_sResource = ezConversionUtils::ToString(assetInfo->m_Info.m_DocumentID);
          continue;
        }
      }

      ezMaterialAssetDocument* materialDocument = ezDynamicCast<ezMaterialAssetDocument*>(ezQtEditorApp::GetSingleton()->CreateOrOpenDocument(true, newResourcePathAbs, false, false));
      if (!materialDocument)
      {
        ezLog::Error("Failed to create new material '%s'", materialName.GetData());
        continue;
      }

      ezCommandHistory* history = materialDocument->GetCommandHistory();
      history->StartTransaction("Apply Materials");
      ezUuid propertySetterTarget = materialDocument->GetPropertyObject()->GetGuid();

      auto setProperty = [material, &materialName, materialDocument, history, &propertySetterTarget](const char* szProperty, const ezVariant& newValue) -> ezResult
      {
        ezSetObjectPropertyCommand cmd;
        cmd.m_Object = propertySetterTarget;
        cmd.m_NewValue = newValue;
        cmd.m_Index = 0;
        cmd.m_sProperty = szProperty;
        ezStatus status = history->AddCommand(cmd);
        if (status.m_Result.Failed())
        {
          ezLog::Error("Material import '%s' failed: %s", materialName.GetData(), status.m_sMessage.GetData());
          history->CancelTransaction();
          return EZ_FAILURE;
        }
        return EZ_SUCCESS;
      };

      ezString meshFileDirectory = ezPathUtils::GetFileDirectory(sMeshFileAbs);

      // Set base material.
      if (setProperty("Base Material", litMaterialAssetId) == EZ_FAILURE)
        continue;

      // From now on we're setting shader properties.
      propertySetterTarget = materialDocument->GetShaderPropertyObject()->GetGuid();

      // Set base color
      if (const ezModelImporter::Property* baseColor = material->GetProperty(ezModelImporter::SemanticHint::DIFFUSE))
      {
        if (setProperty("BaseColor", baseColor->m_Value) == EZ_FAILURE)
          continue;
      }

      // Set base texture.
      if (const ezModelImporter::TextureReference* baseTexture = material->GetTexture(ezModelImporter::SemanticHint::DIFFUSE))
      {
        if (setProperty("DEFAULT_MAT_USE_BASE_TEXTURE", true) == EZ_FAILURE)
          continue;
        if (setProperty("BaseTexture", ImportOrResolveTexture(meshFileDirectory, baseTexture)) == EZ_FAILURE)
          continue;
      }
      else
      {
        if (setProperty("DEFAULT_MAT_USE_BASE_TEXTURE", false) == EZ_FAILURE)
          continue;
      }

      // Set Normal Texture
      if (const ezModelImporter::TextureReference* normalTexture = material->GetTexture(ezModelImporter::SemanticHint::NORMAL))
      {
        if (setProperty("DEFAULT_MAT_USE_NORMAL_TEXTURE", true) == EZ_FAILURE)
          continue;
        if (setProperty("NormalTexture", ImportOrResolveTexture(meshFileDirectory, normalTexture)) == EZ_FAILURE)
          continue;
      }
      else
      {
        if (setProperty("DEFAULT_MAT_USE_NORMAL_TEXTURE", false) == EZ_FAILURE)
          continue;
      }

      // Todo:
      // * Shading Mode
      // * Two Sided
      // * Mask Threshold
      // * Use Metallic Texture / Metallic Texture
      // * Metallic Value
      // * Use Roughness Texture / Roughness Texture
      // * Roughness Value


      history->FinishTransaction();
      materialDocument->SaveDocument();
      ezAssetCurator::GetSingleton()->TransformAsset(materialDocument->GetGuid());
      pProp->m_Slots[subMeshIdx].m_sResource = ezConversionUtils::ToString(materialDocument->GetGuid());

      materialDocument->GetDocumentManager()->CloseDocument(materialDocument);
    }

    // If we have a material now, fill the mapping.
    // It is important to do this even for "old"/known materials since a mesh might have gotton a new slot that points to the same material than previous slots.
    if (pProp->m_Slots[subMeshIdx].m_sResource)
    {
      // Note that this overwrites the slot with the newest resource.
      // Should we instead check whether this particular resource exists before we do that?
      importMatToMaterialGuid.Insert(material, pProp->m_Slots[subMeshIdx].m_sResource);
    }
  }
}

ezStatus ezMeshAssetDocument::InternalRetrieveAssetInfo(const char* szPlatform)
{
  ezMeshAssetProperties* pProp = GetProperties();
  ezDocumentObject* pPropObj = GetPropertyObject();

  if (pProp->m_PrimitiveType == ezMeshPrimitive::File)
  {
    ezString sMeshFileAbs = pProp->m_sMeshFile;
    if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sMeshFileAbs))
    {
      ezLog::Error("Mesh Asset Transform failed: Input Path '%s' is not in any data directory", sMeshFileAbs.GetData());
      return ezStatus("Could not make path absolute: '%s;", sMeshFileAbs.GetData());
    }

    using namespace ezModelImporter;

    ezUniquePtr<Scene> scene = Importer::GetSingleton()->ImportScene(sMeshFileAbs);
    if (!scene)
    {
      ezLog::Error("Could not import file '%s'", sMeshFileAbs.GetData());
      return ezStatus("Mesh Asset input file '%s' could not be imported", sMeshFileAbs.GetData());
    }

    ezLog::Success("Scene '%s' has been imported", sMeshFileAbs.GetData());

    if (scene->GetMeshes().GetCount() == 0)
    {
      return ezStatus("Scene does not contain any meshes.");
    }

    ezLog::Success("Scene has been imported", sMeshFileAbs.GetData());

    // Want a single mesh so let's all merge it together.
    // Todo: Later we might want to point to a specific mesh inside the scene!
    const Mesh* mesh = scene->MergeAllMeshes();

    ezUInt32 uiTriangles = mesh->GetNumTriangles();
    ezLog::Info("Number of Triangles: %u", uiTriangles);


    GetObjectAccessor()->StartTransaction("Update Mesh Asset Info");

    pProp->m_Slots.SetCount(mesh->GetNumSubMeshes());
    for (ezUInt32 subMeshIdx = 0; subMeshIdx < mesh->GetNumSubMeshes(); ++subMeshIdx)
    {
      const ezModelImporter::SubMesh& subMesh = mesh->GetSubMesh(subMeshIdx);
      const ezModelImporter::Material* material = scene->GetMaterial(subMesh.m_Material);
      if(material)
        pProp->m_Slots[subMeshIdx].m_sLabel = material->m_Name;
    }

    if (pProp->m_bImportMaterials)
      ImportMaterials(*scene, *mesh, pProp, sMeshFileAbs);

    if (mesh->GetNumSubMeshes() == 0)
    {
      pProp->m_Slots.SetCount(1);
      pProp->m_Slots[0].m_sLabel = "Default";
    }
  }
  else
  {
    GetObjectAccessor()->StartTransaction("Update Mesh Asset Info");

    pProp->m_Slots.SetCount(1);
    pProp->m_Slots[0].m_sLabel = "Default";
  }

  ApplyNativePropertyChangesToObjectManager();

  GetObjectAccessor()->FinishTransaction();

  GetSelectionManager()->Clear();
  GetSelectionManager()->AddObject(pPropObj);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezMeshAssetDocument::InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(AssetHeader);
  return status;
}

