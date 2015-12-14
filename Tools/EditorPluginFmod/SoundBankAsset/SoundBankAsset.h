#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAssetObjects.h>

class ezSoundBankAssetDocument : public ezSimpleAssetDocument<ezSoundBankAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSoundBankAssetDocument, ezSimpleAssetDocument<ezSoundBankAssetProperties>);

public:
  ezSoundBankAssetDocument(const char* szDocumentPath);

  virtual const char* GetDocumentTypeDisplayString() const override { return "Sound Bank Asset"; }

  virtual const char* QueryAssetType() const override { return "Sound Bank"; }

protected:
  virtual ezUInt16 GetAssetTypeVersion() const override { return 1; }
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) override;
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform) override;

  virtual ezStatus InternalRetrieveAssetInfo(const char* szPlatform) override;

};
