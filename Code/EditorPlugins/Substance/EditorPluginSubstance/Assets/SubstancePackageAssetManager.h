#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class ezSubstancePackageAssetDocumentManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSubstancePackageAssetDocumentManager, ezAssetDocumentManager);

public:
  ezSubstancePackageAssetDocumentManager();
  ~ezSubstancePackageAssetDocumentManager();

  const ezAssetDocumentTypeDescriptor& GetTextureTypeDesc() const { return m_TextureTypeDesc; }

private:
  virtual void FillOutSubAssetList(const ezAssetDocumentInfo& assetInfo, ezDynamicArray<ezSubAssetData>& out_subAssets) const override;
  virtual ezString GetAssetTableEntry(const ezSubAsset* pSubAsset, ezStringView sDataDirectory, const ezPlatformProfile* pAssetProfile) const override;
  virtual ezUInt64 ComputeAssetProfileHashImpl(const ezPlatformProfile* pAssetProfile) const override { return 1; }

  void OnDocumentManagerEvent(const ezDocumentManager::Event& e);

  virtual void InternalCreateDocument(ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return true; }

  ezAssetDocumentTypeDescriptor m_PackageTypeDesc;
  ezAssetDocumentTypeDescriptor m_TextureTypeDesc;
};
