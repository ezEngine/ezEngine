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
  ezEditorEngineSyncObject() { m_SyncObjectGuid.CreateNewUuid(); m_bModified = true; }

  void SetDocumentGuid(const ezUuid& guid) { m_DocumentGuid = guid; }
  void SetModified(bool b = true) { m_bModified = b; }
  bool GetModified() const { return m_bModified; }

  static void SyncObjectsToEngine(ezEditorEngineConnection& connection, bool bSyncAll);

private:
  bool m_bModified;
  ezUuid m_SyncObjectGuid;
  ezUuid m_DocumentGuid;
};
