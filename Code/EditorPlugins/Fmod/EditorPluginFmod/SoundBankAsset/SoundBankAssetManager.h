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

  virtual OutputReliability GetAssetTypeOutputReliability() const override { return ezAssetDocumentManager::OutputReliability::Perfect; }

  virtual void FillOutSubAssetList(const ezAssetDocumentInfo& assetInfo, ezDynamicArray<ezSubAssetData>& out_subAssets) const override;
  virtual ezString GetAssetTableEntry(const ezSubAsset* pSubAsset, ezStringView sDataDirectory, const ezPlatformProfile* pAssetProfile) const override;

private:
  void OnDocumentManagerEvent(const ezDocumentManager::Event& e);
  ezString GetSoundBankAssetTableEntry(const ezSubAsset* pSubAsset, ezStringView sDataDirectory, const ezPlatformProfile* pAssetProfile) const;

  virtual void InternalCreateDocument(ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return true; }

  virtual ezUInt64 ComputeAssetProfileHashImpl(const ezPlatformProfile* pAssetProfile) const override;

  ezAssetDocumentTypeDescriptor m_DocTypeDesc;
  ezUniquePtr<ezSimpleFmod> m_pFmod;

  struct SoundBankCache
  {
    ezTimestamp m_LastModification;
    ezDynamicArray<ezSubAssetData> m_CachedSubAssets;
  };

  mutable ezMap<ezUuid, SoundBankCache> m_Cache;
};
