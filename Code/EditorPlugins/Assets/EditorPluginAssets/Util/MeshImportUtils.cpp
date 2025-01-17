#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/Utilities/Progress.h>
#include <ModelImporter2/Importer/Importer.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

namespace ezMeshImportUtils
{
  void FillFileFilter(ezDynamicArray<ezString>& out_list, ezStringView sSeparated)
  {
    sSeparated.Split(false, out_list, ";", "*", ".");
  }

  ezString ImportOrResolveTexture(const char* szImportSourceFolder, const char* szImportTargetFolder, ezStringView sTexturePath, ezModelImporter2::TextureSemantic hint, bool bTextureClamp, const ezModelImporter2::Importer* pImporter)
  {
    if (!ezUnicodeUtils::IsValidUtf8(sTexturePath.GetStartPointer(), sTexturePath.GetEndPointer()))
    {
      ezLog::Error("Texture to resolve is not a valid UTF-8 string.");
      return ezString();
    }

    ezHybridArray<ezString, 16> allowedExtensions;
    FillFileFilter(allowedExtensions, ezFileBrowserAttribute::ImagesLdrAndHdr);

    ezStringBuilder sFinalTextureName;
    ezPathUtils::MakeValidFilename(sTexturePath.GetFileName(), '_', sFinalTextureName);

    ezStringBuilder relTexturePath = szImportSourceFolder;
    relTexturePath.AppendPath(sFinalTextureName);

    if (auto itTex = pImporter->m_OutputTextures.Find(sTexturePath); itTex.IsValid())
    {
      if (itTex.Value().m_RawData.IsEmpty() || !allowedExtensions.Contains(itTex.Value().m_sFileFormatExtension))
      {
        ezLog::Error("Mesh uses embedded texture of unsupported type ('{}').", itTex.Value().m_sFileFormatExtension);
        return ezString();
      }

      itTex.Value().GenerateFileName(sFinalTextureName);

      ezStringBuilder sEmbededFile;
      sEmbededFile = szImportTargetFolder;
      sEmbededFile.AppendPath(sFinalTextureName);

      relTexturePath = sEmbededFile;
    }

    ezStringBuilder newAssetPathAbs = szImportTargetFolder;
    newAssetPathAbs.AppendPath(sFinalTextureName);
    newAssetPathAbs.ChangeFileExtension("ezTextureAsset");

    if (auto textureAssetInfo = ezAssetCurator::GetSingleton()->FindSubAsset(newAssetPathAbs))
    {
      // Try to resolve.

      ezStringBuilder guidString;
      return ezConversionUtils::ToString(textureAssetInfo->m_Data.m_Guid, guidString);
    }
    else
    {
      // Import otherwise

      ezTextureAssetDocument* textureDocument = ezDynamicCast<ezTextureAssetDocument*>(ezQtEditorApp::GetSingleton()->CreateDocument(newAssetPathAbs, ezDocumentFlags::None));
      if (!textureDocument)
      {
        ezLog::Error("Failed to create new texture asset '{0}'", sTexturePath);
        return sFinalTextureName;
      }

      ezObjectAccessorBase* pAccessor = textureDocument->GetObjectAccessor();
      pAccessor->StartTransaction("Import Texture");
      ezDocumentObject* pTextureAsset = textureDocument->GetPropertyObject();

      if (ezAssetCurator::GetSingleton()->FindBestMatchForFile(relTexturePath, allowedExtensions).Failed())
      {
        relTexturePath = sFinalTextureName;
      }

      pAccessor->SetValueByName(pTextureAsset, "Input1", relTexturePath.GetData()).LogFailure();

      ezEnum<ezTexture2DChannelMappingEnum> channelMapping;

      // Try to map usage.
      ezEnum<ezTexConvUsage> usage;
      switch (hint)
      {
        case ezModelImporter2::TextureSemantic::DiffuseMap:
          usage = ezTexConvUsage::Color;
          break;

        case ezModelImporter2::TextureSemantic::DiffuseAlphaMap:
          usage = ezTexConvUsage::Color;
          channelMapping = ezTexture2DChannelMappingEnum::RGBA1;
          break;
        case ezModelImporter2::TextureSemantic::OcclusionMap: // Making wild guesses here.
        case ezModelImporter2::TextureSemantic::EmissiveMap:
          usage = ezTexConvUsage::Color;
          break;

        case ezModelImporter2::TextureSemantic::RoughnessMap:
        case ezModelImporter2::TextureSemantic::MetallicMap:
          channelMapping = ezTexture2DChannelMappingEnum::R1;
          usage = ezTexConvUsage::Linear;
          break;

        case ezModelImporter2::TextureSemantic::OrmMap:
          channelMapping = ezTexture2DChannelMappingEnum::RGB1;
          usage = ezTexConvUsage::Linear;
          break;

        case ezModelImporter2::TextureSemantic::NormalMap:
          usage = ezTexConvUsage::NormalMap;
          break;

        case ezModelImporter2::TextureSemantic::DisplacementMap:
          usage = ezTexConvUsage::Linear;
          channelMapping = ezTexture2DChannelMappingEnum::R1;
          break;

        default:
          usage = ezTexConvUsage::Auto;
      }

      pAccessor->SetValueByName(pTextureAsset, "Usage", usage.GetValue()).LogFailure();
      pAccessor->SetValueByName(pTextureAsset, "ChannelMapping", channelMapping.GetValue()).LogFailure();

      if (bTextureClamp)
      {
        pAccessor->SetValueByName(pTextureAsset, "AddressModeU", (int)ezImageAddressMode::Clamp).LogFailure();
        pAccessor->SetValueByName(pTextureAsset, "AddressModeV", (int)ezImageAddressMode::Clamp).LogFailure();
        pAccessor->SetValueByName(pTextureAsset, "AddressModeW", (int)ezImageAddressMode::Clamp).LogFailure();
      }

      // TODO: Set... something else?

      pAccessor->FinishTransaction();
      textureDocument->SaveDocument().LogFailure();

      ezStringBuilder guid;
      ezConversionUtils::ToString(textureDocument->GetGuid(), guid);
      textureDocument->GetDocumentManager()->CloseDocument(textureDocument);

      return guid;
    }
  };

  void SetMeshAssetMaterialSlots(ezHybridArray<ezMaterialResourceSlot, 8>& inout_materialSlots, const ezModelImporter2::Importer* pImporter)
  {
    const auto& opt = pImporter->GetImportOptions();

    const ezUInt32 uiNumSubmeshes = opt.m_pMeshOutput->GetSubMeshes().GetCount();

    inout_materialSlots.SetCount(uiNumSubmeshes);

    for (const auto& material : pImporter->m_OutputMaterials)
    {
      if (material.m_iReferencedByMesh < 0)
        continue;

      inout_materialSlots[material.m_iReferencedByMesh].m_sLabel = material.m_sName;
    }
  }

  void CopyMeshAssetMaterialSlotToResource(ezMeshResourceDescriptor& ref_desc, const ezHybridArray<ezMaterialResourceSlot, 8>& materialSlots)
  {
    for (ezUInt32 i = 0; i < materialSlots.GetCount(); ++i)
    {
      ref_desc.SetMaterial(i, materialSlots[i].m_sResource);
    }
  }

  static void ImportMeshAssetMaterialProperties(ezMaterialAssetDocument* pMaterialDoc, const ezModelImporter2::OutputMaterial& material, const char* szImportSourceFolder, const char* szImportTargetFolder, const ezModelImporter2::Importer* pImporter)
  {
    ezStringBuilder materialName = ezPathUtils::GetFileName(pMaterialDoc->GetDocumentPath());

    EZ_LOG_BLOCK("Apply Material Settings", materialName.GetData());

    ezObjectAccessorBase* pAccessor = pMaterialDoc->GetObjectAccessor();
    pAccessor->StartTransaction("Apply Material Settings");
    ezDocumentObject* pMaterialAsset = pMaterialDoc->GetPropertyObject();

    ezStringBuilder tmp;

    // Set base material.
    ezStatus res = pAccessor->SetValueByName(pMaterialAsset, "BaseMaterial", ezConversionUtils::ToString(ezMaterialAssetDocument::GetLitBaseMaterial(), tmp).GetData());
    res.LogFailure();
    if (res.Failed())
      return;

    // From now on we're setting shader properties.
    ezDocumentObject* pMaterialProperties = pMaterialDoc->GetShaderPropertyObject();

    ezVariant propertyValue;

    ezString textureAo, textureRoughness, textureMetallic;
    material.m_TextureReferences.TryGetValue(ezModelImporter2::TextureSemantic::OcclusionMap, textureAo);
    material.m_TextureReferences.TryGetValue(ezModelImporter2::TextureSemantic::RoughnessMap, textureRoughness);
    material.m_TextureReferences.TryGetValue(ezModelImporter2::TextureSemantic::MetallicMap, textureMetallic);

    const bool bHasOrmTexture = !textureRoughness.IsEmpty() && ((textureAo == textureRoughness) || (textureMetallic == textureRoughness));


    // Set base texture.
    {
      ezString textureDiffuse;

      if (material.m_TextureReferences.TryGetValue(ezModelImporter2::TextureSemantic::DiffuseMap, textureDiffuse))
      {
        pAccessor->SetValueByName(pMaterialProperties, "UseBaseTexture", true).LogFailure();
        pAccessor->SetValueByName(pMaterialProperties, "BaseTexture", ezVariant(ezMeshImportUtils::ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureDiffuse, ezModelImporter2::TextureSemantic::DiffuseMap, false, pImporter))).LogFailure();
      }
      else
      {
        pAccessor->SetValueByName(pMaterialProperties, "UseBaseTexture", false).LogFailure();
      }
    }

    // Set Normal Texture / Roughness Texture
    {
      ezString textureNormal;

      if (!material.m_TextureReferences.TryGetValue(ezModelImporter2::TextureSemantic::NormalMap, textureNormal))
      {
        // Due to the lack of options in stuff like obj files, people stuff normals into the bump slot.
        material.m_TextureReferences.TryGetValue(ezModelImporter2::TextureSemantic::DisplacementMap, textureNormal);
      }

      if (!textureNormal.IsEmpty())
      {
        pAccessor->SetValueByName(pMaterialProperties, "UseNormalTexture", true).LogFailure();

        pAccessor->SetValueByName(pMaterialProperties, "NormalTexture", ezVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureNormal, ezModelImporter2::TextureSemantic::NormalMap, false, pImporter))).LogFailure();
      }
      else
      {
        pAccessor->SetValueByName(pMaterialProperties, "NormalTexture", ezConversionUtils::ToString(ezMaterialAssetDocument::GetNeutralNormalMap(), tmp).GetData()).LogFailure();
      }
    }

    if (!bHasOrmTexture)
    {
      if (!textureRoughness.IsEmpty())
      {
        pAccessor->SetValueByName(pMaterialProperties, "UseRoughnessTexture", true).LogFailure();

        pAccessor->SetValueByName(pMaterialProperties, "RoughnessTexture", ezVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureRoughness, ezModelImporter2::TextureSemantic::RoughnessMap, false, pImporter))).LogFailure();
      }
      else
      {
        pAccessor->SetValueByName(pMaterialProperties, "RoughnessTexture", "White.color").LogFailure();
      }
    }

    // Set metallic texture
    if (!bHasOrmTexture)
    {
      if (!textureMetallic.IsEmpty())
      {
        pAccessor->SetValueByName(pMaterialProperties, "UseMetallicTexture", true).LogFailure();
        pAccessor->SetValueByName(pMaterialProperties, "MetallicTexture", ezVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureMetallic, ezModelImporter2::TextureSemantic::MetallicMap, false, pImporter))).LogFailure();
      }
    }

    // Set emissive texture
    {
      ezString textureEmissive;

      if (material.m_TextureReferences.TryGetValue(ezModelImporter2::TextureSemantic::EmissiveMap, textureEmissive))
      {
        pAccessor->SetValueByName(pMaterialProperties, "UseEmissiveTexture", true).LogFailure();
        pAccessor->SetValueByName(pMaterialProperties, "EmissiveTexture", ezVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureEmissive, ezModelImporter2::TextureSemantic::EmissiveMap, false, pImporter))).LogFailure();
      }
    }

    // Set AO texture
    if (!bHasOrmTexture)
    {
      ezString textureAo;

      if (material.m_TextureReferences.TryGetValue(ezModelImporter2::TextureSemantic::OcclusionMap, textureAo))
      {
        pAccessor->SetValueByName(pMaterialProperties, "UseOcclusionTexture", true).LogFailure();
        pAccessor->SetValueByName(pMaterialProperties, "OcclusionTexture", ezVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureAo, ezModelImporter2::TextureSemantic::OcclusionMap, false, pImporter))).LogFailure();
      }
    }

    // TODO: ambient occlusion texture

    // Set base color property
    if (material.m_Properties.TryGetValue(ezModelImporter2::PropertySemantic::DiffuseColor, propertyValue) && propertyValue.IsA<ezColor>())
    {
      pAccessor->SetValueByName(pMaterialProperties, "BaseColor", propertyValue).LogFailure();
    }

    // Set emissive color property
    if (material.m_Properties.TryGetValue(ezModelImporter2::PropertySemantic::EmissiveColor, propertyValue) && propertyValue.IsA<ezColor>())
    {
      pAccessor->SetValueByName(pMaterialProperties, "EmissiveColor", propertyValue).LogFailure();
    }

    // Set two-sided property
    if (material.m_Properties.TryGetValue(ezModelImporter2::PropertySemantic::TwosidedValue, propertyValue) && propertyValue.IsNumber())
    {
      pAccessor->SetValueByName(pMaterialProperties, "TWO_SIDED", propertyValue.ConvertTo<bool>()).LogFailure();
    }

    // Set metallic property
    if (material.m_Properties.TryGetValue(ezModelImporter2::PropertySemantic::MetallicValue, propertyValue) && propertyValue.IsNumber())
    {
      float value = propertyValue.ConvertTo<float>();

      // probably in 0-255 range
      if (value >= 1.0f)
        value = 1.0f;
      else
        value = 0.0f;

      pAccessor->SetValueByName(pMaterialProperties, "MetallicValue", value).LogFailure();
    }

    // Set roughness property
    if (material.m_Properties.TryGetValue(ezModelImporter2::PropertySemantic::RoughnessValue, propertyValue) && propertyValue.IsNumber())
    {
      float value = propertyValue.ConvertTo<float>();

      // probably in 0-255 range
      if (value > 1.0f)
        value /= 255.0f;

      value = ezMath::Clamp(value, 0.0f, 1.0f);
      value = ezMath::Lerp(0.4f, 1.0f, value);

      // the extracted roughness value is really just a guess to get started

      pAccessor->SetValueByName(pMaterialProperties, "RoughnessValue", value).LogFailure();
    }

    // Set ORM Texture
    if (bHasOrmTexture)
    {
      pAccessor->SetValueByName(pMaterialProperties, "UseOrmTexture", true).LogFailure();
      pAccessor->SetValueByName(pMaterialProperties, "UseOcclusionTexture", false).LogFailure();
      pAccessor->SetValueByName(pMaterialProperties, "UseRoughnessTexture", false).LogFailure();
      pAccessor->SetValueByName(pMaterialProperties, "UseMetallicTexture", false).LogFailure();

      pAccessor->SetValueByName(pMaterialProperties, "MetallicTexture", "").LogFailure();
      pAccessor->SetValueByName(pMaterialProperties, "OcclusionTexture", "").LogFailure();
      pAccessor->SetValueByName(pMaterialProperties, "RoughnessTexture", "").LogFailure();
      pAccessor->SetValueByName(pMaterialProperties, "RoughnessValue", 1.0f).LogFailure();
      pAccessor->SetValueByName(pMaterialProperties, "MetallicValue", 0.0f).LogFailure();

      pAccessor->SetValueByName(pMaterialProperties, "OrmTexture", ezVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureRoughness, ezModelImporter2::TextureSemantic::OrmMap, false, pImporter))).LogFailure();
    }

    // Todo:
    // * Shading Mode
    // * Mask Threshold

    pAccessor->FinishTransaction();
  }

  void ImportMeshAssetMaterials(ezHybridArray<ezMaterialResourceSlot, 8>& inout_materialSlots, ezStringView sDocumentDirectory, const ezModelImporter2::Importer* pImporter)
  {
    EZ_PROFILE_SCOPE("ImportMeshAssetMaterials");

    ezStringBuilder targetDirectory = sDocumentDirectory;
    targetDirectory.RemoveFileExtension();
    targetDirectory.Append("_data/");
    const ezStringBuilder sourceDirectory = ezPathUtils::GetFileDirectory(pImporter->GetImportOptions().m_sSourceFile);

    ezStringBuilder tmp;
    ezStringBuilder newResourcePathAbs;

    const ezUInt32 uiNumSubmeshes = inout_materialSlots.GetCount();

    if (uiNumSubmeshes == 0)
      return;

    ezProgressRange range("Importing Materials", uiNumSubmeshes, false);

    ezHashTable<const ezModelImporter2::OutputMaterial*, ezString> importMatToGuid;

    ezHybridArray<ezDocument*, 32> pendingSaveTasks;

    auto WaitForPendingTasks = [&pendingSaveTasks]()
    {
      EZ_PROFILE_SCOPE("WaitForPendingTasks");
      for (ezDocument* pDoc : pendingSaveTasks)
      {
        pDoc->GetDocumentManager()->CloseDocument(pDoc);
      }
      pendingSaveTasks.Clear();
    };

    ezHybridArray<ezString, 16> allowedExtensions;
    FillFileFilter(allowedExtensions, ezFileBrowserAttribute::ImagesLdrAndHdr);

    for (const auto& itTex : pImporter->m_OutputTextures)
    {
      if (itTex.Value().m_RawData.IsEmpty() || !allowedExtensions.Contains(itTex.Value().m_sFileFormatExtension))
      {
        ezLog::Error("Mesh uses embedded texture of unsupported type ('{}').", itTex.Value().m_sFileFormatExtension);
        continue;
      }

      ezStringBuilder sFinalTextureName;
      itTex.Value().GenerateFileName(sFinalTextureName);

      ezStringBuilder sEmbededFile;
      sEmbededFile = targetDirectory;
      sEmbededFile.AppendPath(sFinalTextureName);

      ezDeferredFileWriter out;
      out.SetOutput(sEmbededFile, true);
      out.WriteBytes(itTex.Value().m_RawData.GetPtr(), itTex.Value().m_RawData.GetCount()).AssertSuccess();
      out.Close().IgnoreResult();
    }

    for (const auto& impMaterial : pImporter->m_OutputMaterials)
    {
      if (impMaterial.m_iReferencedByMesh < 0)
        continue;

      const ezUInt32 subMeshIdx = impMaterial.m_iReferencedByMesh;

      range.BeginNextStep("Importing Material");

      // Didn't find currently set resource, create new imported material.
      if (!ezAssetCurator::GetSingleton()->FindSubAsset(inout_materialSlots[subMeshIdx].m_sResource))
      {
        // Check first if we already imported this material.
        if (importMatToGuid.TryGetValue(&impMaterial, inout_materialSlots[subMeshIdx].m_sResource))
          continue;

        // Put the new asset in the data folder.
        newResourcePathAbs = targetDirectory;
        newResourcePathAbs.AppendPath(impMaterial.m_sName);
        newResourcePathAbs.Append(".ezMaterialAsset");

        // Does the generated path already exist? Use it.
        if (const auto assetInfo = ezAssetCurator::GetSingleton()->FindSubAsset(newResourcePathAbs))
        {
          inout_materialSlots[subMeshIdx].m_sResource = ezConversionUtils::ToString(assetInfo->m_Data.m_Guid, tmp);
          continue;
        }

        ezMaterialAssetDocument* pMaterialDoc = ezDynamicCast<ezMaterialAssetDocument*>(ezQtEditorApp::GetSingleton()->CreateDocument(newResourcePathAbs, ezDocumentFlags::AsyncSave));
        if (!pMaterialDoc)
        {
          ezLog::Error("Failed to create new material '{0}'", impMaterial.m_sName);
          continue;
        }

        ImportMeshAssetMaterialProperties(pMaterialDoc, impMaterial, sourceDirectory, targetDirectory, pImporter);
        inout_materialSlots[subMeshIdx].m_sResource = ezConversionUtils::ToString(pMaterialDoc->GetGuid(), tmp);

        pMaterialDoc->SaveDocumentAsync({});
        pendingSaveTasks.PushBack(pMaterialDoc);

        // we have to flush because materials create worlds in the engine process and there is a world limit of 64
        if (pendingSaveTasks.GetCount() >= 16)
          WaitForPendingTasks();
      }

      // If we have a material now, fill the mapping.
      // It is important to do this even for "old"/known materials since a mesh might have gotten a new slot that points to the same
      // material as previous slots.
      if (!inout_materialSlots[subMeshIdx].m_sResource.IsEmpty())
      {
        importMatToGuid.Insert(&impMaterial, inout_materialSlots[subMeshIdx].m_sResource);
      }
    }

    WaitForPendingTasks();
  }

} // namespace ezMeshImportUtils
