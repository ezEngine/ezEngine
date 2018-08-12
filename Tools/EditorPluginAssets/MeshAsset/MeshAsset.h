#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>
#include <ModelImporter/Declarations.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>

class ezMeshResourceDescriptor;
class ezGeometry;
class ezMaterialAssetDocument;

namespace ezModelImporter
{
  struct Material;
  class Mesh;
  class Scene;
  struct TextureReference;
}

class ezMeshAssetDocument : public ezSimpleAssetDocument<ezMeshAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshAssetDocument, ezSimpleAssetDocument<ezMeshAssetProperties>);

public:
  ezMeshAssetDocument(const char* szDocumentPath);

  virtual const char* QueryAssetType() const override { return "Mesh"; }

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override;

  void CreateMeshFromGeom(ezMeshAssetProperties* pProp, ezMeshResourceDescriptor& desc);
  ezStatus CreateMeshFromFile(ezMeshAssetProperties* pProp, ezMeshResourceDescriptor &desc);

  virtual ezStatus InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader) override;

};

//////////////////////////////////////////////////////////////////////////

class ezMeshAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezMeshAssetDocumentGenerator();
  ~ezMeshAssetDocumentGenerator();

  virtual void GetImportModes(const char* szParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual ezStatus Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument) override;
  virtual const char* GetDocumentExtension() const override { return "ezMeshAsset"; }
  virtual const char* GetGeneratorGroup() const override { return "Meshes"; }
};
