#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetObjects.h>

struct ezKrautTreeResourceDescriptor;
struct ezKrautGeneratorResourceDescriptor;

namespace ezModelImporter2
{
  enum class TextureSemantic : ezInt8;
}

class ezKrautTreeAssetDocument : public ezSimpleAssetDocument<ezKrautTreeAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezKrautTreeAssetDocument, ezSimpleAssetDocument<ezKrautTreeAssetProperties>);

public:
  ezKrautTreeAssetDocument(ezStringView sDocumentPath);

protected:
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  void SyncBackAssetProperties(ezKrautTreeAssetProperties*& pProp, const ezKrautGeneratorResourceDescriptor& desc);

  virtual ezTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};

//////////////////////////////////////////////////////////////////////////


class ezKrautTreeAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezKrautTreeAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezKrautTreeAssetDocumentGenerator();
  ~ezKrautTreeAssetDocumentGenerator();

  virtual void GetImportModes(ezStringView sParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_modes) const override;
  virtual ezStatus Generate(ezStringView sDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument) override;
  virtual ezStringView GetDocumentExtension() const override { return "ezKrautTreeAsset"; }
  virtual ezStringView GetGeneratorGroup() const override { return "KrautTrees"; }
};
