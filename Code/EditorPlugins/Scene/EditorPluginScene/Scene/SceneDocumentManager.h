#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Document/DocumentManager.h>

class ezSceneDocumentManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneDocumentManager, ezAssetDocumentManager);

public:
  ezSceneDocumentManager();

private:
  virtual void InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const override;
  virtual void InternalCloneDocument(const char* szPath, const char* szClonePath, const ezUuid& documentId, const ezUuid& seedGuid, const ezUuid& cloneGuid, ezAbstractObjectGraph* pHeader, ezAbstractObjectGraph* pObjects, ezAbstractObjectGraph* pTypes);

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

  void SetupDefaultScene(ezDocument* pDocument);


  ezStaticArray<ezAssetDocumentTypeDescriptor, 4> m_DocTypeDescs;
};
