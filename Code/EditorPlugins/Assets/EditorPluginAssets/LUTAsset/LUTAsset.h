#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetObjects.h>

class ezTextureAssetProfileConfig;

class ezLUTAssetDocument : public ezSimpleAssetDocument<ezLUTAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLUTAssetDocument, ezSimpleAssetDocument<ezLUTAssetProperties>);

public:
  ezLUTAssetDocument(const char* szDocumentPath);

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override
  {
    return ezStatus(EZ_SUCCESS);
  }
  virtual ezStatus InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;
};

//////////////////////////////////////////////////////////////////////////

class ezLUTAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLUTAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezLUTAssetDocumentGenerator();
  ~ezLUTAssetDocumentGenerator();

  virtual void GetImportModes(const char* szParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual ezStatus Generate(
    const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument) override;
  virtual const char* GetDocumentExtension() const override { return "ezLUTAsset"; }
  virtual const char* GetGeneratorGroup() const override { return "LUTs"; }
};
