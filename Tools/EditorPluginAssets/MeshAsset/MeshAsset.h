#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>

class ezMeshAssetDocument : public ezSimpleAssetDocument<ezMeshAssetProperties, ezMeshAssetObject, ezMeshAssetObjectManager>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshAssetDocument);

public:
  ezMeshAssetDocument(const char* szDocumentPath);

  virtual const char* GetDocumentTypeDisplayString() const override { return "Mesh Asset"; }

  virtual const char* QueryAssetType() const override { return "Mesh"; }

protected:
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) override;
  virtual ezStatus InternalTransformAsset(ezStreamWriterBase& stream, const char* szPlatform) override;

};
