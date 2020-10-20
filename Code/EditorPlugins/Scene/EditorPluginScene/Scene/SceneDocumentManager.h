#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Document/DocumentManager.h>

class ezSceneDocumentManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneDocumentManager, ezAssetDocumentManager);

public:
  ezSceneDocumentManager();

  static ezSceneDocumentManager* s_pSingleton;

private:
  virtual void InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument) override;
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

  void SetupDefaultScene(ezDocument* pDocument);

  ezAssetDocumentTypeDescriptor m_DocTypeDesc;
  ezAssetDocumentTypeDescriptor m_DocTypeDesc2;
};
