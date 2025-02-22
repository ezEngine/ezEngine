#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>

class ezAngelScriptAssetManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAngelScriptAssetManager, ezAssetDocumentManager);

public:
  ezAngelScriptAssetManager();
  ~ezAngelScriptAssetManager();

private:
  void OnDocumentManagerEvent(const ezDocumentManager::Event& e);

  virtual void InternalCreateDocument(
    ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

  ezAssetDocumentTypeDescriptor m_DocTypeDesc;
};
