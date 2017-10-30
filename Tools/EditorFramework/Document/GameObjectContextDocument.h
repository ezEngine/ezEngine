#pragma once
#include <EditorFramework/Plugin.h>
#include <EditorFramework/Document/GameObjectDocument.h>

class EZ_EDITORFRAMEWORK_DLL ezGameObjectContextDocument : public ezGameObjectDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameObjectContextDocument, ezGameObjectDocument);
public:
  ezGameObjectContextDocument(const char* szDocumentPath, ezDocumentObjectManager* pObjectManager, bool bUseEngineConnection = true, bool bUseIPCObjectMirror = true);
  ~ezGameObjectContextDocument();

  ezStatus SetContext(ezUuid documentGuid, ezUuid objectGuid);
  ezUuid GetContextDocument() const;
  ezUuid GetContextObject() const;

private:
  void ClearContext();

private:
  ezUuid m_ContextDocument;
  ezUuid m_ContextObject;
};
