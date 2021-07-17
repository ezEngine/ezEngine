#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class ezMaterialAssetDocumentManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMaterialAssetDocumentManager, ezAssetDocumentManager);

public:
  ezMaterialAssetDocumentManager();
  ~ezMaterialAssetDocumentManager();

  virtual ezString GetRelativeOutputFileName(const ezAssetDocumentTypeDescriptor* pTypeDescriptor, const char* szDataDirectory,
    const char* szDocumentPath, const char* szOutputTag, const ezPlatformProfile* pAssetProfile) const override;
  virtual bool IsOutputUpToDate(
    const char* szDocumentPath, const char* szOutputTag, ezUInt64 uiHash, const ezAssetDocumentTypeDescriptor* pTypeDescriptor) override;

  static const char* const s_szShaderOutputTag;

private:
  void OnDocumentManagerEvent(const ezDocumentManager::Event& e);

  virtual void InternalCreateDocument(
    const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

private:
  ezAssetDocumentTypeDescriptor m_DocTypeDesc;
};
