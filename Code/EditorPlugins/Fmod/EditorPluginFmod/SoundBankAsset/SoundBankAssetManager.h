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

  virtual void FillOutSubAssetList(const ezAssetDocumentInfo& assetInfo, ezHybridArray<ezSubAssetData, 4>& out_SubAssets) const override;
  virtual ezString GetAssetTableEntry(const ezSubAsset* pSubAsset, const char* szDataDirectory,
                                      const ezPlatformProfile* pAssetProfile) const override;

private:
  void OnDocumentManagerEvent(const ezDocumentManager::Event& e);
  ezString GetSoundBankAssetTableEntry(const ezSubAsset* pSubAsset, const char* szDataDirectory, const ezPlatformProfile* pAssetProfile) const;

  virtual void InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument) override;
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return true; }

  virtual ezUInt64 ComputeAssetProfileHashImpl(const ezPlatformProfile* pAssetProfile) const override;

  ezAssetDocumentTypeDescriptor m_DocTypeDesc;
  ezUniquePtr<ezSimpleFmod> m_Fmod;
};
