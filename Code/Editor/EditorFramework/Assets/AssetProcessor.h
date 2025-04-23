#pragma once

#include <EditorFramework/Assets/Declarations.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EditorProcessCommunicationChannel.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/LogEntry.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Types/UniquePtr.h>
#include <ToolsFoundation/FileSystem/DataDirPath.h>
#include <atomic>

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
  enum class State
  {
    LookingForWork,
    WaitingForConnection,
    Ready,
    Processing,
    ReportResult
  };

public:
  ezProcessTask();
  ~ezProcessTask();

  ezUInt32 m_uiProcessorID;

  bool Tick(bool bStartNewWork); // returns false, if all processing is done, otherwise call Tick again.

  bool IsConnected();

  bool HasProcessCrashed();

  ezResult StartProcess();

  void ShutdownProcess();

private:
  void EventHandlerIPC(const ezProcessCommunicationChannel::Event& e);

  bool GetNextAssetToProcess(ezAssetInfo* pInfo, ezUuid& out_guid, ezDataDirPath& out_path);
  bool GetNextAssetToProcess(ezUuid& out_guid, ezDataDirPath& out_path);
  void OnProcessCrashed(ezStringView message);


  State m_State = State::LookingForWork;
  ezUuid m_AssetGuid;
  ezUInt64 m_uiAssetHash = 0;
  ezUInt64 m_uiThumbHash = 0;
  ezUInt64 m_uiPackageHash = 0;
  ezDataDirPath m_AssetPath;
  ezEditorProcessCommunicationChannel* m_pIPC;
  bool m_bProcessShouldBeRunning = false;
  ezTransformStatus m_Status;
  ezDynamicArray<ezLogEntry> m_LogEntries;
  ezDynamicArray<ezString> m_TransitiveHull;
};

/// \brief Background asset processing is handled by this class.
/// Creates EditorProcessor processes.
class EZ_EDITORFRAMEWORK_DLL ezAssetProcessor
{
  EZ_DECLARE_SINGLETON(ezAssetProcessor);

public:
  enum class ProcessTaskState : ezUInt8
  {
    Stopped,  ///< No EditorProcessor or the process thread is running.
    Running,  ///< Everything is active.
    Stopping, ///< Everything is still running but no new tasks are put into the EditorProcessors.
  };

  ezAssetProcessor();
  ~ezAssetProcessor();

  // used to temporarily not process assets, usually because currently assets get imported
  ezAtomicInteger32 m_iPauseProcessing;

  void StartProcessTask();
  void StopProcessTask(bool bForce);
  ProcessTaskState GetProcessTaskState() const
  {
    return m_ProcessTaskState;
  }

  void AddLogWriter(ezLoggingEvent::Handler handler);
  void RemoveLogWriter(ezLoggingEvent::Handler handler);

public:
  // Can be called from worker threads!
  ezEvent<const ezAssetProcessorEvent&> m_Events;

private:
  friend class ezProcessTask;
  friend class ezProcessThread;
  friend class ezAssetCurator;

  void Run();

private:
  ezAssetProcessorLog m_CuratorLog;

  // Process thread and its state
  ezUniquePtr<ezProcessThread> m_pThread;
  std::atomic<bool> m_bForceStop = false; ///< If set, background processes will be killed when stopping without waiting for their current task to finish.

  // Locks writes to m_ProcessTaskState to make sure the state machine does not go from running to stopped before having fired stopping.
  mutable ezMutex m_ProcessorMutex;
  std::atomic<ProcessTaskState> m_ProcessTaskState = ProcessTaskState::Stopped;

  // Data owned by the process thread.
  ezDynamicArray<ezProcessTask> m_ProcessTasks;
};
