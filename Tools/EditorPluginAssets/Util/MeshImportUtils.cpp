#include <PCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <ModelImporter/Material.h>
#include <ModelImporter/Mesh.h>
#include <ModelImporter/ModelImporter.h>
#include <ModelImporter/Scene.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

namespace ezMeshImportUtils
{
  ezString ImportOrResolveTexture(const char* szImportSourceFolder, const char* szImportTargetFolder, const char* szTexturePath,
                                  ezModelImporter::SemanticHint::Enum hint, bool bTextureClamp)
  {
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
      ezStringBuilder guidString;
      return ezConversionUtils::ToString(textureAssetInfo->m_Data.m_Guid, guidString);
    }

    // Import otherwise.
    else
    {
      ezTextureAssetDocument* textureDocument =
          ezDynamicCast<ezTextureAssetDocument*>(ezQtEditorApp::GetSingleton()->CreateDocument(newAssetPathAbs, ezDocumentFlags::None));
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
      ezStringBuilder relTexturePath = szImportSourceFolder;
      relTexturePath.AppendPath(szTexturePath);

      ezAssetCurator::GetSingleton()->FindBestMatchForFile(relTexturePath, allowedExtensions);
      pAccessor->SetValue(pTextureAsset, "Input1", relTexturePath.GetData()).LogFailure();

      ezEnum<ezTexture2DChannelMappingEnum> channelMapping;

      // Try to map usage.
      ezEnum<ezTexture2DUsageEnum> usage;
      switch (hint)
      {
        case ezModelImporter::SemanticHint::DIFFUSE:
          usage = ezTexture2DUsageEnum::Diffuse;
          break;

        case ezModelImporter::SemanticHint::DIFFUSE_ALPHA:
          usage = ezTexture2DUsageEnum::Diffuse;
          channelMapping = ezTexture2DChannelMappingEnum::RGBA1;
          break;

        case ezModelImporter::SemanticHint::AMBIENT: // Making wild guesses here.
        case ezModelImporter::SemanticHint::EMISSIVE:
          usage = ezTexture2DUsageEnum::Other_sRGB;
          break;

        case ezModelImporter::SemanticHint::ROUGHNESS:
        case ezModelImporter::SemanticHint::METALLIC:
          channelMapping = ezTexture2DChannelMappingEnum::R1;
          usage = ezTexture2DUsageEnum::Other_Linear;
          break;

        case ezModelImporter::SemanticHint::LIGHTMAP: // Lightmap linear? Modern ones likely.
          usage = ezTexture2DUsageEnum::Other_Linear;
          break;

        case ezModelImporter::SemanticHint::NORMAL:
          usage = ezTexture2DUsageEnum::NormalMap;
          break;

        case ezModelImporter::SemanticHint::DISPLACEMENT:
          usage = ezTexture2DUsageEnum::Height;
          channelMapping = ezTexture2DChannelMappingEnum::R1;
          break;

        default:
          usage = ezTexture2DUsageEnum::Unknown;
      }

      pAccessor->SetValue(pTextureAsset, "Usage", usage.GetValue()).LogFailure();
      pAccessor->SetValue(pTextureAsset, "ChannelMapping", channelMapping.GetValue()).LogFailure();

      if (bTextureClamp)
      {
        pAccessor->SetValue(pTextureAsset, "AddressModeU", (int)ezTexture2DAddressMode::Clamp).LogFailure();
        pAccessor->SetValue(pTextureAsset, "AddressModeV", (int)ezTexture2DAddressMode::Clamp).LogFailure();
        pAccessor->SetValue(pTextureAsset, "AddressModeW", (int)ezTexture2DAddressMode::Clamp).LogFailure();
      }

      // TODO: Set... something else?

      pAccessor->FinishTransaction();
      textureDocument->SaveDocument();

      ezStringBuilder guid;
      ezConversionUtils::ToString(textureDocument->GetGuid(), guid);
      textureDocument->GetDocumentManager()->CloseDocument(textureDocument);

      return guid;
    }
  };

  void ImportMaterial(ezMaterialAssetDocument* materialDocument, const ezModelImporter::Material* material,
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
                     ezMeshImportUtils::ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, baseTexture->m_FileName,
                                                               ezModelImporter::SemanticHint::DIFFUSE, false))
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
                       ezMeshImportUtils::ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, normalTexture->m_FileName,
                                                                 ezModelImporter::SemanticHint::NORMAL, false))
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
                       ezMeshImportUtils::ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, roughnessTexture->m_FileName,
                                                                 ezModelImporter::SemanticHint::ROUGHNESS, false))
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
                     ezMeshImportUtils::ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, metalTexture->m_FileName,
                                                               ezModelImporter::SemanticHint::METALLIC, false))
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

  void ImportMeshMaterials(const ezModelImporter::Scene& scene, const ezModelImporter::Mesh& mesh,
                           ezHybridArray<ezMaterialResourceSlot, 8>& inout_MaterialSlots, const char* szImportSourceFolder,
                           const char* szImportTargetFolder)
  {
    EZ_PROFILE("ImportMeshMaterials");
    ezStringBuilder materialName, tmp;
    ezStringBuilder materialNameTemp;
    ezStringBuilder newResourcePathAbs;

    ezProgressRange range("Importing Materials", mesh.GetNumSubMeshes(), false);

    ezHashTable<const ezModelImporter::Material*, ezString> importMatToMaterialGuid;
    ezDynamicArray<ezTaskGroupID> pendingSaveTasks;
    pendingSaveTasks.Reserve(mesh.GetNumSubMeshes());
    auto WaitForPendingTasks = [&pendingSaveTasks]() {
      EZ_PROFILE("WaitForPendingTasks");
      for (ezTaskGroupID& id : pendingSaveTasks)
      {
        ezTaskSystem::WaitForGroup(id);
      }
      pendingSaveTasks.Clear();
    };

    for (ezUInt32 subMeshIdx = 0; subMeshIdx < mesh.GetNumSubMeshes(); ++subMeshIdx)
    {
      range.BeginNextStep("Importing Material");

      const ezModelImporter::SubMesh& subMesh = mesh.GetSubMesh(subMeshIdx);
      const ezModelImporter::Material* material = scene.GetMaterial(subMesh.m_Material);
      if (!material) // No material? Leave default or user set.
        continue;

      // Didn't find currently set resource, create new imported material.
      if (!ezAssetCurator::GetSingleton()->FindSubAsset(inout_MaterialSlots[subMeshIdx].m_sResource))
      {
        // Check first if we already imported this material.
        if (importMatToMaterialGuid.TryGetValue(material, inout_MaterialSlots[subMeshIdx].m_sResource))
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
            inout_MaterialSlots[subMeshIdx].m_sResource = ezConversionUtils::ToString(assetInfo->m_Data.m_Guid, tmp);
            continue;
          }
        }

        ezMaterialAssetDocument* materialDocument = ezDynamicCast<ezMaterialAssetDocument*>(
            ezQtEditorApp::GetSingleton()->CreateDocument(newResourcePathAbs, ezDocumentFlags::AsyncSave));
        if (!materialDocument)
        {
          ezLog::Error("Failed to create new material '{0}'", material->m_Name);
          continue;
        }

        ezMeshImportUtils::ImportMaterial(materialDocument, material, szImportSourceFolder, szImportTargetFolder);
        // ezAssetCurator::GetSingleton()->TransformAsset(materialDocument->GetGuid());
        inout_MaterialSlots[subMeshIdx].m_sResource = ezConversionUtils::ToString(materialDocument->GetGuid(), tmp);

        ezTaskGroupID id =
            materialDocument->SaveDocumentAsync([](ezDocument* doc, ezStatus res) { doc->GetDocumentManager()->CloseDocument(doc); });
        pendingSaveTasks.PushBack(id);

        // TODO: We have to flush because Materials create worlds in the engine process and there
        // is a world limit of 64. So at half of that we flush.
        if ((subMeshIdx % 32) == 0)
          WaitForPendingTasks();
      }

      // If we have a material now, fill the mapping.
      // It is important to do this even for "old"/known materials since a mesh might have gotten a new slot that points to the same
      // material than previous slots.
      if (inout_MaterialSlots[subMeshIdx].m_sResource)
      {
        // Note that this overwrites the slot with the newest resource.
        // Should we instead check whether this particular resource exists before we do that?
        importMatToMaterialGuid.Insert(material, inout_MaterialSlots[subMeshIdx].m_sResource);
      }
    }
    WaitForPendingTasks();
  }

  void ImportMeshAssetMaterials(const char* szAssetDocument, const char* szMeshFile, bool bUseSubFolderForImportedMaterials,
                                const ezModelImporter::Scene& scene, const ezModelImporter::Mesh& mesh,
                                ezHybridArray<ezMaterialResourceSlot, 8>& inout_MaterialSlots)
  {
    EZ_LOG_BLOCK("Import Mesh Materials");
    ezStringBuilder importTargetDirectory = szAssetDocument;

    if (bUseSubFolderForImportedMaterials)
    {
      importTargetDirectory.Append("_data");
      importTargetDirectory.Append(ezPathUtils::OsSpecificPathSeparator);
    }
    else
      importTargetDirectory = importTargetDirectory.GetFileDirectory();

    ezStringBuilder sImportSourceDirectory = ezPathUtils::GetFileDirectory(szMeshFile);

    ezMeshImportUtils::ImportMeshMaterials(scene, mesh, inout_MaterialSlots, sImportSourceDirectory, importTargetDirectory);
  }

  const ezString GetResourceSlotProperty(const ezHybridArray<ezMaterialResourceSlot, 8>& materialSlots, ezUInt32 uiSlot)
  {
    if (materialSlots.IsEmpty())
      return "";

    uiSlot %= materialSlots.GetCount();
    return materialSlots[uiSlot].m_sResource;
  }


  void AddMeshToDescriptor(ezMeshResourceDescriptor& meshDescriptor, const ezModelImporter::Scene& scene, const ezModelImporter::Mesh& mesh,
                           const ezHybridArray<ezMaterialResourceSlot, 8>& materialSlots)
  {
    ezStringBuilder defaultMaterialAssetId;

    // find the default material asset
    {
      static const char* defaultMaterialAssetPath = "Base/Materials/BaseMaterials/Lit.ezMaterialAsset";
      ezConversionUtils::ToString(ezAssetCurator::GetSingleton()->FindSubAsset(defaultMaterialAssetPath)->m_Data.m_Guid,
                                  defaultMaterialAssetId);
    }

    for (ezUInt32 subMeshIdx = 0; subMeshIdx < mesh.GetNumSubMeshes(); ++subMeshIdx)
    {
      const ezModelImporter::SubMesh& subMesh = mesh.GetSubMesh(subMeshIdx);
      const ezModelImporter::Material* material = scene.GetMaterial(subMesh.m_Material);

      meshDescriptor.AddSubMesh(subMesh.m_uiTriangleCount, subMesh.m_uiFirstTriangle, subMeshIdx);

      // Try to find material in property list.
      int assetMaterialIndex = -1;
      if (material)
      {
        for (ezUInt32 i = 0; i < materialSlots.GetCount(); ++i)
        {
          if (materialSlots[i].m_sLabel == material->m_Name)
          {
            assetMaterialIndex = i;
            break;
          }
        }
      }

      if (assetMaterialIndex >= 0)
        meshDescriptor.SetMaterial(subMeshIdx, GetResourceSlotProperty(materialSlots, assetMaterialIndex));
      else
        meshDescriptor.SetMaterial(subMeshIdx, defaultMaterialAssetId);
    }
  }

  void UpdateMaterialSlots(const char* szDocumentPath, const ezModelImporter::Scene& scene, const ezModelImporter::Mesh& mesh,
                           bool bImportMaterials, bool bUseSubFolderForImportedMaterials, const char* szMeshFile,
                           ezHybridArray<ezMaterialResourceSlot, 8>& inout_MaterialSlots)
  {
    EZ_PROFILE("UpdateMaterialSlots");
    inout_MaterialSlots.SetCount(mesh.GetNumSubMeshes());
    for (ezUInt32 subMeshIdx = 0; subMeshIdx < mesh.GetNumSubMeshes(); ++subMeshIdx)
    {
      const ezModelImporter::SubMesh& subMesh = mesh.GetSubMesh(subMeshIdx);
      const ezModelImporter::Material* material = scene.GetMaterial(subMesh.m_Material);

      if (material)
      {
        inout_MaterialSlots[subMeshIdx].m_sLabel = material->m_Name;
      }
    }

    if (bImportMaterials)
    {
      ezMeshImportUtils::ImportMeshAssetMaterials(szDocumentPath, szMeshFile, bUseSubFolderForImportedMaterials, scene, mesh,
                                                  inout_MaterialSlots);
    }

    if (mesh.GetNumSubMeshes() == 0)
    {
      inout_MaterialSlots.SetCount(1);
      inout_MaterialSlots[0].m_sLabel = "Default";
    }
  }

  void PrepareMeshForImport(ezModelImporter::Mesh& mesh, bool bRecalculateNormals, ezProgressRange& range)
  {
    const bool calculateNewNormals = !mesh.GetDataStream(ezGALVertexAttributeSemantic::Normal) || bRecalculateNormals;
    {
      ezStopwatch timer;
      if (calculateNewNormals)
      {
        range.BeginNextStep("Computing Normals");

        if (mesh.ComputeNormals().Succeeded())
          ezLog::Success("Computed normals (time {0}s)", ezArgF(timer.GetRunningTotal().GetSeconds(), 2));
        else
          ezLog::Success("Failed to compute normals");
      }
    }
    {
      const bool calculateNewTangents = !mesh.GetDataStream(ezGALVertexAttributeSemantic::Tangent) ||
                                        !mesh.GetDataStream(ezGALVertexAttributeSemantic::BiTangent) || calculateNewNormals;
      ezStopwatch timer;
      if (calculateNewTangents)
      {
        range.BeginNextStep("Computing Tangents");

        if (mesh.ComputeTangents().Succeeded())
          ezLog::Success("Computed tangents (time {0}s)", ezArgF(timer.GetRunningTotal().GetSeconds(), 2));
        else
          ezLog::Success("Failed to compute tangents");
      }
    }
  }

  ezStatus GenerateMeshBuffer(const ezModelImporter::Mesh& mesh, ezMeshResourceDescriptor& meshDescriptor, const ezMat3& mTransformation,
                              bool bInvertNormals, bool bSkinnedMesh)
  {
    const bool bFlipTriangles = ezGraphicsUtils::IsTriangleFlipRequired(mTransformation);

    ezMat3 mInverseTransform = mTransformation;
    if (mInverseTransform.Invert(0.0f).Failed())
    {
      return ezStatus("Could not invert the mesh transform matrix. Make sure the forward/right/up settings are valid.");
    }

    const ezMat3 mTransformNormals = mInverseTransform.GetTranspose();

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

    const bool bUseTexCoord0 = true;
    const bool bUseTexCoord1 = mesh.GetDataStream(ezGALVertexAttributeSemantic::TexCoord1) != nullptr;
    const bool bUseTangents = true;
    const bool bUseColor = mesh.GetDataStream(ezGALVertexAttributeSemantic::Color) != nullptr;
    const bool bUseJoints = bSkinnedMesh;

    // query data streams
    {
      dataStreams[Position] = mesh.GetDataStream(ezGALVertexAttributeSemantic::Position);
      dataStreams[Normal] = mesh.GetDataStream(ezGALVertexAttributeSemantic::Normal);

      if (bUseTexCoord0)
        dataStreams[Texcoord0] = mesh.GetDataStream(ezGALVertexAttributeSemantic::TexCoord0);

      if (bUseTexCoord1)
        dataStreams[Texcoord1] = mesh.GetDataStream(ezGALVertexAttributeSemantic::TexCoord1);

      if (bUseTangents)
      {
        dataStreams[Tangent] = mesh.GetDataStream(ezGALVertexAttributeSemantic::Tangent);
        dataStreams[BiTangent] = mesh.GetDataStream(ezGALVertexAttributeSemantic::BiTangent);
      }

      if (bUseColor)
        dataStreams[Color] = mesh.GetDataStream(ezGALVertexAttributeSemantic::Color);

      if (bUseJoints)
      {
        dataStreams[BoneIndices0] = mesh.GetDataStream(ezGALVertexAttributeSemantic::BoneIndices0);
        dataStreams[BoneWeights0] = mesh.GetDataStream(ezGALVertexAttributeSemantic::BoneWeights0);
      }
    }

    // validate basic data streams
    {

      if (dataStreams[Position] == nullptr)
        return ezStatus(ezFmt("Mesh '{0}' from has no position vertex data stream.", mesh.m_Name));

      if (dataStreams[Normal] == nullptr)
        return ezStatus(
            ezFmt("Mesh '{0}' from has no normal vertex data stream. Something went wrong during normal generation.", mesh.m_Name));
    }

    const ezModelImporter::TypedVertexDataStreamView<ezVec3> streamPosition(*dataStreams[Position]);
    const ezModelImporter::TypedVertexDataStreamView<ezVec3> streamNormal(*dataStreams[Normal]);


    // Compute indices for interleaved data.
    ezHashTable<ezModelImporter::Mesh::DataIndexBundle<Streams::ENUM_COUNT>, ezUInt32> dataIndices_to_InterleavedVertexIndices;

    ezDynamicArray<ezUInt32> triangleVertexIndices;
    {
      auto triangles = mesh.GetTriangles();
      ezModelImporter::Mesh::GenerateInterleavedVertexMapping<Streams::ENUM_COUNT>(
          triangles, dataStreams, dataIndices_to_InterleavedVertexIndices, triangleVertexIndices);
    }

    const ezUInt32 uiNumTriangles = mesh.GetNumTriangles();
    const ezUInt32 uiNumVertices = dataIndices_to_InterleavedVertexIndices.GetCount();
    EZ_ASSERT_DEBUG(triangleVertexIndices.GetCount() == uiNumTriangles * 3, "Number of indices for index buffer is not triangles times 3");

    ezLog::Info("Number of Vertices: {0}", uiNumVertices);
    ezLog::Info("Number of Triangles: {0}", uiNumTriangles);

    ezUInt32 uiStreamIdx[Streams::ENUM_COUNT] = {0};

    // Allocate buffer
    {
      uiStreamIdx[Streams::Position] =
          meshDescriptor.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);

      uiStreamIdx[Streams::Normal] =
          meshDescriptor.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);

      if (bUseTangents)
      {
        uiStreamIdx[Streams::Tangent] =
            meshDescriptor.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Tangent, ezGALResourceFormat::XYZFloat);
      }

      if (bUseTexCoord0)
      {
        uiStreamIdx[Streams::Texcoord0] =
            meshDescriptor.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::UVFloat);
      }

      if (bUseTexCoord1)
      {
        uiStreamIdx[Streams::Texcoord1] =
            meshDescriptor.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::TexCoord1, ezGALResourceFormat::UVFloat);
      }

      if (dataStreams[Color])
      {
        uiStreamIdx[Streams::Color] =
            meshDescriptor.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Color, ezGALResourceFormat::RGBAUByteNormalized);
      }

      if (bUseJoints)
      {
        uiStreamIdx[Streams::BoneIndices0] =
            meshDescriptor.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::BoneIndices0,
                                                      ezGALResourceFormat::RGBAUInt); // TODO: use 16 bit ints ?

        uiStreamIdx[Streams::BoneWeights0] =
            meshDescriptor.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::BoneWeights0,
                                                      ezGALResourceFormat::RGBAFloat); // TODO: only use HALF type
      }

      meshDescriptor.MeshBufferDesc().AllocateStreams(uiNumVertices, ezGALPrimitiveTopology::Triangles, uiNumTriangles);
    }

    // Read in vertices and set positions and normals
    {
      for (auto it = dataIndices_to_InterleavedVertexIndices.GetIterator(); it.IsValid(); ++it)
      {
        ezModelImporter::Mesh::DataIndexBundle<Streams::ENUM_COUNT> dataIndices = it.Key();
        ezUInt32 uiVertexIndex = it.Value();

        ezVec3 vPosition = streamPosition.GetValue(dataIndices[Position]);
        vPosition = mTransformation * vPosition;

        ezVec3 vNormal = streamNormal.GetValue(dataIndices[Normal]);
        vNormal = mTransformNormals.TransformDirection(vNormal);
        vNormal.NormalizeIfNotZero();

        if (bInvertNormals)
          vNormal = -vNormal;

        meshDescriptor.MeshBufferDesc().SetVertexData(uiStreamIdx[Streams::Position], uiVertexIndex, vPosition);
        meshDescriptor.MeshBufferDesc().SetVertexData(uiStreamIdx[Streams::Normal], uiVertexIndex, vNormal);
      }
    }

    // Set Tangents.
    if (bUseTangents)
    {
      if (dataStreams[Tangent] && dataStreams[BiTangent])
      {
        const ezModelImporter::TypedVertexDataStreamView<ezVec3> streamTangent(*dataStreams[Tangent]);

        for (auto it = dataIndices_to_InterleavedVertexIndices.GetIterator(); it.IsValid(); ++it)
        {
          ezModelImporter::Mesh::DataIndexBundle<Streams::ENUM_COUNT> dataIndices = it.Key();
          ezUInt32 uiVertexIndex = it.Value();

          ezVec3 vTangent = streamTangent.GetValue(dataIndices[Tangent]);
          vTangent = mTransformNormals.TransformDirection(vTangent);
          vTangent.NormalizeIfNotZero();

          float biTangentSign;

          if (dataStreams[BiTangent]->GetNumElementsPerVertex() == 1)
            biTangentSign = ezModelImporter::TypedVertexDataStreamView<float>(*dataStreams[BiTangent]).GetValue(dataIndices[BiTangent]);
          else
          {
            ezVec3 vBiTangent =
                ezModelImporter::TypedVertexDataStreamView<ezVec3>(*dataStreams[BiTangent]).GetValue(dataIndices[BiTangent]);
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

          meshDescriptor.MeshBufferDesc().SetVertexData(uiStreamIdx[Streams::Tangent], uiVertexIndex, vTangent);
        }
      }
      else
      {
        for (auto it = dataIndices_to_InterleavedVertexIndices.GetIterator(); it.IsValid(); ++it)
        {
          ezUInt32 uiVertexIndex = it.Value();
          meshDescriptor.MeshBufferDesc().SetVertexData(uiStreamIdx[Streams::Tangent], uiVertexIndex, ezVec3(0.0f));
        }
      }
    }

    // Set texture coordinates
    if (bUseTexCoord0 || bUseTexCoord1)
    {
      for (ezUInt32 i = 0; i < 2; ++i)
      {
        if (i == 0 && !bUseTexCoord0)
          continue;
        if (i == 1 && !bUseTexCoord1)
          continue;

        const ezUInt32 uiStreamIndex = (i == 0) ? uiStreamIdx[Streams::Texcoord0] : uiStreamIdx[Streams::Texcoord1];

        const ezUInt32 uiDataIndex = Streams::Texcoord0 + i;
        if (dataStreams[uiDataIndex])
        {
          const ezModelImporter::TypedVertexDataStreamView<ezVec2> streamTex(*dataStreams[uiDataIndex]);

          for (auto it = dataIndices_to_InterleavedVertexIndices.GetIterator(); it.IsValid(); ++it)
          {
            ezModelImporter::Mesh::DataIndexBundle<Streams::ENUM_COUNT> dataIndices = it.Key();
            ezUInt32 uiVertexIndex = it.Value();

            ezVec2 vTexcoord = streamTex.GetValue(dataIndices[uiDataIndex]);
            meshDescriptor.MeshBufferDesc().SetVertexData(uiStreamIndex, uiVertexIndex, vTexcoord);
          }
        }
        else
        {
          for (auto it = dataIndices_to_InterleavedVertexIndices.GetIterator(); it.IsValid(); ++it)
          {
            ezUInt32 uiVertexIndex = it.Value();
            meshDescriptor.MeshBufferDesc().SetVertexData(uiStreamIndex, uiVertexIndex, ezVec2(0.0f));
          }
        }
      }
    }

    // Set Color.
    if (bUseColor)
    {
      if (dataStreams[Color])
      {
        const ezModelImporter::TypedVertexDataStreamView<ezVec4> streamColor(*dataStreams[Color]);

        for (auto it = dataIndices_to_InterleavedVertexIndices.GetIterator(); it.IsValid(); ++it)
        {
          ezModelImporter::Mesh::DataIndexBundle<Streams::ENUM_COUNT> dataIndices = it.Key();
          ezUInt32 uiVertexIndex = it.Value();

          ezVec4 c = streamColor.GetValue(dataIndices[Color]);
          ezColorLinearUB color = ezColor(c.x, c.y, c.z, c.w);
          meshDescriptor.MeshBufferDesc().SetVertexData(uiStreamIdx[Streams::Color], uiVertexIndex, color);
        }
      }
      else
      {
        ezColorLinearUB color(255, 255, 255, 255);

        for (auto it = dataIndices_to_InterleavedVertexIndices.GetIterator(); it.IsValid(); ++it)
        {
          ezUInt32 uiVertexIndex = it.Value();
          meshDescriptor.MeshBufferDesc().SetVertexData(uiStreamIdx[Streams::Color], uiVertexIndex, color);
        }
      }
    }

    // Set bone indices/weights
    if (bUseJoints)
    {
      if (dataStreams[BoneIndices0] && dataStreams[BoneWeights0])
      {
        const ezModelImporter::TypedVertexDataStreamView<ezVec4U32> streamBoneIndices(*dataStreams[BoneIndices0]);
        const ezModelImporter::TypedVertexDataStreamView<ezVec4> streamBoneWeights(*dataStreams[BoneWeights0]);

        for (auto it = dataIndices_to_InterleavedVertexIndices.GetIterator(); it.IsValid(); ++it)
        {
          ezModelImporter::Mesh::DataIndexBundle<Streams::ENUM_COUNT> dataIndices = it.Key();
          ezUInt32 uiVertexIndex = it.Value();

          const ezVec4U32 bIdx = streamBoneIndices.GetValue(dataIndices[BoneIndices0]);
          meshDescriptor.MeshBufferDesc().SetVertexData(uiStreamIdx[Streams::BoneIndices0], uiVertexIndex, bIdx);

          const ezVec4 bWgt = streamBoneWeights.GetValue(dataIndices[BoneWeights0]);
          meshDescriptor.MeshBufferDesc().SetVertexData(uiStreamIdx[Streams::BoneWeights0], uiVertexIndex, bWgt);
        }
      }
      else
      {
        const ezVec4U32 bIdx(0, 0, 0, 0);
        const ezVec4 bWgt(1, 0, 0, 0);

        for (auto it = dataIndices_to_InterleavedVertexIndices.GetIterator(); it.IsValid(); ++it)
        {
          ezUInt32 uiVertexIndex = it.Value();

          meshDescriptor.MeshBufferDesc().SetVertexData(uiStreamIdx[Streams::BoneIndices0], uiVertexIndex, bIdx);
          meshDescriptor.MeshBufferDesc().SetVertexData(uiStreamIdx[Streams::BoneWeights0], uiVertexIndex, bWgt);
        }
      }
    }

    // Read in indices
    {
      if (bFlipTriangles)
      {
        for (ezUInt32 i = 0; i < triangleVertexIndices.GetCount(); i += 3)
          meshDescriptor.MeshBufferDesc().SetTriangleIndices(i / 3, triangleVertexIndices[i + 2], triangleVertexIndices[i + 1],
                                                             triangleVertexIndices[i + 0]);
      }
      else
      {
        for (ezUInt32 i = 0; i < triangleVertexIndices.GetCount(); i += 3)
          meshDescriptor.MeshBufferDesc().SetTriangleIndices(i / 3, triangleVertexIndices[i + 0], triangleVertexIndices[i + 1],
                                                             triangleVertexIndices[i + 2]);
      }
    }

    return ezStatus(EZ_SUCCESS);
  }

  ezStatus TryImportMesh(ezSharedPtr<ezModelImporter::Scene>& out_pScene, ezModelImporter::Mesh*& out_pMesh, const char* szMeshFile,
                         const char* szSubMeshName, const ezMat3& mMeshTransform, bool bRecalculateNormals, bool bInvertNormals,
                         ezProgressRange& range, ezMeshResourceDescriptor& meshDescriptor, bool bSkinnedMesh)
  {
    ezMat3 mInverseTransform = mMeshTransform;

    if (mInverseTransform.Invert(0.0f).Failed())
    {
      return ezStatus("Could not invert the mesh transform matrix. Make sure the forward/right/up settings are valid.");
    }

    const ezMat3 mTransformNormals = mInverseTransform.GetTranspose();
    const bool bFlipTriangles = ezGraphicsUtils::IsTriangleFlipRequired(mMeshTransform);

    ezStringBuilder sMeshFileAbs = szMeshFile;
    if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sMeshFileAbs))
    {
      return ezStatus(ezFmt("Could not make path absolute: '{0};", szMeshFile));
    }

    EZ_SUCCEED_OR_RETURN(
        ezModelImporter::Importer::GetSingleton()->ImportMesh(sMeshFileAbs, szSubMeshName, bSkinnedMesh, out_pScene, out_pMesh));

    ezMeshImportUtils::PrepareMeshForImport(*out_pMesh, bRecalculateNormals, range);

    range.BeginNextStep("Generating Mesh Data");

    ezMeshImportUtils::GenerateMeshBuffer(*out_pMesh, meshDescriptor, mMeshTransform, bInvertNormals, bSkinnedMesh);

    return ezStatus(EZ_SUCCESS);
  }
} // namespace ezMeshImportUtils
