#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>

class ezTextureAssetDocument : public ezSimpleAssetDocument<ezTextureAssetProperties, ezTextureAssetObject, ezTextureAssetObjectManager>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextureAssetDocument);

public:
  ezTextureAssetDocument(const char* szDocumentPath);

  virtual const char* GetDocumentTypeDisplayString() const override { return "Texture Asset"; }

  virtual const char* QueryAssetType() const override;

protected:
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) override;
  virtual ezStatus InternalTransformAsset(ezStreamWriterBase& stream, const char* szPlatform) override;

};
