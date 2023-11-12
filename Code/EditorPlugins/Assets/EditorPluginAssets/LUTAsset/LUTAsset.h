#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetObjects.h>

class ezTextureAssetProfileConfig;

class ezLUTAssetDocument : public ezSimpleAssetDocument<ezLUTAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLUTAssetDocument, ezSimpleAssetDocument<ezLUTAssetProperties>);

public:
  ezLUTAssetDocument(ezStringView sDocumentPath);

protected:
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override
  {
    return ezStatus(EZ_SUCCESS);
  }
  virtual ezTransformStatus InternalTransformAsset(const char* szTargetFile, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;
};

//////////////////////////////////////////////////////////////////////////

class ezLUTAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLUTAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezLUTAssetDocumentGenerator();
  ~ezLUTAssetDocumentGenerator();

  virtual void GetImportModes(ezStringView sParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_modes) const override;
  virtual ezStatus Generate(ezStringView sDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument) override;
  virtual ezStringView GetDocumentExtension() const override { return "ezLUTAsset"; }
  virtual ezStringView GetGeneratorGroup() const override { return "LUTs"; }
};
