#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>

class ezMeshResourceDescriptor;
class ezGeometry;

namespace ezModelImporter
{
  class Mesh;
  class Scene;
}

class ezMeshAssetDocument : public ezSimpleAssetDocument<ezMeshAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshAssetDocument, ezSimpleAssetDocument<ezMeshAssetProperties>);

public:
  ezMeshAssetDocument(const char* szDocumentPath);

  virtual const char* QueryAssetType() const override { return "Mesh"; }

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform, const ezAssetFileHeader& AssetHeader) override;

  void CreateMeshFromGeom(const ezMeshAssetProperties* pProp, ezGeometry &geom, ezMeshResourceDescriptor &desc);

  ezStatus CreateMeshFromFile(ezMeshAssetProperties* pProp, ezMeshResourceDescriptor &desc, const ezMat3 &mTransformation);

  /// Assigns and optionally imports materials.
  /// Used for both InternalRetrieveAssetInfo and InternalTransformAsset.
  /// desc is optional.
  void ProcessMaterials(ezMeshAssetProperties* pProp, ezMeshResourceDescriptor *desc, 
                        const ezModelImporter::Mesh& mesh, const ezModelImporter::Scene& scene);

  virtual ezStatus InternalRetrieveAssetInfo(const char* szPlatform) override;
  virtual ezStatus InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader) override;

};
