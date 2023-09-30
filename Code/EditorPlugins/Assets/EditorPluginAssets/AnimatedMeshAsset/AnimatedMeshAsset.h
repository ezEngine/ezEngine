#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetObjects.h>

class ezMeshResourceDescriptor;
class ezMaterialAssetDocument;

class ezAnimatedMeshAssetDocument : public ezSimpleAssetDocument<ezAnimatedMeshAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimatedMeshAssetDocument, ezSimpleAssetDocument<ezAnimatedMeshAssetProperties>);

public:
  ezAnimatedMeshAssetDocument(ezStringView sDocumentPath);

protected:
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  ezStatus CreateMeshFromFile(ezAnimatedMeshAssetProperties* pProp, ezMeshResourceDescriptor& desc);


  virtual ezTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};

//////////////////////////////////////////////////////////////////////////

class ezAnimatedMeshAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimatedMeshAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezAnimatedMeshAssetDocumentGenerator();
  ~ezAnimatedMeshAssetDocumentGenerator();

  virtual void GetImportModes(ezStringView sParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_modes) const override;
  virtual ezStatus Generate(ezStringView sDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument) override;
  virtual ezStringView GetDocumentExtension() const override { return "ezAnimatedMeshAsset"; }
  virtual ezStringView GetGeneratorGroup() const override { return "Meshes"; }
};
