#pragma once

#include <EditorFramework/Plugin.h>
#include <Core/World/World.h>
#include <Foundation/Types/Uuid.h>

class ezEditorEngineSyncObjectMsg;
class ezEditorEngineSyncObject;
class ezEditorEngineDocumentMsg;
class ezEngineProcessViewContext;
class ezProcessCommunication;
class ezProcessMessage;
class ezEntityMsgToEngine;

template<typename HandleType>
class ezEditorGuidEngineHandleMap
{
public:
  void RegisterObject(ezUuid guid, HandleType handle)
  {
    m_GuidToHandle[guid] = handle;
    m_HandleToGuid[handle] = guid;
  }

  void UnregisterObject(ezUuid guid)
  {
    const HandleType handle = m_GuidToHandle[guid];
    m_GuidToHandle.Remove(guid);
    m_HandleToGuid.Remove(handle);
  }

  void UnregisterObject(HandleType handle)
  {
    const ezUuid guid = m_HandleToGuid[handle];
    m_GuidToHandle.Remove(guid);
    m_HandleToGuid.Remove(handle);
  }

  HandleType GetHandle(ezUuid guid) const
  {
    HandleType res;
    m_GuidToHandle.TryGetValue(guid, res);
    return res;
  }

  ezUuid GetGuid(HandleType handle) const
  {
    auto it = m_HandleToGuid.Find(handle);
    if (it.IsValid())
      return it.Value();
    return ezUuid();
  }

private:
  ezHashTable<ezUuid, HandleType> m_GuidToHandle;
  ezMap<HandleType, ezUuid> m_HandleToGuid;
};

/// \brief A document context is the counter part to an editor document on the engine side.
///
/// For every document in the editor that requires engine output (rendering, picking, etc.), there is a ezEngineProcessDocumentContext
/// created in the engine process.
class EZ_EDITORFRAMEWORK_DLL ezEngineProcessDocumentContext : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEngineProcessDocumentContext);

public:
  ezEngineProcessDocumentContext();
  virtual ~ezEngineProcessDocumentContext();

  void Initialize(const ezUuid& DocumentGuid, ezProcessCommunication* pIPC);
  void Deinitialize();

  void SendProcessMessage(ezProcessMessage* pMsg, bool bSuperHighPriority = false);
  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg);

  static ezEngineProcessDocumentContext* GetDocumentContext(ezUuid guid);
  static void AddDocumentContext(ezUuid guid, ezEngineProcessDocumentContext* pView, ezProcessCommunication* pIPC);
  static void DestroyDocumentContext(ezUuid guid);

  void ProcessEditorEngineSyncObjectMsg(const ezEditorEngineSyncObjectMsg& msg);

  const ezUuid& GetDocumentGuid() const { return m_DocumentGuid; }

  

  ezWorld* m_pWorld;
  ezEditorGuidEngineHandleMap<ezUInt32> m_OtherPickingMap;
  ezEditorGuidEngineHandleMap<ezUInt32> m_ComponentPickingMap;
  ezEditorGuidEngineHandleMap<ezGameObjectHandle> m_GameObjectMap;
  ezEditorGuidEngineHandleMap<ezComponentHandle> m_ComponentMap;

protected:
  virtual void OnInitialize() {}
  virtual void OnDeinitialize() {}
  virtual ezEngineProcessViewContext* CreateViewContext() = 0;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) = 0;
  
  void HandlerEntityMsg(const ezEntityMsgToEngine* pMsg);
  void UpdateProperties(const ezEntityMsgToEngine* pMsg, void* pObject, const ezRTTI* pRtti);
  void HandlerGameObjectMsg(const ezEntityMsgToEngine* pMsg, ezRTTI* pRtti);
  void HandleComponentMsg(const ezEntityMsgToEngine* pMsg, ezRTTI* pRtti);
  void UpdateSyncObjects();

private:
  friend class ezEditorEngineSyncObject;

  void AddSyncObject(ezEditorEngineSyncObject* pSync);
  void RemoveSyncObject(ezEditorEngineSyncObject* pSync);
  ezEditorEngineSyncObject* FindSyncObject(const ezUuid& guid);


private:
  void ClearViewContexts();


  // Maps a document guid to the corresponding context that handles that document on the engine side
  static ezHashTable<ezUuid, ezEngineProcessDocumentContext*> s_DocumentContexts;

  /// Removes all sync objects that are tied to this context
  void CleanUpContextSyncObjects();

  ezUuid m_DocumentGuid;

  ezProcessCommunication* m_pIPC;
  ezHybridArray<ezEngineProcessViewContext*, 4> m_ViewContexts;

  ezMap<ezUuid, ezEditorEngineSyncObject*> m_SyncObjects;
  
  ezUInt32 m_uiNextComponentPickingID;
};

