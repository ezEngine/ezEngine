#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>

class ezMeshResourceDescriptor;
class ezGeometry;
class ezMaterialAssetDocument;

namespace ezModelImporter2
{
  class Importer;
}

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

protected:
  ezMeshAssetDocumentGenerator(bool bAnimMesh);
  virtual ezStatus ConfigureMeshDocument(ezDocument* pDoc, ezStringView sInputFile, ezStringView sOutFile, ezModelImporter2::Importer* pImporter, ezArrayPtr<ezMaterialResourceSlot> materials, ezDynamicArray<ezDocument*>& out_generatedDocuments);

  bool m_bAnimatedMesh = false;
  bool m_bShowImportDlg = true;
  static bool s_bReuseSkeleton;
  static bool s_bImportAllClips;
  static bool s_bUseSharedMaterials;
  static bool s_bCreateMaterials;
  static ezUuid s_SharedSkeleton;
};

class ezAnimatedMeshAssetDocumentGenerator : public ezMeshAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimatedMeshAssetDocumentGenerator, ezMeshAssetDocumentGenerator);

public:
  ezAnimatedMeshAssetDocumentGenerator();
  ~ezAnimatedMeshAssetDocumentGenerator();

  virtual void GetImportModes(ezStringView sAbsInputFile, ezDynamicArray<ezAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual ezStringView GetDocumentExtension() const override { return "ezAnimatedMeshAsset"; }

protected:
  virtual ezStatus ConfigureMeshDocument(ezDocument* pDoc, ezStringView sInputFile, ezStringView sOutFile, ezModelImporter2::Importer* pImporter, ezArrayPtr<ezMaterialResourceSlot> materials, ezDynamicArray<ezDocument*>& out_generatedDocuments) override;
};
