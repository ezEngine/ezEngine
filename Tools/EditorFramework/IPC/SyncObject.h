#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>

class ezQtEngineDocumentWindow;
class ezEngineProcessDocumentContext;

class EZ_EDITORFRAMEWORK_DLL ezEditorEngineSyncObject : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorEngineSyncObject, ezReflectedClass);

public:
  ezEditorEngineSyncObject();
  ~ezEditorEngineSyncObject();

  void SetOwner(ezQtEngineDocumentWindow* pOwner);
  void SetOwner(ezEngineProcessDocumentContext* pOwner);

  ezUuid GetDocumentGuid() const;
  void SetModified(bool b = true) { m_bModified = b; }
  bool GetModified() const { return m_bModified; }

  ezUuid GetGuid() const { return m_SyncObjectGuid; }

private:
  EZ_ALLOW_PRIVATE_PROPERTIES(ezEditorEngineSyncObject);

  friend class ezQtEngineDocumentWindow;

  bool m_bModified;
  ezUuid m_SyncObjectGuid;

  ezQtEngineDocumentWindow* m_pOwner;
  ezEngineProcessDocumentContext* m_pOwnerEngine;
};
