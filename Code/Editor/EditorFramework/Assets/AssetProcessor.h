#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EditorProcessCommunicationChannel.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/LogEntry.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Types/UniquePtr.h>

struct ezAssetCuratorEvent;
class ezTask;
struct ezAssetInfo;

/// \brief Log for all background processing results
class ezAssetProcessorLog : public ezLogInterface
{
public:
  virtual void HandleLogMessage(const ezLoggingEventData& le) override;
  void AddLogWriter(ezLoggingEvent::Handler handler);
  void RemoveLogWriter(ezLoggingEvent::Handler handler);

  ezLoggingEvent m_LoggingEvent;
};

struct ezAssetProcessorEvent
{
  enum class Type
  {
    ProcessTaskStateChanged
  };

  Type m_Type;
};


class ezProcessThread : public ezThread
{
public:
  ezProcessThread()
    : ezThread("ezProcessThread")
  {
  }


  virtual ezUInt32 Run() override;
};

class ezProcessTask
{
public:
  ezProcessTask();
  ~ezProcessTask();

  ezAtomicInteger32 m_bDidWork = true;
  ezUInt32 m_uiProcessorID;

  bool BeginExecute();

  bool FinishExecute();

private:
  void StartProcess();
  void ShutdownProcess();
  void EventHandlerIPC(const ezProcessCommunicationChannel::Event& e);

  bool GetNextAssetToProcess(ezAssetInfo* pInfo, ezUuid& out_guid, ezStringBuilder& out_sAbsPath, ezStringBuilder& out_sRelPath);
  bool GetNextAssetToProcess(ezUuid& out_guid, ezStringBuilder& out_sAbsPath, ezStringBuilder& out_sRelPath);
  void OnProcessCrashed();


  ezUuid m_AssetGuid;
  ezUInt64 m_uiAssetHash = 0;
  ezUInt64 m_uiThumbHash = 0;
  ezStringBuilder m_sAssetPath;
  ezEditorProcessCommunicationChannel* m_pIPC;
  bool m_bProcessShouldBeRunning;
  bool m_bProcessCrashed;
  bool m_bWaiting;
  ezStatus m_Status;
  ezDynamicArray<ezLogEntry> m_LogEntries;
  ezDynamicArray<ezString> m_TransitiveHull;
};

/// \brief Background asset processing is handled by this class.
/// Creates EditorProcessor processes.
class EZ_EDITORFRAMEWORK_DLL ezAssetProcessor
{
  EZ_DECLARE_SINGLETON(ezAssetProcessor);

public:
  ezAssetProcessor();
  ~ezAssetProcessor();

  void RestartProcessTask();
  void ShutdownProcessTask();
  bool IsProcessTaskRunning() const;

  void AddLogWriter(ezLoggingEvent::Handler handler);
  void RemoveLogWriter(ezLoggingEvent::Handler handler);

public:
  ezEvent<const ezAssetProcessorEvent&> m_Events;

private:
  friend class ezProcessTask;
  friend class ezProcessThread;
  friend class ezAssetCurator;

  void Run();

private:
  mutable ezMutex m_ProcessorMutex;
  ezAssetProcessorLog m_CuratorLog;
  ezAtomicBool m_bRunProcessTask;

  ezProcessThread m_Thread;
  ezDynamicArray<bool> m_ProcessRunning;
  ezDynamicArray<ezProcessTask> m_ProcessTasks;
};
