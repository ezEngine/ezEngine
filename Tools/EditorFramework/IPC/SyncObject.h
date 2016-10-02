#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>

class ezAssetDocument;
class ezEngineProcessDocumentContext;
class ezWorld;

class EZ_EDITORFRAMEWORK_DLL ezEditorEngineSyncObject : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorEngineSyncObject, ezReflectedClass);

public:
  ezEditorEngineSyncObject();
  ~ezEditorEngineSyncObject();

  void SetOwner(const ezAssetDocument* pOwner);
  void SetOwner(ezEngineProcessDocumentContext* pOwner);

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

  const ezAssetDocument* m_pOwner;
  ezEngineProcessDocumentContext* m_pOwnerEngine;
};
