#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class ezDecalAssetDocumentManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDecalAssetDocumentManager, ezAssetDocumentManager);

public:
  ezDecalAssetDocumentManager();
  ~ezDecalAssetDocumentManager();

  virtual ezString GetResourceTypeExtension(const char* szDocumentPath) const override { return "ezDecalStub"; }

  virtual void QuerySupportedAssetTypes(ezSet<ezString>& inout_AssetTypeNames) const override
  {
    inout_AssetTypeNames.Insert("Decal");
  }

  virtual ezBitflags<ezAssetDocumentFlags> GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const override;

  virtual void AddEntriesToAssetTable(const char* szDataDirectory, const ezPlatformProfile* pAssetProfile,
                                      ezMap<ezString, ezString>& inout_GuidToPath) const override;
  virtual ezString GetAssetTableEntry(const ezSubAsset* pSubAsset, const char* szDataDirectory, const ezPlatformProfile* pAssetProfile) const override;

  /// \brief There is only a single decal texture per project. This function creates it, in case any decal asset was modified.
  ezStatus GenerateDecalTexture(const ezPlatformProfile* pAssetProfile);
  ezString GetDecalTexturePath(const ezPlatformProfile* pAssetProfile) const;

private:
  void OnDocumentManagerEvent(const ezDocumentManager::Event& e);
  bool IsDecalTextureUpToDate(const char* szDecalFile, ezUInt64 uiSettingsHash) const;
  ezStatus RunTexConv(const char* szTargetFile, const char* szInputFile, const ezAssetFileHeader& AssetHeader);

  virtual ezStatus InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument,
                                          ezDocument*& out_pDocument) override;
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return true; }

  virtual ezUInt64 ComputeAssetProfileHashImpl(const ezPlatformProfile* pAssetProfile) const override;

private:
  ezDocumentTypeDescriptor m_AssetDesc;
};

