#pragma once

#include <EditorFramework/Assets/AssetDocument.h>

class ezMaterialAssetProperties;

class ezMaterialAssetDocument : public ezAssetDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMaterialAssetDocument);

public:
  ezMaterialAssetDocument(const char* szDocumentPath);
  ~ezMaterialAssetDocument();

  const ezMaterialAssetProperties* GetProperties() const;

  virtual const char* GetDocumentTypeDisplayString() const override { return "Material Asset"; }

  virtual const char* QueryAssetType() const override;

protected:
  virtual void Initialize() override;
  virtual ezStatus InternalLoadDocument() override;
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) override;
  virtual ezStatus InternalTransformAsset(ezStreamWriterBase& stream, const char* szPlatform) override;

private:
  void EnsureSettingsObjectExist();

  virtual ezDocumentInfo* CreateDocumentInfo() override { return EZ_DEFAULT_NEW(ezAssetDocumentInfo); }
};
