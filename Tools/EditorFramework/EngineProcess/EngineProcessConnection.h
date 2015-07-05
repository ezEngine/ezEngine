#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <Foundation/Communication/Event.h>

class ezEditorEngineConnection;
class ezDocumentBase;
class ezDocumentObjectBase;
struct ezDocumentObjectPropertyEvent;
struct ezDocumentObjectStructureEvent;
class ezDocumentWindow3D;

class EZ_EDITORFRAMEWORK_DLL ezEditorEngineProcessConnection
{
  static ezEditorEngineProcessConnection* s_pInstance;

public:
  ezEditorEngineProcessConnection();
  ~ezEditorEngineProcessConnection();

  static ezEditorEngineProcessConnection* GetInstance() { return s_pInstance; }

  void SetFileSystemConfig(const ezApplicationFileSystemConfig& cfg) { m_FileSystemConfig = cfg; }
  void Update();
  ezResult RestartProcess();
  void ShutdownProcess();
  bool IsProcessCrashed() const { return m_bProcessCrashed; }

  ezEditorEngineConnection* CreateEngineConnection(ezDocumentWindow3D* pWindow);
  void DestroyEngineConnection(ezDocumentWindow3D* pWindow);

  void SendMessage(ezProcessMessage* pMessage);
  ezResult WaitForMessage(const ezRTTI* pMessageType, ezTime tTimeout);

  void SetWaitForDebugger(bool bWait) { m_bProcessShouldWaitForDebugger = bWait; }
  bool GetWaitForDebugger() const { return m_bProcessShouldWaitForDebugger; }

  bool IsEngineSetup() const { return m_bClientIsConfigured; }

  struct Event
  {
    enum class Type
    {
      Invalid,
      ProcessStarted,
      ProcessCrashed,
      ProcessShutdown,
      ProcessMessage,
    };

    Event()
    {
      m_Type = Type::Invalid;
      m_pMsg = nullptr;
    }

    Type m_Type;
    const ezProcessMessage* m_pMsg;
  };

  static ezEvent<const Event&> s_Events;


private:
  void Initialize();
  void HandleIPCEvent(const ezProcessCommunication::Event& e);
  void SendDocumentOpenMessage(ezUInt32 uiViewID, const ezUuid& guid, bool bOpen);

  bool m_bProcessShouldWaitForDebugger;
  bool m_bProcessShouldBeRunning;
  bool m_bProcessCrashed;
  bool m_bClientIsConfigured;
  ezUInt32 m_uiNextEngineViewID;
  ezInt32 m_iNumViews;
  ezProcessCommunication m_IPC;
  ezApplicationFileSystemConfig m_FileSystemConfig;
  ezHashTable<ezUInt32, ezDocumentWindow3D*> m_EngineViewsByID;
};

class EZ_EDITORFRAMEWORK_DLL ezEditorEngineConnection
{
public:

  void SendMessage(ezEditorEngineDocumentMsg* pMessage);

  void SendObjectProperties(const ezDocumentObjectPropertyEvent& e);
  void SendDocumentTreeChange(const ezDocumentObjectStructureEvent& e);
  void SendDocument();

  ezDocumentBase* GetDocument() const { return m_pDocument; }

private:
  friend class ezEditorEngineProcessConnection;
  ezEditorEngineConnection(ezDocumentBase* pDocument, ezInt32 iEngineViewID) { m_pDocument = pDocument; m_iEngineViewID = iEngineViewID; }
  ~ezEditorEngineConnection() { }

  void SendObject(const ezDocumentObjectBase* pObject);

  ezDocumentBase* m_pDocument;
  ezInt32 m_iEngineViewID;

};
