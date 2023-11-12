#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAssetObjects.h>

struct ezRmlUiResourceDescriptor;

class ezRmlUiAssetDocument : public ezSimpleAssetDocument<ezRmlUiAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRmlUiAssetDocument, ezSimpleAssetDocument<ezRmlUiAssetProperties>);

public:
  ezRmlUiAssetDocument(ezStringView sDocumentPath);

protected:
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  virtual ezTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};

//////////////////////////////////////////////////////////////////////////

class ezRmlUiAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRmlUiAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezRmlUiAssetDocumentGenerator();
  ~ezRmlUiAssetDocumentGenerator();

  virtual void GetImportModes(ezStringView sParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_modes) const override;
  virtual ezStatus Generate(ezStringView sDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument) override;
  virtual ezStringView GetDocumentExtension() const override { return "ezRmlUiAsset"; }
  virtual ezStringView GetGeneratorGroup() const override { return "RmlUis"; }
};
