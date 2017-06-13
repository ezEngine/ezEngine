#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class ezDecalAssetDocumentManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDecalAssetDocumentManager, ezAssetDocumentManager);

public:
  ezDecalAssetDocumentManager();
  ~ezDecalAssetDocumentManager();

  virtual ezString GetResourceTypeExtension() const override { return "ezDecalStub"; }

  virtual void QuerySupportedAssetTypes(ezSet<ezString>& inout_AssetTypeNames) const override
  {
    inout_AssetTypeNames.Insert("Decal");
  }

  virtual ezBitflags<ezAssetDocumentFlags> GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const override;


  virtual ezString GetAssetTableEntry(const ezSubAsset* pSubAsset, const char* szDataDirectory, const char* szPlatform) const override;

  /// \brief There is only a single decal texture per project. This function creates it, in case any decal asset was modified.
  ezStatus GenerateDecalTexture(const char* szPlatform);
  ezString GetDecalTexturePath(const char* szPlatform) const;

private:
  void OnDocumentManagerEvent(const ezDocumentManager::Event& e);
  bool IsDecalTextureUpToDate(const char* szDecalFile, ezUInt64 uiSettingsHash) const;
  ezStatus RunTexConv(const char* szTargetFile, const char* szInputFile, const ezAssetFileHeader& AssetHeader);

  virtual ezStatus InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument) override;
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesPlatformSpecificAssets() const override { return true; }

private:
  ezDocumentTypeDescriptor m_AssetDesc;
};

