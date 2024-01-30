#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>

class ezMeshResourceDescriptor;
class ezGeometry;
class ezMaterialAssetDocument;

class ezMeshAssetDocument : public ezSimpleAssetDocument<ezMeshAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshAssetDocument, ezSimpleAssetDocument<ezMeshAssetProperties>);

public:
  ezMeshAssetDocument(ezStringView sDocumentPath);

protected:
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  void CreateMeshFromGeom(ezMeshAssetProperties* pProp, ezMeshResourceDescriptor& desc);
  ezTransformStatus CreateMeshFromFile(ezMeshAssetProperties* pProp, ezMeshResourceDescriptor& desc, bool bAllowMaterialImport);

  virtual ezTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;
};

//////////////////////////////////////////////////////////////////////////

class ezMeshAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezMeshAssetDocumentGenerator();
  ~ezMeshAssetDocumentGenerator();

  virtual void GetImportModes(ezStringView sAbsInputFile, ezDynamicArray<ezAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual ezStringView GetDocumentExtension() const override { return "ezMeshAsset"; }
  virtual ezStringView GetGeneratorGroup() const override { return "Meshes"; }
  virtual ezStatus Generate(ezStringView sInputFileAbs, ezStringView sMode, ezDynamicArray<ezDocument*>& out_generatedDocuments) override;
};
