#include <PCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAsset.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/Progress.h>
#include <ModelImporter/Material.h>
#include <ModelImporter/Mesh.h>
#include <ModelImporter/ModelImporter.h>
#include <ModelImporter/VertexData.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimatedMeshAssetDocument, 4, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

static ezMat3 CalculateTransformationMatrix(const ezAnimatedMeshAssetProperties* pProp)
{
  const float us = ezMath::Clamp(pProp->m_fUniformScaling, 0.0001f, 10000.0f);

  return ezBasisAxis::CalculateTransformationMatrix(pProp->m_ForwardDir, pProp->m_RightDir, pProp->m_UpDir, us);
}

namespace ImportHelper
{
  ezStatus ImportMesh2(const char* filename, const char* subMeshFilename, ezSharedPtr<ezModelImporter::Scene>& outScene,
                      ezModelImporter::Mesh*& outMesh, ezString& outMeshFileAbs)
  {
    outMeshFileAbs = filename;
    if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(outMeshFileAbs))
    {
      return ezStatus(ezFmt("Could not make path absolute: '{0};", outMeshFileAbs));
    }

    return ezModelImporter::Importer::GetSingleton()->ImportMesh(outMeshFileAbs, subMeshFilename, outScene, outMesh);
  }
}

ezAnimatedMeshAssetDocument::ezAnimatedMeshAssetDocument(const char* szDocumentPath)
    : ezSimpleAssetDocument<ezAnimatedMeshAssetProperties>(szDocumentPath, true)
{
}

ezStatus ezAnimatedMeshAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform,
                                                             const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  ezProgressRange range("Transforming Asset", 2, false);

  ezAnimatedMeshAssetProperties* pProp = GetProperties();

  ezMeshResourceDescriptor desc;

  const ezMat3 mTransformation = CalculateTransformationMatrix(pProp);

  range.SetStepWeighting(0, 0.9);
  range.BeginNextStep("Importing Mesh");

  auto ret = CreateMeshFromFile(pProp, desc, mTransformation);

  if (ret.m_Result.Failed())
    return ret;

  range.BeginNextStep("Writing Result");
  desc.Save(stream);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezAnimatedMeshAssetDocument::CreateMeshFromFile(ezAnimatedMeshAssetProperties* pProp, ezMeshResourceDescriptor& desc,
                                                         const ezMat3& mTransformation)
{
  ezProgressRange range("Mesh Import", 6, false);

  range.SetStepWeighting(0, 0.7f);
  range.BeginNextStep("Importing Mesh Data");

  using namespace ezModelImporter;

  ezMat3 mInverseTransform = mTransformation;
  if (mInverseTransform.Invert(0.0f).Failed())
  {
    return ezStatus("Could not invert the mesh transform matrix. Make sure the forward/right/up settings are valid.");
  }

  const ezMat3 mTransformNormals = mInverseTransform.GetTranspose();

  const bool bFlipTriangles = (mTransformation.GetColumn(0).Cross(mTransformation.GetColumn(1)).Dot(mTransformation.GetColumn(2)) < 0.0f);

  ezSharedPtr<Scene> scene;
  Mesh* mesh = nullptr;
  ezString sMeshFileAbs;
  EZ_SUCCEED_OR_RETURN(ImportHelper::ImportMesh2(pProp->m_sMeshFile, "", scene, mesh, sMeshFileAbs));

  ezUInt32 uiTriangles = mesh->GetNumTriangles();
  ezLog::Info("Number of Triangles: {0}", uiTriangles);

  const bool calculateNewNormals = !mesh->GetDataStream(ezGALVertexAttributeSemantic::Normal) || pProp->m_bRecalculateNormals;
  {
    ezStopwatch timer;
    if (calculateNewNormals)
    {
      range.BeginNextStep("Computing Normals");

      if (mesh->ComputeNormals().Succeeded())
        ezLog::Success("Computed normals (time {0}s)", ezArgF(timer.GetRunningTotal().GetSeconds(), 2));
      else
        ezLog::Success("Failed to compute normals");
    }
  }
  {
    const bool calculateNewTangents = !mesh->GetDataStream(ezGALVertexAttributeSemantic::Tangent) ||
                                      !mesh->GetDataStream(ezGALVertexAttributeSemantic::BiTangent) || calculateNewNormals;
    ezStopwatch timer;
    if (calculateNewTangents)
    {
      range.BeginNextStep("Computing Tangents");

      if (mesh->ComputeTangents().Succeeded())
        ezLog::Success("Computed tangents (time {0}s)", ezArgF(timer.GetRunningTotal().GetSeconds(), 2));
      else
        ezLog::Success("Failed to compute tangents");
    }
  }

  range.BeginNextStep("Generating Mesh Data");

  // Create vertex & index buffer.
  {
    ezStopwatch timer;
    // Prepare streams.
    enum Streams
    {
      Position,
      Texcoord0,
      Texcoord1,
      Normal,
      Tangent,
      BiTangent,
      Color,
      BoneIndices0,
      BoneWeights0,

      ENUM_COUNT
    };

    const ezModelImporter::VertexDataStream* dataStreams[Streams::ENUM_COUNT] = {};
    dataStreams[Position] = mesh->GetDataStream(ezGALVertexAttributeSemantic::Position);
    if (dataStreams[Position] == nullptr)
    {
      ezLog::Error("Mesh '{0}' from '{1}' has no position vertex data stream.", mesh->m_Name, sMeshFileAbs);
      return ezStatus(ezFmt("Mesh '{0}' from '{1}' is missing a required vertex data stream.", mesh->m_Name, sMeshFileAbs));
    }
    const ezModelImporter::TypedVertexDataStreamView<ezVec3> streamPosition(*dataStreams[Position]);

    dataStreams[Texcoord0] = mesh->GetDataStream(ezGALVertexAttributeSemantic::TexCoord0);
    dataStreams[Texcoord1] = mesh->GetDataStream(ezGALVertexAttributeSemantic::TexCoord1);

    dataStreams[Normal] = mesh->GetDataStream(ezGALVertexAttributeSemantic::Normal);
    if (dataStreams[Normal] == nullptr)
    {
      ezLog::Error("Mesh '{0}' from '{1}' has no normal vertex data stream. Something went wrong during normal generation.", mesh->m_Name,
                   sMeshFileAbs);
      return ezStatus(ezFmt("Mesh '{0}' from '{1}' has no normal vertex data stream. Something went wrong during normal generation.",
                            mesh->m_Name, sMeshFileAbs));
    }
    const ezModelImporter::TypedVertexDataStreamView<ezVec3> streamNormal(*dataStreams[Normal]);

    dataStreams[Tangent] = mesh->GetDataStream(ezGALVertexAttributeSemantic::Tangent);
    dataStreams[BiTangent] = mesh->GetDataStream(ezGALVertexAttributeSemantic::BiTangent);
    dataStreams[Color] = mesh->GetDataStream(ezGALVertexAttributeSemantic::Color);
    dataStreams[BoneIndices0] = mesh->GetDataStream(ezGALVertexAttributeSemantic::BoneIndices0);
    dataStreams[BoneWeights0] = mesh->GetDataStream(ezGALVertexAttributeSemantic::BoneWeights0);

    // Compute indices for interleaved data.
    ezHashTable<Mesh::DataIndexBundle<Streams::ENUM_COUNT>, ezUInt32> dataIndices_to_InterleavedVertexIndices;
    ezDynamicArray<ezUInt32> triangleVertexIndices;
    auto triangles = mesh->GetTriangles();
    Mesh::GenerateInterleavedVertexMapping<Streams::ENUM_COUNT>(triangles, dataStreams, dataIndices_to_InterleavedVertexIndices,
                                                                triangleVertexIndices);
    ezTime mappingTime = timer.Checkpoint();


    ezUInt32 uiNumTriangles = mesh->GetNumTriangles();
    ezUInt32 uiNumVertices = dataIndices_to_InterleavedVertexIndices.GetCount();
    EZ_ASSERT_DEBUG(triangleVertexIndices.GetCount() == uiNumTriangles * 3, "Number of indices for index buffer is not triangles times 3");

    ezLog::Info("Number of Vertices: {0}", uiNumVertices);
    ezLog::Info("Number of Triangles: {0}", uiNumTriangles);

    // Seems to be necessary with current rendering pipeline.
#define GENERATE_FAKE_DATA

    // Allocate buffer.
    const ezUInt32 uiPosStream = desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    const ezUInt32 uiNormalStream = desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);
    ezUInt32 uiTangentStream = 0;
#ifndef GENERATE_FAKE_DATA
    if (dataStreams[Tangent] && dataStreams[BiTangent])
#endif
      uiTangentStream = desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Tangent, ezGALResourceFormat::XYZFloat);

    ezUInt32 uiTexStream[2] = {0, 0};
#ifndef GENERATE_FAKE_DATA
    if (dataStreams[Texcoord0])
#endif
      uiTexStream[0] = desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::UVFloat);

    if (dataStreams[Texcoord1])
    {
      uiTexStream[1] = desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::TexCoord1, ezGALResourceFormat::UVFloat);
    }

    ezUInt32 uiColorStream = 0;
    if (dataStreams[Color])
    {
      uiColorStream = desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Color, ezGALResourceFormat::RGBAUByteNormalized);
    }

    ezUInt32 uiBoneIndex0Stream = 0;
    ezUInt32 uiBoneWeight0Stream = 0;
    if (dataStreams[BoneIndices0] && dataStreams[BoneWeights0])
    {
      uiBoneIndex0Stream = desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::BoneIndices0,
                                                           ezGALResourceFormat::RGBAUInt); // TODO: use 16 bit ints ?
      uiBoneWeight0Stream = desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::BoneWeights0,
                                                            ezGALResourceFormat::RGBAFloat); // TODO: only use HALF type
    }

    desc.MeshBufferDesc().AllocateStreams(uiNumVertices, ezGALPrimitiveTopology::Triangles, uiNumTriangles);

    // Read in vertices.
    // Set positions and normals (should always be there)
    for (auto it = dataIndices_to_InterleavedVertexIndices.GetIterator(); it.IsValid(); ++it)
    {
      Mesh::DataIndexBundle<Streams::ENUM_COUNT> dataIndices = it.Key();
      ezUInt32 uiVertexIndex = it.Value();

      ezVec3 vPosition = streamPosition.GetValue(dataIndices[Position]);
      vPosition = mTransformation * vPosition;

      ezVec3 vNormal = streamNormal.GetValue(dataIndices[Normal]);
      vNormal = mTransformNormals.TransformDirection(vNormal);
      vNormal.NormalizeIfNotZero();

      if (pProp->m_bInvertNormals)
        vNormal = -vNormal;

      desc.MeshBufferDesc().SetVertexData(uiPosStream, uiVertexIndex, vPosition);
      desc.MeshBufferDesc().SetVertexData(uiNormalStream, uiVertexIndex, vNormal);
    }

    // Set Tangents.
    if (dataStreams[Tangent] && dataStreams[BiTangent])
    {
      const ezModelImporter::TypedVertexDataStreamView<ezVec3> streamTangent(*dataStreams[Tangent]);

      for (auto it = dataIndices_to_InterleavedVertexIndices.GetIterator(); it.IsValid(); ++it)
      {
        Mesh::DataIndexBundle<Streams::ENUM_COUNT> dataIndices = it.Key();
        ezUInt32 uiVertexIndex = it.Value();

        ezVec3 vTangent = streamTangent.GetValue(dataIndices[Tangent]);
        vTangent = mTransformNormals.TransformDirection(vTangent);
        vTangent.NormalizeIfNotZero();

        float biTangentSign;

        if (dataStreams[BiTangent]->GetNumElementsPerVertex() == 1)
          biTangentSign = ezModelImporter::TypedVertexDataStreamView<float>(*dataStreams[BiTangent]).GetValue(dataIndices[BiTangent]);
        else
        {
          ezVec3 vBiTangent = ezModelImporter::TypedVertexDataStreamView<ezVec3>(*dataStreams[BiTangent]).GetValue(dataIndices[BiTangent]);
          vBiTangent = mTransformNormals.TransformDirection(vBiTangent);
          vBiTangent.NormalizeIfNotZero();
          biTangentSign = -vBiTangent.Dot(vTangent);
        }

        biTangentSign = bFlipTriangles ? -biTangentSign : biTangentSign;

        // We encode the handedness of the tangent space in the length of the tangent.
        if (biTangentSign < 0.0f)
        {
          vTangent *= 1.7320508075688772935274463415059f; // ezMath::Root(3, 2)
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
    for (ezUInt32 i = 0; i < 2; ++i)
    {
      ezUInt32 uiDataIndex = Texcoord0 + i;
      if (dataStreams[uiDataIndex])
      {
        const ezModelImporter::TypedVertexDataStreamView<ezVec2> streamTex(*dataStreams[uiDataIndex]);

        for (auto it = dataIndices_to_InterleavedVertexIndices.GetIterator(); it.IsValid(); ++it)
        {
          Mesh::DataIndexBundle<Streams::ENUM_COUNT> dataIndices = it.Key();
          ezUInt32 uiVertexIndex = it.Value();

          ezVec2 vTexcoord = streamTex.GetValue(dataIndices[uiDataIndex]);
          desc.MeshBufferDesc().SetVertexData(uiTexStream[i], uiVertexIndex, vTexcoord);
        }
      }
#ifdef GENERATE_FAKE_DATA
      else if (i == 0)
      {
        for (auto it = dataIndices_to_InterleavedVertexIndices.GetIterator(); it.IsValid(); ++it)
        {
          ezUInt32 uiVertexIndex = it.Value();
          desc.MeshBufferDesc().SetVertexData(uiTexStream[i], uiVertexIndex, ezVec2(0.0f));
        }
      }
#endif
    }

    // Set Color.
    if (dataStreams[Color])
    {
      const ezModelImporter::TypedVertexDataStreamView<ezVec4> streamColor(*dataStreams[Color]);

      for (auto it = dataIndices_to_InterleavedVertexIndices.GetIterator(); it.IsValid(); ++it)
      {
        Mesh::DataIndexBundle<Streams::ENUM_COUNT> dataIndices = it.Key();
        ezUInt32 uiVertexIndex = it.Value();

        ezVec4 c = streamColor.GetValue(dataIndices[Color]);
        ezColorLinearUB color = ezColor(c.x, c.y, c.z, c.w);
        desc.MeshBufferDesc().SetVertexData(uiColorStream, uiVertexIndex, color);
      }
    }

    // Set bone indices/weights
    if (dataStreams[BoneIndices0] && dataStreams[BoneWeights0])
    {
      const ezModelImporter::TypedVertexDataStreamView<ezVec4U32> streamBoneIndices(*dataStreams[BoneIndices0]);
      const ezModelImporter::TypedVertexDataStreamView<ezVec4> streamBoneWeights(*dataStreams[BoneWeights0]);

      for (auto it = dataIndices_to_InterleavedVertexIndices.GetIterator(); it.IsValid(); ++it)
      {
        Mesh::DataIndexBundle<Streams::ENUM_COUNT> dataIndices = it.Key();
        ezUInt32 uiVertexIndex = it.Value();

        const ezVec4U32 bIdx = streamBoneIndices.GetValue(dataIndices[BoneIndices0]);
        desc.MeshBufferDesc().SetVertexData(uiBoneIndex0Stream, uiVertexIndex, bIdx);

        const ezVec4 bWgt = streamBoneWeights.GetValue(dataIndices[BoneWeights0]);
        desc.MeshBufferDesc().SetVertexData(uiBoneWeight0Stream, uiVertexIndex, bWgt);
      }
    }

    // Read in indices.
    if (bFlipTriangles)
    {
      for (ezUInt32 i = 0; i < triangleVertexIndices.GetCount(); i += 3)
        desc.MeshBufferDesc().SetTriangleIndices(i / 3, triangleVertexIndices[i + 2], triangleVertexIndices[i + 1],
                                                 triangleVertexIndices[i + 0]);
    }
    else
    {
      for (ezUInt32 i = 0; i < triangleVertexIndices.GetCount(); i += 3)
        desc.MeshBufferDesc().SetTriangleIndices(i / 3, triangleVertexIndices[i + 0], triangleVertexIndices[i + 1],
                                                 triangleVertexIndices[i + 2]);
    }

    ezLog::Success("Generated Vertex and Index Buffer (total time {0}s, vertex mapping {1}s)",
                   ezArgF(timer.GetRunningTotal().GetSeconds(), 2), ezArgF(mappingTime.GetSeconds(), 2));
  }

  // Materials/Submeshes.
  static const char* defaultMaterialAssetPath = "Base/Materials/BaseMaterials/Lit.ezMaterialAsset";
  ezStringBuilder defaultMaterialAssetId;
  ezConversionUtils::ToString(ezAssetCurator::GetSingleton()->FindSubAsset(defaultMaterialAssetPath)->m_Data.m_Guid,
                              defaultMaterialAssetId);

  range.BeginNextStep("Importing Materials");

  // Option material slot count correction & material import.
  if (pProp->m_bImportMaterials || pProp->m_Slots.GetCount() != mesh->GetNumSubMeshes())
  {
    GetObjectAccessor()->StartTransaction("Update Mesh Material Info");

    pProp->m_Slots.SetCount(mesh->GetNumSubMeshes());
    for (ezUInt32 subMeshIdx = 0; subMeshIdx < mesh->GetNumSubMeshes(); ++subMeshIdx)
    {
      const ezModelImporter::SubMesh& subMesh = mesh->GetSubMesh(subMeshIdx);
      const ezModelImporter::Material* material = scene->GetMaterial(subMesh.m_Material);
      if (material)
        pProp->m_Slots[subMeshIdx].m_sLabel = material->m_Name;
    }

    if (pProp->m_bImportMaterials)
    {
      ezStringBuilder importTargetDirectory = GetDocumentPath();

      if (pProp->m_bUseSubFolderForImportedMaterials)
      {
        importTargetDirectory.Append("_data");
        importTargetDirectory.Append(ezPathUtils::OsSpecificPathSeparator);
      }
      else
        importTargetDirectory = importTargetDirectory.GetFileDirectory();

      ezStringBuilder sImportSourceDirectory = ezPathUtils::GetFileDirectory(pProp->m_sMeshFile);

      ezStopwatch sw;
      ImportMaterials(*scene, *mesh, pProp, sImportSourceDirectory, importTargetDirectory);
      ezLog::Success("Import Materials (time {0}s)", ezArgF(sw.GetRunningTotal().GetSeconds(), 2));
    }

    if (mesh->GetNumSubMeshes() == 0)
    {
      pProp->m_Slots.SetCount(1);
      pProp->m_Slots[0].m_sLabel = "Default";
    }

    ezStopwatch sw;
    ApplyNativePropertyChangesToObjectManager();
    GetObjectAccessor()->FinishTransaction();
    ezLog::Success("Apply Native Property Changes (time {0}s)", ezArgF(sw.GetRunningTotal().GetSeconds(), 2));

    // Need to reacquire pProp pointer since it might be reallocated.
    pProp = GetProperties();
  }

  range.BeginNextStep("Setting Materials");

  // Setting materials.
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

    if (assetMaterialIndex >= 0)
      desc.SetMaterial(subMeshIdx, pProp->GetResourceSlotProperty(assetMaterialIndex));
    else
      desc.SetMaterial(subMeshIdx, defaultMaterialAssetId);
  }

  return ezStatus(EZ_SUCCESS);
}

ezString ezAnimatedMeshAssetDocument::ImportOrResolveTexture(const char* szImportSourceFolder, const char* szImportTargetFolder,
                                                             const char* szTexturePath, ezModelImporter::SemanticHint::Enum hint)
{
  ezStringBuilder relTexturePath = szImportSourceFolder;
  relTexturePath.AppendPath(szTexturePath);

  ezStringBuilder textureNameTemp = ezStringBuilder(szTexturePath).GetFileName();
  ezStringBuilder textureName;
  ezPathUtils::MakeValidFilename(textureNameTemp, '_', textureName);

  ezStringBuilder newAssetPathAbs = szImportTargetFolder;
  newAssetPathAbs.AppendPath(ezStringBuilder(szTexturePath).GetFileName().GetData());
  newAssetPathAbs.ChangeFileExtension("ezTextureAsset");

  // Try to resolve.
  auto textureAssetInfo = ezAssetCurator::GetSingleton()->FindSubAsset(newAssetPathAbs);
  if (textureAssetInfo)
  {
    return ezConversionUtils::ToString(textureAssetInfo->m_Data.m_Guid, relTexturePath); // just reusing this variable
  }

  // Import otherwise.
  else
  {
    ezTextureAssetDocument* textureDocument =
        ezDynamicCast<ezTextureAssetDocument*>(ezQtEditorApp::GetSingleton()->CreateOrOpenDocument(true, newAssetPathAbs, false, false));
    if (!textureDocument)
    {
      ezLog::Error("Failed to create new texture asset '{0}'", szTexturePath);
      return szTexturePath;
    }

    ezObjectAccessorBase* pAccessor = textureDocument->GetObjectAccessor();
    pAccessor->StartTransaction("Import Texture");
    ezDocumentObject* pTextureAsset = textureDocument->GetPropertyObject();

    // TODO: we already have a list of allowed texture formats somewhere (file browse attribute?), use that
    ezString allowedExtensions[] = {"dds", "png", "tga", "jpg"};

    // Set filename.
    ezAssetCurator::GetSingleton()->FindBestMatchForFile(relTexturePath, allowedExtensions);
    pAccessor->SetValue(pTextureAsset, "Input1", relTexturePath.GetData()).LogFailure();

    // Try to map usage.
    ezEnum<ezTexture2DUsageEnum> usage;
    switch (hint)
    {
      case ezModelImporter::SemanticHint::DIFFUSE:
        usage = ezTexture2DUsageEnum::Diffuse;
        break;
      case ezModelImporter::SemanticHint::AMBIENT: // Making wild guesses here.
      case ezModelImporter::SemanticHint::EMISSIVE:
        usage = ezTexture2DUsageEnum::Other_sRGB;
        break;
      case ezModelImporter::SemanticHint::ROUGHNESS:
      case ezModelImporter::SemanticHint::METALLIC:
      case ezModelImporter::SemanticHint::LIGHTMAP: // Lightmap linear? Modern ones likely.
        usage = ezTexture2DUsageEnum::Other_Linear;
        break;
      case ezModelImporter::SemanticHint::NORMAL:
        usage = ezTexture2DUsageEnum::NormalMap;
        break;
      case ezModelImporter::SemanticHint::DISPLACEMENT:
        usage = ezTexture2DUsageEnum::Height;
        break;
      default:
        usage = ezTexture2DUsageEnum::Unknown;
    }

    pAccessor->SetValue(pTextureAsset, "Usage", usage.GetValue()).LogFailure();

    // Set... something else? Todo.

    pAccessor->FinishTransaction();
    textureDocument->SaveDocument();
    // ezAssetCurator::GetSingleton()->TransformAsset(textureDocument->GetGuid());

    ezStringBuilder guid;
    ezConversionUtils::ToString(textureDocument->GetGuid(), guid);
    textureDocument->GetDocumentManager()->CloseDocument(textureDocument);

    return guid;
  }
};

void ezAnimatedMeshAssetDocument::ImportMaterials(const ezModelImporter::Scene& scene, const ezModelImporter::Mesh& mesh,
                                                  ezAnimatedMeshAssetProperties* pProp, const char* szImportSourceFolder,
                                                  const char* szImportTargetFolder)
{
  ezStringBuilder materialName, tmp;
  ezStringBuilder materialNameTemp;
  ezStringBuilder newResourcePathAbs;

  ezProgressRange range("Importing Materials", mesh.GetNumSubMeshes(), false);

  ezHashTable<const ezModelImporter::Material*, ezString> importMatToMaterialGuid;
  for (ezUInt32 subMeshIdx = 0; subMeshIdx < mesh.GetNumSubMeshes(); ++subMeshIdx)
  {
    range.BeginNextStep("Importing Material");

    const ezModelImporter::SubMesh& subMesh = mesh.GetSubMesh(subMeshIdx);
    const ezModelImporter::Material* material = scene.GetMaterial(subMesh.m_Material);
    if (!material) // No material? Leave default or user set.
      continue;

    // Didn't find currently set resource, create new imported material.
    if (!ezAssetCurator::GetSingleton()->FindSubAsset(pProp->m_Slots[subMeshIdx].m_sResource))
    {
      // Check first if we already imported this material.
      if (importMatToMaterialGuid.TryGetValue(material, pProp->m_Slots[subMeshIdx].m_sResource))
        continue;


      materialName = material->m_Name;
      // Might have not a name.
      if (materialName.IsEmpty())
      {
        materialName = "Unnamed";
        materialName.Append(ezConversionUtils::ToString(subMeshIdx, tmp).GetData());
      }
      else
      {
        materialNameTemp = materialName;
        ezPathUtils::MakeValidFilename(materialNameTemp, '_', materialName);
      }

      // Put the new asset in the data folder.
      newResourcePathAbs = szImportTargetFolder;
      newResourcePathAbs.AppendPath(materialName.GetData());
      newResourcePathAbs.Append(".ezMaterialAsset");

      // Does the generated path already exist? Use it.
      {
        const auto assetInfo = ezAssetCurator::GetSingleton()->FindSubAsset(newResourcePathAbs);
        if (assetInfo != nullptr)
        {
          pProp->m_Slots[subMeshIdx].m_sResource = ezConversionUtils::ToString(assetInfo->m_Data.m_Guid, tmp);
          continue;
        }
      }

      ezMaterialAssetDocument* materialDocument = ezDynamicCast<ezMaterialAssetDocument*>(
          ezQtEditorApp::GetSingleton()->CreateOrOpenDocument(true, newResourcePathAbs, false, false));
      if (!materialDocument)
      {
        ezLog::Error("Failed to create new material '{0}'", material->m_Name);
        continue;
      }

      ImportMaterial(materialDocument, material, szImportSourceFolder, szImportTargetFolder);
      materialDocument->SaveDocument();

      // ezAssetCurator::GetSingleton()->TransformAsset(materialDocument->GetGuid());
      pProp->m_Slots[subMeshIdx].m_sResource = ezConversionUtils::ToString(materialDocument->GetGuid(), tmp);

      materialDocument->GetDocumentManager()->CloseDocument(materialDocument);
    }

    // If we have a material now, fill the mapping.
    // It is important to do this even for "old"/known materials since a mesh might have gotten a new slot that points to the same material
    // than previous slots.
    if (pProp->m_Slots[subMeshIdx].m_sResource)
    {
      // Note that this overwrites the slot with the newest resource.
      // Should we instead check whether this particular resource exists before we do that?
      importMatToMaterialGuid.Insert(material, pProp->m_Slots[subMeshIdx].m_sResource);
    }
  }
}

void ezAnimatedMeshAssetDocument::ImportMaterial(ezMaterialAssetDocument* materialDocument, const ezModelImporter::Material* material,
                                                 const char* szImportSourceFolder, const char* szImportTargetFolder)
{
  ezStringBuilder materialName = ezPathUtils::GetFileName(materialDocument->GetDocumentPath());

  ezLogBlock logScope("Apply Material Settings", materialName.GetData());

  ezObjectAccessorBase* pAccessor = materialDocument->GetObjectAccessor();
  pAccessor->StartTransaction("Apply Material Settings");
  ezDocumentObject* pMaterialAsset = materialDocument->GetPropertyObject();

  ezStringBuilder tmp;

  // Set base material.
  ezStatus res = pAccessor->SetValue(pMaterialAsset, "BaseMaterial",
                                     ezConversionUtils::ToString(ezMaterialAssetDocument::GetLitBaseMaterial(), tmp).GetData());
  res.LogFailure();
  if (res.Failed())
    return;

  // From now on we're setting shader properties.
  ezDocumentObject* pMaterialProperties = materialDocument->GetShaderPropertyObject();

  // Set base color
  if (const ezModelImporter::Property* baseColor = material->GetProperty(ezModelImporter::SemanticHint::DIFFUSE))
  {
    pAccessor->SetValue(pMaterialProperties, "BaseColor", baseColor->m_Value).LogFailure();
  }

  // Set base texture.
  if (const ezModelImporter::TextureReference* baseTexture = material->GetTexture(ezModelImporter::SemanticHint::DIFFUSE))
  {
    pAccessor->SetValue(pMaterialProperties, "UseBaseTexture", true).LogFailure();
    pAccessor
        ->SetValue(pMaterialProperties, "BaseTexture",
                   ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, baseTexture->m_FileName,
                                          ezModelImporter::SemanticHint::DIFFUSE))
        .LogFailure();
  }
  else
  {
    pAccessor->SetValue(pMaterialProperties, "UseBaseTexture", false).LogFailure();
  }

  // Set Normal Texture / Roughness Texture
  const ezModelImporter::TextureReference* normalTexture = material->GetTexture(ezModelImporter::SemanticHint::NORMAL);
  if (!normalTexture)
  {
    // Due to the lack of options in stuff like obj files, people stuff normals into the bump slot.
    normalTexture = material->GetTexture(ezModelImporter::SemanticHint::DISPLACEMENT);
  }
  const ezModelImporter::TextureReference* roughnessTexture = material->GetTexture(ezModelImporter::SemanticHint::ROUGHNESS);
  if (normalTexture || roughnessTexture)
  {
    pAccessor->SetValue(pMaterialProperties, "UseNormalAndRoughnessTexture", true).LogFailure();
    if (normalTexture)
      pAccessor
          ->SetValue(pMaterialProperties, "NormalTexture",
                     ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, normalTexture->m_FileName,
                                            ezModelImporter::SemanticHint::NORMAL))
          .LogFailure();
    else
    {
      pAccessor
          ->SetValue(pMaterialProperties, "NormalTexture",
                     ezConversionUtils::ToString(ezMaterialAssetDocument::GetNeutralNormalMap(), tmp).GetData())
          .LogFailure();
    }
    if (roughnessTexture)
      pAccessor
          ->SetValue(pMaterialProperties, "RoughnessTexture",
                     ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, roughnessTexture->m_FileName,
                                            ezModelImporter::SemanticHint::ROUGHNESS))
          .LogFailure();
    else
      pAccessor->SetValue(pMaterialProperties, "RoughnessTexture", "White.color").LogFailure();
  }
  else
  {
    pAccessor->SetValue(pMaterialProperties, "UseNormalAndRoughnessTexture", false).LogFailure();
  }

  // Set base texture.
  const ezModelImporter::TextureReference* metalTexture = material->GetTexture(ezModelImporter::SemanticHint::METALLIC);
  if (!metalTexture)
  {
    // Due to the lack of options in stuff like obj files, people stuff metallic into the ambient slot.
    metalTexture = material->GetTexture(ezModelImporter::SemanticHint::AMBIENT);
  }
  if (metalTexture)
  {
    pAccessor->SetValue(pMaterialProperties, "UseMetallicTexture", true).LogFailure();
    pAccessor
        ->SetValue(pMaterialProperties, "MetallicTexture",
                   ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, metalTexture->m_FileName,
                                          ezModelImporter::SemanticHint::METALLIC))
        .LogFailure();
  }
  else
  {
    pAccessor->SetValue(pMaterialProperties, "UseMetallicTexture", false).LogFailure();
  }

  // Todo:
  // * Shading Mode
  // * Two Sided
  // * Mask Threshold
  // * Metallic Value
  // * Roughness Value

  pAccessor->FinishTransaction();
}

ezStatus ezAnimatedMeshAssetDocument::InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(AssetHeader);
  return status;
}


//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimatedMeshAssetDocumentGenerator, 1, ezRTTIDefaultAllocator<ezAnimatedMeshAssetDocumentGenerator>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezAnimatedMeshAssetDocumentGenerator::ezAnimatedMeshAssetDocumentGenerator()
{
  AddSupportedFileType("fbx");
}

ezAnimatedMeshAssetDocumentGenerator::~ezAnimatedMeshAssetDocumentGenerator() {}

void ezAnimatedMeshAssetDocumentGenerator::GetImportModes(const char* szParentDirRelativePath,
                                                          ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const
{
  ezStringBuilder baseOutputFile = szParentDirRelativePath;
  baseOutputFile.ChangeFileExtension(GetDocumentExtension());

  {
    ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    info.m_sName = "AnimatedMeshImport.WithMaterials";
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sIcon = ":/AssetIcons/Animated_Mesh.png";
  }

  {
    ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    info.m_sName = "AnimatedMeshImport.NoMaterials";
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sIcon = ":/AssetIcons/Animated_Mesh.png";
  }
}

ezStatus ezAnimatedMeshAssetDocumentGenerator::Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info,
                                                        ezDocument*& out_pGeneratedDocument)
{
  auto pApp = ezQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateOrOpenDocument(true, info.m_sOutputFileAbsolute, false, false);
  if (out_pGeneratedDocument == nullptr)
    return ezStatus("Could not create target document");

  ezAnimatedMeshAssetDocument* pAssetDoc = ezDynamicCast<ezAnimatedMeshAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return ezStatus("Target document is not a valid ezAnimatedMeshAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("MeshFile", szDataDirRelativePath);

  if (info.m_sName == "AnimatedMeshImport.WithMaterials")
  {
    accessor.SetValue("ImportMaterials", true);
  }

  if (info.m_sName == "AnimatedMeshImport.NoMaterials")
  {
    accessor.SetValue("ImportMaterials", false);
  }

  return ezStatus(EZ_SUCCESS);
}
