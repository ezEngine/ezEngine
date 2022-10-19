#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <ModelImporter2/Importer/Importer.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

namespace ezMeshImportUtils
{
  ezString ImportOrResolveTexture(const char* szImportSourceFolder, const char* szImportTargetFolder, const char* szTexturePath, ezModelImporter2::TextureSemantic hint, bool bTextureClamp)
  {
    if (!ezUnicodeUtils::IsValidUtf8(szTexturePath))
    {
      ezLog::Error("Texture to resolve is not a valid UTF-8 string.");
      return ezString();
    }

    ezStringBuilder textureNameTemp = ezStringBuilder(szTexturePath).GetFileName();
    ezStringBuilder textureName;
    ezPathUtils::MakeValidFilename(textureNameTemp, '_', textureName);

    ezStringBuilder newAssetPathAbs = szImportTargetFolder;
    newAssetPathAbs.AppendPath(ezStringBuilder(szTexturePath).GetFileNameAndExtension().GetStartPointer());
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
      ezTextureAssetDocument* textureDocument = ezDynamicCast<ezTextureAssetDocument*>(ezQtEditorApp::GetSingleton()->CreateDocument(newAssetPathAbs, ezDocumentFlags::None));
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

      ezAssetCurator::GetSingleton()->FindBestMatchForFile(relTexturePath, allowedExtensions).IgnoreResult();
      pAccessor->SetValue(pTextureAsset, "Input1", relTexturePath.GetData()).LogFailure();

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
        case ezModelImporter2::TextureSemantic::AmbientMap: // Making wild guesses here.
        case ezModelImporter2::TextureSemantic::EmissiveMap:
          usage = ezTexConvUsage::Color;
          break;

        case ezModelImporter2::TextureSemantic::RoughnessMap:
        case ezModelImporter2::TextureSemantic::MetallicMap:
          channelMapping = ezTexture2DChannelMappingEnum::R1;
          usage = ezTexConvUsage::Linear;
          break;

        case ezModelImporter2::TextureSemantic::AoRoughMetalMap:
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

      pAccessor->SetValue(pTextureAsset, "Usage", usage.GetValue()).LogFailure();
      pAccessor->SetValue(pTextureAsset, "ChannelMapping", channelMapping.GetValue()).LogFailure();

      if (bTextureClamp)
      {
        pAccessor->SetValue(pTextureAsset, "AddressModeU", (int)ezImageAddressMode::Clamp).LogFailure();
        pAccessor->SetValue(pTextureAsset, "AddressModeV", (int)ezImageAddressMode::Clamp).LogFailure();
        pAccessor->SetValue(pTextureAsset, "AddressModeW", (int)ezImageAddressMode::Clamp).LogFailure();
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

  void SetMeshAssetMaterialSlots(ezHybridArray<ezMaterialResourceSlot, 8>& inout_MaterialSlots, const ezModelImporter2::Importer* pImporter)
  {
    const auto& opt = pImporter->GetImportOptions();

    const ezUInt32 uiNumSubmeshes = opt.m_pMeshOutput->GetSubMeshes().GetCount();

    inout_MaterialSlots.SetCount(uiNumSubmeshes);

    for (const auto& material : pImporter->m_OutputMaterials)
    {
      if (material.m_iReferencedByMesh < 0)
        continue;

      inout_MaterialSlots[material.m_iReferencedByMesh].m_sLabel = material.m_sName;
    }
  }

  void CopyMeshAssetMaterialSlotToResource(ezMeshResourceDescriptor& desc, const ezHybridArray<ezMaterialResourceSlot, 8>& materialSlots)
  {
    for (ezUInt32 i = 0; i < materialSlots.GetCount(); ++i)
    {
      desc.SetMaterial(i, materialSlots[i].m_sResource);
    }
  }

  static void ImportMeshAssetMaterialProperties(ezMaterialAssetDocument* pMaterialDoc, const ezModelImporter2::OutputMaterial& material, const char* szImportSourceFolder, const char* szImportTargetFolder)
  {
    ezStringBuilder materialName = ezPathUtils::GetFileName(pMaterialDoc->GetDocumentPath());

    EZ_LOG_BLOCK("Apply Material Settings", materialName.GetData());

    ezObjectAccessorBase* pAccessor = pMaterialDoc->GetObjectAccessor();
    pAccessor->StartTransaction("Apply Material Settings");
    ezDocumentObject* pMaterialAsset = pMaterialDoc->GetPropertyObject();

    ezStringBuilder tmp;

    // Set base material.
    ezStatus res = pAccessor->SetValue(pMaterialAsset, "BaseMaterial", ezConversionUtils::ToString(ezMaterialAssetDocument::GetLitBaseMaterial(), tmp).GetData());
    res.LogFailure();
    if (res.Failed())
      return;

    // From now on we're setting shader properties.
    ezDocumentObject* pMaterialProperties = pMaterialDoc->GetShaderPropertyObject();

    ezVariant propertyValue;

    ezString textureAo, textureRoughness, textureMetallic;
    material.m_TextureReferences.TryGetValue(ezModelImporter2::TextureSemantic::AmbientMap, textureAo);
    material.m_TextureReferences.TryGetValue(ezModelImporter2::TextureSemantic::RoughnessMap, textureRoughness);
    material.m_TextureReferences.TryGetValue(ezModelImporter2::TextureSemantic::MetallicMap, textureMetallic);

    const bool bHasArmTexture = !textureRoughness.IsEmpty() && ((textureAo == textureRoughness) || (textureMetallic == textureRoughness));


    // Set base texture.
    {
      ezString textureDiffuse;

      if (material.m_TextureReferences.TryGetValue(ezModelImporter2::TextureSemantic::DiffuseMap, textureDiffuse))
      {
        pAccessor->SetValue(pMaterialProperties, "UseBaseTexture", true).LogFailure();
        pAccessor->SetValue(pMaterialProperties, "BaseTexture", ezVariant(ezMeshImportUtils::ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureDiffuse, ezModelImporter2::TextureSemantic::DiffuseMap, false))).LogFailure();
      }
      else
      {
        pAccessor->SetValue(pMaterialProperties, "UseBaseTexture", false).LogFailure();
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
        pAccessor->SetValue(pMaterialProperties, "UseNormalAndRoughnessTexture", true).LogFailure();

        pAccessor->SetValue(pMaterialProperties, "NormalTexture", ezVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureNormal, ezModelImporter2::TextureSemantic::NormalMap, false))).LogFailure();
      }
      else
      {
        pAccessor->SetValue(pMaterialProperties, "NormalTexture", ezConversionUtils::ToString(ezMaterialAssetDocument::GetNeutralNormalMap(), tmp).GetData()).LogFailure();
      }
    }

    if (!bHasArmTexture)
    {
      if (!textureRoughness.IsEmpty())
      {
        pAccessor->SetValue(pMaterialProperties, "UseNormalAndRoughnessTexture", true).LogFailure();

        pAccessor->SetValue(pMaterialProperties, "RoughnessTexture", ezVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureRoughness, ezModelImporter2::TextureSemantic::RoughnessMap, false))).LogFailure();
      }
      else
      {
        pAccessor->SetValue(pMaterialProperties, "RoughnessTexture", "White.color").LogFailure();
      }
    }

    // Set metallic texture
    if (!bHasArmTexture)
    {
      if (!textureMetallic.IsEmpty())
      {
        pAccessor->SetValue(pMaterialProperties, "UseMetallicTexture", true).LogFailure();
        pAccessor->SetValue(pMaterialProperties, "MetallicTexture", ezVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureMetallic, ezModelImporter2::TextureSemantic::MetallicMap, false))).LogFailure();
      }
    }

    // Set emissive texture
    {
      ezString textureEmissive;

      if (material.m_TextureReferences.TryGetValue(ezModelImporter2::TextureSemantic::EmissiveMap, textureEmissive))
      {
        pAccessor->SetValue(pMaterialProperties, "UseEmissiveTexture", true).LogFailure();
        pAccessor->SetValue(pMaterialProperties, "EmissiveTexture", ezVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureEmissive, ezModelImporter2::TextureSemantic::EmissiveMap, false))).LogFailure();
      }
    }

    // Set AO texture
    if (!bHasArmTexture)
    {
      ezString textureAo;

      if (material.m_TextureReferences.TryGetValue(ezModelImporter2::TextureSemantic::AmbientMap, textureAo))
      {
        pAccessor->SetValue(pMaterialProperties, "UseOcclusionTexture", true).LogFailure();
        pAccessor->SetValue(pMaterialProperties, "OcclusionTexture", ezVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureAo, ezModelImporter2::TextureSemantic::AmbientMap, false))).LogFailure();
      }
    }

    // TODO: ambient occlusion texture

    // Set base color property
    if (material.m_Properties.TryGetValue(ezModelImporter2::PropertySemantic::DiffuseColor, propertyValue) && propertyValue.IsA<ezColor>())
    {
      pAccessor->SetValue(pMaterialProperties, "BaseColor", propertyValue).LogFailure();
    }

    // Set emissive color property
    if (material.m_Properties.TryGetValue(ezModelImporter2::PropertySemantic::EmissiveColor, propertyValue) && propertyValue.IsA<ezColor>())
    {
      pAccessor->SetValue(pMaterialProperties, "EmissiveColor", propertyValue).LogFailure();
    }

    // Set two-sided property
    if (material.m_Properties.TryGetValue(ezModelImporter2::PropertySemantic::TwosidedValue, propertyValue) && propertyValue.IsNumber())
    {
      pAccessor->SetValue(pMaterialProperties, "TWO_SIDED", propertyValue.ConvertTo<bool>()).LogFailure();
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

      pAccessor->SetValue(pMaterialProperties, "MetallicValue", value).LogFailure();
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

      pAccessor->SetValue(pMaterialProperties, "RoughnessValue", value).LogFailure();
    }

    // Set ARM Texture
    if (bHasArmTexture)
    {
      pAccessor->SetValue(pMaterialProperties, "UseAoRoughMetalTexture", true).LogFailure();
      pAccessor->SetValue(pMaterialProperties, "UseOcclusionTexture", true).LogFailure();
      pAccessor->SetValue(pMaterialProperties, "UseNormalAndRoughnessTexture", true).LogFailure();
      pAccessor->SetValue(pMaterialProperties, "UseMetallicTexture", true).LogFailure();

      pAccessor->SetValue(pMaterialProperties, "MetallicTexture", "").LogFailure();
      pAccessor->SetValue(pMaterialProperties, "OcclusionTexture", "").LogFailure();
      pAccessor->SetValue(pMaterialProperties, "RoughnessTexture", "").LogFailure();
      pAccessor->SetValue(pMaterialProperties, "RoughnessValue", 1.0f).LogFailure();
      pAccessor->SetValue(pMaterialProperties, "MetallicValue", 0.0f).LogFailure();

      pAccessor->SetValue(pMaterialProperties, "AoRoughMetalTexture", ezVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureRoughness, ezModelImporter2::TextureSemantic::AoRoughMetalMap, false))).LogFailure();
    }

    // Todo:
    // * Shading Mode
    // * Mask Threshold

    pAccessor->FinishTransaction();
  }

  void ImportMeshAssetMaterials(ezHybridArray<ezMaterialResourceSlot, 8>& inout_MaterialSlots, const char* szDocumentDirectory, const ezModelImporter2::Importer* pImporter)
  {
    EZ_PROFILE_SCOPE("ImportMeshAssetMaterials");

    const ezStringBuilder targetDirectory(szDocumentDirectory, "_data/");
    const ezStringBuilder sourceDirectory = ezPathUtils::GetFileDirectory(pImporter->GetImportOptions().m_sSourceFile);

    ezStringBuilder tmp;
    ezStringBuilder newResourcePathAbs;

    const ezUInt32 uiNumSubmeshes = inout_MaterialSlots.GetCount();

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

    for (const auto& impMaterial : pImporter->m_OutputMaterials)
    {
      if (impMaterial.m_iReferencedByMesh < 0)
        continue;

      const ezUInt32 subMeshIdx = impMaterial.m_iReferencedByMesh;

      range.BeginNextStep("Importing Material");

      // Didn't find currently set resource, create new imported material.
      if (!ezAssetCurator::GetSingleton()->FindSubAsset(inout_MaterialSlots[subMeshIdx].m_sResource))
      {
        // Check first if we already imported this material.
        if (importMatToGuid.TryGetValue(&impMaterial, inout_MaterialSlots[subMeshIdx].m_sResource))
          continue;

        // Put the new asset in the data folder.
        newResourcePathAbs = targetDirectory;
        newResourcePathAbs.AppendPath(impMaterial.m_sName);
        newResourcePathAbs.Append(".ezMaterialAsset");

        // Does the generated path already exist? Use it.
        if (const auto assetInfo = ezAssetCurator::GetSingleton()->FindSubAsset(newResourcePathAbs))
        {
          inout_MaterialSlots[subMeshIdx].m_sResource = ezConversionUtils::ToString(assetInfo->m_Data.m_Guid, tmp);
          continue;
        }

        ezMaterialAssetDocument* pMaterialDoc = ezDynamicCast<ezMaterialAssetDocument*>(ezQtEditorApp::GetSingleton()->CreateDocument(newResourcePathAbs, ezDocumentFlags::AsyncSave));
        if (!pMaterialDoc)
        {
          ezLog::Error("Failed to create new material '{0}'", impMaterial.m_sName);
          continue;
        }

        ImportMeshAssetMaterialProperties(pMaterialDoc, impMaterial, sourceDirectory, targetDirectory);
        inout_MaterialSlots[subMeshIdx].m_sResource = ezConversionUtils::ToString(pMaterialDoc->GetGuid(), tmp);

        pMaterialDoc->SaveDocumentAsync({});
        pendingSaveTasks.PushBack(pMaterialDoc);

        // we have to flush because materials create worlds in the engine process and there is a world limit of 64
        if (pendingSaveTasks.GetCount() >= 16)
          WaitForPendingTasks();
      }

      // If we have a material now, fill the mapping.
      // It is important to do this even for "old"/known materials since a mesh might have gotten a new slot that points to the same
      // material as previous slots.
      if (inout_MaterialSlots[subMeshIdx].m_sResource)
      {
        importMatToGuid.Insert(&impMaterial, inout_MaterialSlots[subMeshIdx].m_sResource);
      }
    }

    WaitForPendingTasks();
  }

} // namespace ezMeshImportUtils
