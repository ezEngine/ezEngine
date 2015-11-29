#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <ToolsFoundation/Basics/Status.h>

class ezSurfaceAssetDocumentManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSurfaceAssetDocumentManager, ezAssetDocumentManager);

public:
  ezSurfaceAssetDocumentManager();
  ~ezSurfaceAssetDocumentManager();

  virtual ezString GetResourceTypeExtension() const override { return "ezSurface"; }

  virtual void QuerySupportedAssetTypes(ezSet<ezString>& inout_AssetTypeNames) const override
  {
    inout_AssetTypeNames.Insert("Surface");
  }

private:
  void OnDocumentManagerEvent(const ezDocumentManager::Event& e);

  virtual ezStatus InternalCanOpenDocument(const char* szDocumentTypeName, const char* szFilePath) const;
  virtual ezStatus InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument);
  virtual void InternalGetSupportedDocumentTypes(ezHybridArray<ezDocumentTypeDescriptor, 4>& out_DocumentTypes) const;

};

