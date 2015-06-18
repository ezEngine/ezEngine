#pragma once

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <QApplication>

#include <EditorFramework/IPC/ProcessCommunication.h>
#include <EditorEngineProcess/ViewContext.h>

#include <GameFoundation/GameApplication.h>
#include <Core/World/GameObject.h>
#include <Core/World/Component.h>

class ezEngineProcessDocumentContext;

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

class ezEngineProcessGameState : public ezGameStateBase
{
public:
  ezEngineProcessGameState();
  void EventHandlerIPC(const ezProcessCommunication::Event& e);

  static ezEngineProcessGameState* GetInstance() { return s_pInstance; }

  ezProcessCommunication& ProcessCommunication() { return m_IPC; }

  ezEditorGuidEngineHandleMap<ezGameObjectHandle> m_GameObjectMap;
  ezEditorGuidEngineHandleMap<ezComponentHandle> m_ComponentMap;
  ezEditorGuidEngineHandleMap<ezUInt32> m_ComponentPickingMap;
  ezEditorGuidEngineHandleMap<ezUInt32> m_OtherPickingMap;
  ezUInt32 m_uiNextComponentPickingID;

  void ProcessIPCMessages();

private:
  virtual void Activate() override;
  virtual void Deactivate() override;
  
  void LogWriter(const ezLoggingEventData& e);

  void HandlerEntityMsg(ezEngineProcessDocumentContext* pDocumentContext, ezViewContext* pViewContext, ezEntityMsgToEngine* pMsg);
  void UpdateProperties(ezEntityMsgToEngine* pMsg, void* pObject, const ezRTTI* pRtti);
  void HandlerGameObjectMsg(ezEngineProcessDocumentContext* pDocumentContext, ezViewContext* pViewContext, ezEntityMsgToEngine* pMsg, ezRTTI* pRtti);
  void HandleComponentMsg(ezEngineProcessDocumentContext* pDocumentContext, ezViewContext* pViewContext, ezEntityMsgToEngine* pMsg, ezRTTI* pRtti);

  void SendReflectionInformation();
  void SendProjectReadyMessage();

  static ezEngineProcessGameState* s_pInstance;

  QApplication* m_pApp;
  ezProcessCommunication m_IPC;
};

