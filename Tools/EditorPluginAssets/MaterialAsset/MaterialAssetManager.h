#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <ToolsFoundation/Basics/Status.h>

class ezMaterialAssetDocumentManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMaterialAssetDocumentManager);

public:
  ezMaterialAssetDocumentManager();
  ~ezMaterialAssetDocumentManager();

  virtual ezString GetResourceTypeExtension() const override { return "ezMaterialBin"; }

  virtual void QuerySupportedAssetTypes(ezSet<ezString>& inout_AssetTypeNames) const override
  {
    inout_AssetTypeNames.Insert("Material");
  }

private:
  void OnDocumentManagerEvent(const ezDocumentManagerBase::Event& e);

  virtual ezStatus InternalCanOpenDocument(const char* szDocumentTypeName, const char* szFilePath) const;
  virtual ezStatus InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocumentBase*& out_pDocument);
  virtual void InternalGetSupportedDocumentTypes(ezHybridArray<ezDocumentTypeDescriptor, 4>& out_DocumentTypes) const;

};

