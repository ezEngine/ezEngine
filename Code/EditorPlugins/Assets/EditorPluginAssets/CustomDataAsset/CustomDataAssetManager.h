#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class ezCustomDataAssetDocumentManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCustomDataAssetDocumentManager, ezAssetDocumentManager);

public:
  ezCustomDataAssetDocumentManager();
  ~ezCustomDataAssetDocumentManager();

  virtual OutputReliability GetAssetTypeOutputReliability() const override
  {
    // CustomData structs are typically defined in plugins, which may have changed, so they are a candidate for clearing them from the asset cache
    return ezAssetDocumentManager::OutputReliability::Unknown;
  }

private:
  void OnDocumentManagerEvent(const ezDocumentManager::Event& e);

  virtual void InternalCreateDocument(ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

private:
  ezAssetDocumentTypeDescriptor m_DocTypeDesc;
};
