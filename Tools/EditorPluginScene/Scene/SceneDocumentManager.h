#pragma once

#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Basics/Status.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>

class ezSceneDocumentManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneDocumentManager, ezAssetDocumentManager);

public:
  ezSceneDocumentManager()
  {
    s_pSingleton = this;
  }

  static ezSceneDocumentManager* s_pSingleton;

private:
  virtual ezStatus InternalCanOpenDocument(const char* szDocumentTypeName, const char* szFilePath) const;
  virtual ezStatus InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument);
  virtual void InternalGetSupportedDocumentTypes(ezHybridArray<ezDocumentTypeDescriptor, 4>& out_DocumentTypes) const;

  virtual ezString GetResourceTypeExtension() const override;

  virtual void QuerySupportedAssetTypes(ezSet<ezString>& inout_AssetTypeNames) const override;



};