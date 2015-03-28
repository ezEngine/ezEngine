#pragma once

#include <EditorFramework/Assets/AssetDocument.h>

class ezTextureAssetDocument : public ezAssetDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextureAssetDocument);

public:
  ezTextureAssetDocument(const char* szDocumentPath);
  ~ezTextureAssetDocument();

  virtual const char* GetDocumentTypeDisplayString() const override { return "Texture Asset"; }

private:
  virtual ezDocumentInfo* CreateDocumentInfo() override { return EZ_DEFAULT_NEW(ezAssetDocumentInfo); }
};
