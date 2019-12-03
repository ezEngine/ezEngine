#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>

class ezUltralightHTMLAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezUltralightHTMLAssetProperties, ezReflectedClass);

public:
  ezUltralightHTMLAssetProperties() {}

  ezString m_sHTMLContent;
  ezString m_sHTMLFile;

  ezUInt16 m_uiWidth = 512;
  ezUInt16 m_uiHeight = 512;

  bool m_bTransparentBackground = false;
};

class ezUltralightHTMLAssetDocument : public ezSimpleAssetDocument<ezUltralightHTMLAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezUltralightHTMLAssetDocument, ezSimpleAssetDocument<ezUltralightHTMLAssetProperties>);

public:
  ezUltralightHTMLAssetDocument(const char* szDocumentPath);

  virtual const char* GetDocumentTypeDisplayString() const override { return "Ultralight HTML Asset"; }

  virtual const char* QueryAssetType() const override { return "HTML Texture"; }

protected:
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

};
