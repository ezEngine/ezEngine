#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetObjects.h>
#include <ModelImporter/Declarations.h>

struct ezKrautTreeResourceDescriptor;

class ezKrautTreeAssetDocument : public ezSimpleAssetDocument<ezKrautTreeAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezKrautTreeAssetDocument, ezSimpleAssetDocument<ezKrautTreeAssetProperties>);

public:
  ezKrautTreeAssetDocument(const char* szDocumentPath);

  virtual const char* QueryAssetType() const override;

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
                                          const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  void SyncBackAssetProperties(ezKrautTreeAssetProperties*& pProp, ezKrautTreeResourceDescriptor& desc);

  virtual ezStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  ezString ImportTexture(const char* szFilename, ezModelImporter::SemanticHint::Enum hint, bool bTextureClamp);
};

//////////////////////////////////////////////////////////////////////////


class ezKrautTreeAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezKrautTreeAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezKrautTreeAssetDocumentGenerator();
  ~ezKrautTreeAssetDocumentGenerator();

  virtual void GetImportModes(const char* szParentDirRelativePath,
                              ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual ezStatus Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info,
                            ezDocument*& out_pGeneratedDocument) override;
  virtual const char* GetDocumentExtension() const override { return "ezKrautTreeAsset"; }
  virtual const char* GetGeneratorGroup() const override { return "KrautTrees"; }
};
