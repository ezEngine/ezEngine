#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>

class ezAiControllerAssetManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiControllerAssetManager, ezAssetDocumentManager);

public:
  ezAiControllerAssetManager();
  ~ezAiControllerAssetManager();

  virtual OutputReliability GetAssetTypeOutputReliability() const override { return ezAssetDocumentManager::OutputReliability::Perfect; }

private:
  void OnDocumentManagerEvent(const ezDocumentManager::Event& e);

  virtual void InternalCreateDocument(ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

  ezAssetDocumentTypeDescriptor m_DocTypeDesc;
};
