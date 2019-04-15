#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class ezProceduralPlacementAssetDocumentManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProceduralPlacementAssetDocumentManager, ezAssetDocumentManager);

public:
  ezProceduralPlacementAssetDocumentManager();
  ~ezProceduralPlacementAssetDocumentManager();

  virtual ezString GetResourceTypeExtension(const char* szDocumentPath) const override { return "ezProceduralPlacement"; }

  virtual void QuerySupportedAssetTypes(ezSet<ezString>& inout_AssetTypeNames) const override
  {
    inout_AssetTypeNames.Insert("Procedural Placement");
  }
  
  virtual ezBitflags<ezAssetDocumentFlags> GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const override;

private:
  void OnDocumentManagerEvent(const ezDocumentManager::Event& e);

  virtual ezStatus InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument,
                                          ezDocument*& out_pDocument) override;
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

private:
  ezDocumentTypeDescriptor m_AssetDesc;
};

