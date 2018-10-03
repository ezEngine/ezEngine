#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class ezSimpleFmod;

class ezSoundBankAssetDocumentManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSoundBankAssetDocumentManager, ezAssetDocumentManager);

public:
  ezSoundBankAssetDocumentManager();
  ~ezSoundBankAssetDocumentManager();

  virtual ezString GetResourceTypeExtension() const override { return "ezFmodSoundBank"; }

  virtual void QuerySupportedAssetTypes(ezSet<ezString>& inout_AssetTypeNames) const override
  {
    inout_AssetTypeNames.Insert("Sound Bank");
  }

  virtual void FillOutSubAssetList(const ezAssetDocumentInfo& assetInfo, ezHybridArray<ezSubAssetData, 4>& out_SubAssets) const override;
  virtual ezString GetAssetTableEntry(const ezSubAsset* pSubAsset, const char* szDataDirectory, const ezAssetPlatformConfig* pPlatformConfig) const override;

private:
  void OnDocumentManagerEvent(const ezDocumentManager::Event& e);
  ezString GetSoundBankAssetTableEntry(const ezSubAsset* pSubAsset, const char* szDataDirectory, const ezAssetPlatformConfig* pPlatformConfig) const;

  virtual ezStatus InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument,
                                          ezDocument*& out_pDocument) override;
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesPlatformSpecificAssets() const override { return true; }

private:
  ezDocumentTypeDescriptor m_AssetDesc;
  ezUniquePtr<ezSimpleFmod> m_Fmod;
};

