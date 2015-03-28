#pragma once

#include <EditorFramework/Plugin.h>
#include <ToolsFoundation/Document/Document.h>

class EZ_EDITORFRAMEWORK_DLL ezAssetDocumentInfo : public ezDocumentInfo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetDocumentInfo);

public:
  ezHybridArray<ezString, 16> m_FileDependencies;   ///< Files that are required to generate the asset, ie. if one changes, the asset needs to be recreated
  ezHybridArray<ezString, 16> m_FileReferences;     ///< Other files that are used at runtime together with this asset, e.g. materials for a mesh
};

class EZ_EDITORFRAMEWORK_DLL ezAssetDocument : public ezDocumentBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetDocument);

public:
  ezAssetDocument(const char* szDocumentPath, ezDocumentObjectManagerBase* pObjectManager);
  ~ezAssetDocument();

private:
  virtual ezDocumentInfo* CreateDocumentInfo() override;

};

