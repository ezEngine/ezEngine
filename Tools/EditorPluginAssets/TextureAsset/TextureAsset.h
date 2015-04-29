#pragma once

#include <EditorFramework/Assets/AssetDocument.h>

class ezTextureAssetProperties;

class ezTextureAssetDocument : public ezAssetDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextureAssetDocument);

public:
  ezTextureAssetDocument(const char* szDocumentPath);
  ~ezTextureAssetDocument();

  const ezTextureAssetProperties* GetProperties() const;

  virtual const char* GetDocumentTypeDisplayString() const override { return "Texture Asset"; }

protected:
  virtual void Initialize() override;
  virtual ezStatus InternalLoadDocument() override;
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) override;
  virtual ezStatus InternalTransformAsset(ezStreamWriterBase& stream, const char* szPlatform) override;

private:
  void EnsureSettingsObjectExist();

  virtual ezDocumentInfo* CreateDocumentInfo() override { return EZ_DEFAULT_NEW(ezAssetDocumentInfo); }
};
