#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Utilities/EnumerableClass.h>

class ezWorld;

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezEditorEngineSyncObject : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorEngineSyncObject, ezReflectedClass);

public:
  ezEditorEngineSyncObject();
  ~ezEditorEngineSyncObject();

  void Configure(ezUuid ownerGuid, ezDelegate<void(ezEditorEngineSyncObject*)> onDestruction);

  ezUuid GetDocumentGuid() const;
  void SetModified(bool b = true) { m_bModified = b; }
  bool GetModified() const { return m_bModified; }

  ezUuid GetGuid() const { return m_SyncObjectGuid; }

  virtual bool SetupForEngine(ezWorld* pWorld, ezUInt32 uiNextComponentPickingID) { return false; }
  virtual void UpdateForEngine(ezWorld* pWorld) {}

private:
  EZ_ALLOW_PRIVATE_PROPERTIES(ezEditorEngineSyncObject);

  friend class ezAssetDocument;

  bool m_bModified;
  ezUuid m_SyncObjectGuid;
  ezUuid m_OwnerGuid;

  ezDelegate<void(ezEditorEngineSyncObject*)> m_OnDestruction;
};
