#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class ezMaterialAssetDocumentManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMaterialAssetDocumentManager, ezAssetDocumentManager);

public:
  ezMaterialAssetDocumentManager();
  ~ezMaterialAssetDocumentManager();

  virtual ezString GetRelativeOutputFileName(const ezAssetDocumentTypeDescriptor* pTypeDescriptor, ezStringView sDataDirectory, ezStringView sDocumentPath, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile) const override;
  virtual bool IsOutputUpToDate(ezStringView sDocumentPath, ezStringView sOutputTag, ezUInt64 uiHash, const ezAssetDocumentTypeDescriptor* pTypeDescriptor) override;
  virtual ezStringView GetOutputDocumentType(const ezAssetDocumentTypeDescriptor* pTypeDesc, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile = nullptr) const override;

  static const char* const s_szShaderOutputTag;

private:
  void OnDocumentManagerEvent(const ezDocumentManager::Event& e);

  virtual void InternalCreateDocument(ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

private:
  ezAssetDocumentTypeDescriptor m_DocTypeDesc;
};
