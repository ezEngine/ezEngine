#include <PCH.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetManager.h>
#include <EditorPluginAssets/ModelImporter/ModelImporter.h>
#include <EditorPluginAssets/ModelImporter/Scene.h>
#include <EditorPluginAssets/ModelImporter/Mesh.h>
#include <EditorPluginAssets/ModelImporter/Material.h>

#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

#include <Foundation/Containers/HashTable.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <CoreUtils/Image/Image.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <CoreUtils/Geometry/GeomUtils.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshAssetDocument, 1, ezRTTINoAllocator);
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
      return ezHashing::CRC32Hash(&value, sizeof(ValueType));
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
    for (ezUInt32 t=0; t<triangles.GetCount(); ++t)
    {
      for (int v = 0; v < 3; ++v)
      {
        DataIndexBundle<NumStreams> dataIndices;
        for (int stream = 0; stream < NumStreams; ++stream)
        {
          dataIndices[stream] = dataStreams[stream]->GetDataIndex(triangles[t].m_Vertices[v]);
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

  ezLog::Success("Scene '%s' has been imported", sMeshFileAbs.GetData());

  if (scene->GetMeshes().GetCount() == 0)
  {
    return ezStatus("Scene does not contain any meshes.");
  }

  // Want a single mesh so let's all merge it together.
  // Todo: Later we might want to point to a specific mesh inside the scene!
  Mesh* mesh = scene->MergeAllMeshes();
 
  // TODO: Generate normals and tangents!
  //mesh->ComputeNormals();

  // Prepare streams.
  const static int numReadMeshStreams = 5;
  const ezModelImporter::VertexDataStream* dataStreams[numReadMeshStreams];
  dataStreams[0] = mesh->GetDataStream(ezGALVertexAttributeSemantic::Position);
  dataStreams[1] = mesh->GetDataStream(ezGALVertexAttributeSemantic::TexCoord0);
  dataStreams[2] = mesh->GetDataStream(ezGALVertexAttributeSemantic::Normal);
  dataStreams[3] = mesh->GetDataStream(ezGALVertexAttributeSemantic::Tangent);
  dataStreams[4] = mesh->GetDataStream(ezGALVertexAttributeSemantic::BiTangent);
  for (int i = 0; i < numReadMeshStreams; ++i)
  {
    if (dataStreams[i] == nullptr)
    {
      // Todo: Would be nice to output which data stream it missing.
      // Todo: We should be able to work around an empty stream!
      ezLog::Warning("Mesh '%s' from '%s' is missing a required vertex data stream.", mesh->m_Name.GetData(), sMeshFileAbs.GetData());
      return ezStatus("Mesh '%s' from '%s' is missing a required vertex data stream.", mesh->m_Name.GetData(), sMeshFileAbs.GetData());
    }
  }

  ezHashTable<ImportHelper::DataIndexBundle<numReadMeshStreams>, ezUInt32> dataIndices_to_InterleavedVertexIndices;
  ezDynamicArray<ezUInt32> triangleVertexIndices;
  auto triangles = mesh->GetTriangles();
  ImportHelper::GenerateInterleavedVertexMapping<numReadMeshStreams>(triangles, dataStreams, dataIndices_to_InterleavedVertexIndices, triangleVertexIndices);

  ezUInt32 uiNumTriangles = mesh->GetNumTriangles();
  ezUInt32 uiNumVertices = dataIndices_to_InterleavedVertexIndices.GetCount();
  EZ_ASSERT_DEBUG(triangleVertexIndices.GetCount() == uiNumTriangles * 3, "Number of indices for index buffer is not triangles times 3");

  ezLog::Info("Number of Vertices: %u", uiNumVertices);
  ezLog::Info("Number of Triangles: %u", uiNumTriangles);

  // Allocate buffer.
  const ezUInt32 uiPosStream = desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
  const ezUInt32 uiTexStream = desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::UVFloat);
  const ezUInt32 uiNormalStream = desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);
  const ezUInt32 uiTangentStream = desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Tangent, ezGALResourceFormat::XYZFloat);
  desc.MeshBufferDesc().AllocateStreams(uiNumVertices, ezGALPrimitiveTopology::Triangles, uiNumTriangles);

  // Read in vertices.
  for (auto it = dataIndices_to_InterleavedVertexIndices.GetIterator(); it.IsValid(); ++it)
  {
    ImportHelper::DataIndexBundle<numReadMeshStreams> dataIndices = it.Key();
    ezUInt32 uiVertexIndex = it.Value();

    ezVec3 vPosition = dataStreams[0]->GetValueVec3(dataIndices[0]);
    vPosition = mTransformation * vPosition;
    ezVec2 vTexcoord = dataStreams[1]->GetValueVec2(dataIndices[1]);
    ezVec3 vNormal = dataStreams[2]->GetValueVec3(dataIndices[2]);
    vNormal = mTransformation.TransformDirection(vNormal).GetNormalized();
    ezVec3 vTangent = dataStreams[3]->GetValueVec3(dataIndices[3]);
    vTangent = mTransformation.TransformDirection(vTangent).GetNormalized();
    ezVec3 vBitangent = dataStreams[4]->GetValueVec3(dataIndices[4]);
    vBitangent = mTransformation.TransformDirection(vBitangent).GetNormalized();

    // We encode the handedness of the tangent space in the length of the tangent.
    ezVec3 vBitangentTest = vNormal.Cross(vTangent);
    if (vBitangent.Dot(vBitangentTest) < 0.0f)
    {
      vTangent *= 1.7320508075688772935274463415059f; //ezMath::Root(3, 2)
    }

    desc.MeshBufferDesc().SetVertexData(uiPosStream, uiVertexIndex, vPosition);
    desc.MeshBufferDesc().SetVertexData(uiTexStream, uiVertexIndex, vTexcoord);
    desc.MeshBufferDesc().SetVertexData(uiNormalStream, uiVertexIndex, vNormal);
    desc.MeshBufferDesc().SetVertexData(uiTangentStream, uiVertexIndex, vTangent);
  }

  // Read in indices.
  if (bFlipTriangles)
  {
    for (ezUInt32 i = 0; i < triangleVertexIndices.GetCount(); i += 3)
      desc.MeshBufferDesc().SetTriangleIndices(i / 3, triangleVertexIndices[i+2], triangleVertexIndices[i+1], triangleVertexIndices[i+0]);
  }
  else
  {
    for (ezUInt32 i = 0; i < triangleVertexIndices.GetCount(); i += 3)
      desc.MeshBufferDesc().SetTriangleIndices(i / 3, triangleVertexIndices[i+0], triangleVertexIndices[i+1], triangleVertexIndices[i+2]);
  }

  // Materials/Submeshes.
  ProcessMaterials(pProp, &desc, *mesh, *scene);
  
  return ezStatus(EZ_SUCCESS);
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

    // TODO: Generate normals and tangents!

    // Want a single mesh so let's all merge it together.
    // Todo: Later we might want to point to a specific mesh inside the scene!
    const Mesh* mesh = scene->MergeAllMeshes();

    ezUInt32 uiTriangles = mesh->GetNumTriangles();
    ezLog::Info("Number of Triangles: %u", uiTriangles);

    ProcessMaterials(pProp, nullptr, *mesh, *scene);
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

void ezMeshAssetDocument::ProcessMaterials(ezMeshAssetProperties* pProp, ezMeshResourceDescriptor *desc, const ezModelImporter::Mesh& mesh, const ezModelImporter::Scene& scene)
{
  // Set materials. Try to remap existing materials by label if any.
  // Rebuild up pProp->m_Slots in the process.
  ezHybridArray<ezMaterialResourceSlot, 8> unusedMaterialSlots = std::move(pProp->m_Slots);
  if (unusedMaterialSlots.GetCount() < mesh.GetNumSubMeshes())
    unusedMaterialSlots.SetCount(mesh.GetNumSubMeshes());

  for (ezUInt32 subMeshIdx = 0; subMeshIdx<mesh.GetNumSubMeshes(); ++subMeshIdx)
  {
    const ezModelImporter::SubMesh& subMesh = mesh.GetSubMesh(subMeshIdx);
    const ezModelImporter::Material* material = scene.GetMaterial(subMesh.m_Material);

    // Look for material with same label in old list to preserve the user's material choice.
    bool reusedOldOne = false;
    for (ezUInt32 unusedSlotIdx = 0; unusedSlotIdx < unusedMaterialSlots.GetCount(); ++unusedSlotIdx)
    {
      if (unusedMaterialSlots[unusedSlotIdx].m_sLabel == material->m_Name)
      {
        pProp->m_Slots.PushBack(unusedMaterialSlots[unusedSlotIdx]);
        unusedMaterialSlots.RemoveAt(unusedSlotIdx);
        reusedOldOne = true;
        break;
      }
    }

    if (!reusedOldOne)
    {
      ezMaterialResourceSlot& newSlot = pProp->m_Slots.ExpandAndGetRef();
      newSlot.m_sLabel = material->m_Name;
    }

    // Add submesh and material connection.
    if (desc)
    {
      desc->AddSubMesh(subMesh.m_uiTriangleCount, subMesh.m_uiFirstTriangle, subMeshIdx);
      desc->SetMaterial(subMeshIdx, pProp->GetResourceSlotProperty(subMeshIdx));
    }
  }

  if (mesh.GetNumSubMeshes() == 0)
  {
    pProp->m_Slots.SetCount(1);
    pProp->m_Slots[0].m_sLabel = "Default";

    // Add submesh and material connection.
    if (desc)
    {
      desc->AddSubMesh(mesh.GetNumTriangles(), 0, 0);
      desc->SetMaterial(0, pProp->GetResourceSlotProperty(0));
    }
  }

}

ezStatus ezMeshAssetDocument::InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(AssetHeader);
  return status;
}

