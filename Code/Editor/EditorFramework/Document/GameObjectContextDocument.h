#pragma once

#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/EditorFrameworkDLL.h>

struct EZ_EDITORFRAMEWORK_DLL ezGameObjectContextEvent
{
  enum class Type
  {
    ContextAboutToBeChanged,
    ContextChanged,
  };
  Type m_Type;
};

class EZ_EDITORFRAMEWORK_DLL ezGameObjectContextDocument : public ezGameObjectDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameObjectContextDocument, ezGameObjectDocument);

public:
  ezGameObjectContextDocument(const char* szDocumentPath, ezDocumentObjectManager* pObjectManager, ezAssetDocEngineConnection engineConnectionType = ezAssetDocEngineConnection::FullObjectMirroring);
  ~ezGameObjectContextDocument();

  ezStatus SetContext(ezUuid documentGuid, ezUuid objectGuid);
  ezUuid GetContextDocumentGuid() const;
  ezUuid GetContextObjectGuid() const;
  const ezDocumentObject* GetContextObject() const;

  mutable ezEvent<const ezGameObjectContextEvent&> m_GameObjectContextEvents;

protected:
  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;

private:
  void ClearContext();

private:
  ezUuid m_ContextDocument;
  ezUuid m_ContextObject;
};
