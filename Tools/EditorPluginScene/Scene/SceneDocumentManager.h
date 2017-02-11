#pragma once

#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Basics/Status.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>

class ezSceneDocumentManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneDocumentManager, ezAssetDocumentManager);

public:
  ezSceneDocumentManager();

  static ezSceneDocumentManager* s_pSingleton;


  virtual ezBitflags<ezAssetDocumentFlags> GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const override;

private:
  virtual ezStatus InternalCanOpenDocument(const char* szDocumentTypeName, const char* szFilePath) const override;
  virtual ezStatus InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument) override;
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual ezString GetResourceTypeExtension() const override;

  virtual void QuerySupportedAssetTypes(ezSet<ezString>& inout_AssetTypeNames) const override;

  virtual bool GeneratesPlatformSpecificAssets() const override { return false; }

private:
  ezDocumentTypeDescriptor m_SceneDesc;
  ezDocumentTypeDescriptor m_PrefabDesc;
};