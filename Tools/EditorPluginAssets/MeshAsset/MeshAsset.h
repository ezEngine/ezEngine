#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>
#include <EditorPluginAssets/ModelImporter/Declarations.h>

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
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform, const ezAssetFileHeader& AssetHeader) override;

  void CreateMeshFromGeom(ezMeshAssetProperties* pProp, ezGeometry &geom, ezMeshResourceDescriptor &desc);
  ezStatus CreateMeshFromFile(ezMeshAssetProperties* pProp, ezMeshResourceDescriptor &desc, const ezMat3 &mTransformation);

  void ImportMaterials(const ezModelImporter::Scene& scene, const ezModelImporter::Mesh& mesh, ezMeshAssetProperties* pProp, const char* szImportSourceFolder, const char* szImportTargetFolder);
  void ImportMaterial(ezMaterialAssetDocument* materialDocument, const ezModelImporter::Material* material, const char* szImportSourceFolder, const char* szImportTargetFolder);
  ezString ImportOrResolveTexture(const char* szImportSourceFolder, const char* szImportTargetFolder, const char* szTexturePath, ezModelImporter::SemanticHint::Enum hint);

  virtual ezStatus InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader) override;

};
