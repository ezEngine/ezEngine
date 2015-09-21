#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>

class ezMeshResourceDescriptor;
class ezGeometry;

class ezMeshAssetDocument : public ezSimpleAssetDocument<ezMeshAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshAssetDocument);

public:
  ezMeshAssetDocument(const char* szDocumentPath);

  virtual const char* GetDocumentTypeDisplayString() const override { return "Mesh Asset"; }

  virtual const char* QueryAssetType() const override { return "Mesh"; }

protected:
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) override;
  virtual ezStatus InternalTransformAsset(ezStreamWriterBase& stream, const char* szPlatform) override;

  void CreateMeshFromGeom(const ezMeshAssetProperties* pProp, ezGeometry &geom, const bool bFlipTriangles, ezMeshResourceDescriptor &desc);

  ezStatus CreateMeshFromFile(const ezMeshAssetProperties* pProp, ezMeshResourceDescriptor &desc, bool bFlipTriangles, const ezMat3 &mTransformation);

  virtual ezStatus InternalRetrieveAssetInfo(const char* szPlatform) override;

};
