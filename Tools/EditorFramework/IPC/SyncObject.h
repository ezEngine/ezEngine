#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>

class EZ_EDITORFRAMEWORK_DLL ezEditorEngineSyncObject : public ezEnumerable<ezEditorEngineSyncObject, ezReflectedClass>
{
  EZ_DECLARE_ENUMERABLE_CLASS_WITH_BASE(ezEditorEngineSyncObject, ezReflectedClass);
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorEngineSyncObject);

public:
  ezEditorEngineSyncObject();
  ~ezEditorEngineSyncObject();

  void ChangeObjectGuid(const ezUuid& guid);
  void SetDocumentGuid(const ezUuid& guid) { m_DocumentGuid = guid; }
  ezUuid GetDocumentGuid() const { return m_DocumentGuid; }
  void SetModified(bool b = true) { m_bModified = b; }
  bool GetModified() const { return m_bModified; }

  ezUuid GetGuid() const { return m_SyncObjectGuid; }

  static void SyncObjectsToEngine(ezEditorEngineConnection& connection, bool bSyncAll);

  static ezEditorEngineSyncObject* FindSyncObject(const ezUuid& guid);

private:
  bool m_bModified;
  ezUuid m_SyncObjectGuid;
  ezUuid m_DocumentGuid;

  static ezHashTable<ezUuid, ezEditorEngineSyncObject*> s_AllSyncObjects;
  static ezMap<ezUuid, ezHybridArray<ezUuid, 32> > s_DeletedObjects;
};
