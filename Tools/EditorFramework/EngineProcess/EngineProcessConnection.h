#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <Foundation/Communication/Event.h>
#include <Core/Application/Config/PluginConfig.h>
#include <Foundation/Configuration/Singleton.h>

class ezEditorEngineConnection;
class ezDocument;
class ezDocumentObject;
struct ezDocumentObjectPropertyEvent;
struct ezDocumentObjectStructureEvent;
class ezQtEngineDocumentWindow;
class ezAssetDocument;

class EZ_EDITORFRAMEWORK_DLL ezEditorEngineProcessConnection
{
  EZ_DECLARE_SINGLETON(ezEditorEngineProcessConnection);

public:
  ezEditorEngineProcessConnection();
  ~ezEditorEngineProcessConnection();

  /// \brief The given file system configuration will be used by the engine process to setup the runtime data directories.
  ///        This only takes effect if the editor process is restarted.
  void SetFileSystemConfig(const ezApplicationFileSystemConfig& cfg) { m_FileSystemConfig = cfg; }

  /// \brief The given plugin configuration will be used by the engine process to load runtime plugins.
  ///        This only takes effect if the editor process is restarted.
  void SetPluginConfig(const ezApplicationPluginConfig& cfg) { m_PluginConfig = cfg; }

  void Update();
  ezResult RestartProcess();
  void ShutdownProcess();
  bool IsProcessCrashed() const { return m_bProcessCrashed; }

  ezEditorEngineConnection* CreateEngineConnection(ezAssetDocument* pDocument);
  void DestroyEngineConnection(ezAssetDocument* pDocument);

  void SendMessage(ezProcessMessage* pMessage);

  /// /brief Waits for a message of type pMessageType. If tTimeout is zero, the function will not timeout. If the timeout is valid
  ///        and is it, EZ_FAILURE is returned. If the message type matches and pCallback is valid, the function will be called
  ///        and the return values decides whether the message is to be accepted and the waiting has ended.
  ezResult WaitForMessage(const ezRTTI* pMessageType, ezTime tTimeout, ezProcessCommunication::WaitForMessageCallback* pCallback = nullptr);
  /// /brief Same as WaitForMessage but the message must be to a specific document. Therefore,
  ///        pMessageType must be derived from ezEditorEngineDocumentMsg and the function will only return if the received
  ///        message matches both type, document and is accepted by pCallback.
  ezResult WaitForDocumentMessage(const ezUuid& assetGuid, const ezRTTI* pMessageType, ezTime tTimeout, ezProcessCommunication::WaitForMessageCallback* pCallback = nullptr);

  void SetWaitForDebugger(bool bWait) { m_bProcessShouldWaitForDebugger = bWait; }
  bool GetWaitForDebugger() const { return m_bProcessShouldWaitForDebugger; }

  bool IsEngineSetup() const { return m_bClientIsConfigured; }

  /// /brief Sends a message that the document has been opened or closed. Resends all document data.
  ///
  /// Calling this will always clear the existing document on the engine side and reset the state to the editor state.
  void SendDocumentOpenMessage(const ezDocument* pDocument, bool bOpen);

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
  void Initialize(const ezRTTI* pFirstAllowedMessageType);
  void HandleIPCEvent(const ezProcessCommunication::Event& e);
  

  bool m_bProcessShouldWaitForDebugger;
  bool m_bProcessShouldBeRunning;
  bool m_bProcessCrashed;
  bool m_bClientIsConfigured;
  ezProcessCommunication m_IPC;
  ezApplicationFileSystemConfig m_FileSystemConfig;
  ezApplicationPluginConfig m_PluginConfig;
  ezHashTable<ezUuid, ezAssetDocument*> m_DocumentByGuid;
};

class EZ_EDITORFRAMEWORK_DLL ezEditorEngineConnection
{
public:

  void SendMessage(ezEditorEngineDocumentMsg* pMessage);

  ezDocument* GetDocument() const { return m_pDocument; }

private:
  friend class ezEditorEngineProcessConnection;
  ezEditorEngineConnection(ezDocument* pDocument) { m_pDocument = pDocument; }
  ~ezEditorEngineConnection() { }

  ezDocument* m_pDocument;
};
