#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAssetObjects.h>

struct ezRmlUiResourceDescriptor;

class ezRmlUiAssetDocument : public ezSimpleAssetDocument<ezRmlUiAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRmlUiAssetDocument, ezSimpleAssetDocument<ezRmlUiAssetProperties>);

public:
  ezRmlUiAssetDocument(const char* szDocumentPath);

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  virtual ezStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};

//////////////////////////////////////////////////////////////////////////

class ezRmlUiAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRmlUiAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezRmlUiAssetDocumentGenerator();
  ~ezRmlUiAssetDocumentGenerator();

  virtual void GetImportModes(const char* szParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual ezStatus Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument) override;
  virtual const char* GetDocumentExtension() const override { return "ezRmlUiAsset"; }
  virtual const char* GetGeneratorGroup() const override { return "RmlUis"; }
};
