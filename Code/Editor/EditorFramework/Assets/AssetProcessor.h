#pragma once

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/Declarations.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EditorProcessCommunicationChannel.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/LogEntry.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadSignal.h>
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

/// \brief Event type used by ezAssetProcessor::m_Events
struct ezAssetProcessorEvent
{
  enum class Type
  {
    AssetProcessorStateChanged, ///< ezAssetProcessor::GetProcessorState changed
    ProcessStateChanged,        ///< ezAssetProcessor::GetProcessState changed
  };

  Type m_Type;
  ezUInt8 m_uiProcessCount = 0; ///< Total number of processes. Only valid if ProcessorStateChanged.
  ezUInt8 m_uiProcessorID = 0;  ///< The changed process index. Only valid if ProcessStateChanged.
};

/// \brief Event type used by ezAssetProcessor::m_ProgressEvents
struct ezAssetProcessorProgressEvent
{
  enum class Type : ezUInt8
  {
    ProcessingStarted, ///< A process started working on an asset
    ProcessingFinished ///< A process finished working on an asset
  };

  Type m_Type;
  ezAssetInfo::TransformState m_TransformState = ezAssetInfo::Unknown;
  ezUInt8 m_uiProcessorID;
  ezUuid m_AssetGuid;
  ezString m_sAssetPath;
  ezTime m_StartTime;
  ezTime m_TransformStartTime;
  ezTime m_EndTime;
  ezTransformStatus m_Result; ///< Only valid when m_Type == ProcessingFinished
};

/// \brief Thread used by ezAssetProcessor to schedule work items on the ezEditorProcessorProcess instances.
class ezAssetProcessorThread : public ezThread
{
public:
  ezAssetProcessorThread()
    : ezThread("ezProcessThread")
  {
  }


  virtual ezUInt32 Run() override;
};

/// \brief Encapsulates one ezEditorProcessor process managed by ezAssetProcessor.
class ezEditorProcessorProcess
{
public:
  enum class State
  {
    StartClient,          ///< Starting client process.
    WaitingForConnection, /// < Waiting for IPC connection to client process.
    LookingForWork,       ///< Connected. Waiting for work to become available.
    ReadyForProcessing,   ///< A work item has been found. Ready to start processing.
    Processing,           ///< A work item is being processed.
    ReportResult,         ///< Report result of the precessing phase.
    Crashed               ///< Process has crashed. Dead end until `RequestRestart` is called.
  };

public:
  ezEditorProcessorProcess();
  ~ezEditorProcessorProcess();

  ezUInt32 m_uiProcessorID;
  ezTime m_ProcessingStartTime;  // When a work item was started.
  ezThreadSignal* m_pNewWorkSignal = nullptr;

  bool Tick(bool bStartNewWork); // returns false, if all processing is done, otherwise call Tick again.

  /// \brief Called by the worker thread to restart a crashed process.
  void RequestRestart();

  bool IsConnected() const;
  bool IsRunning() const;
  bool IsCrashed() const;
  ezOsProcessID GetProcessId() const;
  bool HasProcessCrashed();
  void HandleHashMissmatch();

  ezResult StartProcess();
  void ShutdownProcess();

private:
  void EventHandlerIPC(const ezProcessCommunicationChannel::Event& e);
  void ChannelEventHandler(const ezIpcChannelEvent& e);

  bool GetNextAssetToProcess(ezAssetInfo* pInfo, ezUuid& out_guid, ezDataDirPath& out_path, ezAssetInfo::TransformState& out_transformState);
  bool GetNextAssetToProcess(ezUuid& out_guid, ezDataDirPath& out_path, ezAssetInfo::TransformState& out_transformState);
  void OnProcessCrashed(ezStringView message);

private:
  State m_State = State::StartClient;
  ezEditorProcessCommunicationChannel* m_pIPC;
  bool m_bProcessShouldBeRunning = false;
  bool m_bIsIdle = false;
  ezOsProcessID m_CurrentProcessID = {};

  // New asset to process
  ezUuid m_AssetGuid;
  ezDataDirPath m_AssetPath;
  ezAssetInfo::TransformState m_TransformState = ezAssetInfo::TransformState::Unknown;
  ezString m_sPlatform;
  ezUInt64 m_uiAssetHash = 0;
  ezUInt64 m_uiThumbHash = 0;
  ezUInt64 m_uiPackageHash = 0;
  ezDynamicArray<ezString> m_TransitiveHull;

  // Transform result
  ezTransformStatus m_Status;
  ezDynamicArray<ezLogEntry> m_LogEntries;
  ezMap<ezString, ezUInt64> m_MissmatchTransformDependencies;
  ezMap<ezString, ezUInt64> m_MissmatchThumbnailDependencies;
  ezUInt64 m_uiMissmatchAssetHash = 0;
  ezUInt64 m_uiMissmatchThumbHash = 0;
  ezTime m_StartedProcessing;
  ezTime m_StartedTransform;
  ezTime m_FinishedProcessing;
};

/// \brief Background asset processing is handled by this class.
/// Creates ezEditorProcessor processes which are managed by the ezEditorProcessorProcess class.
class EZ_EDITORFRAMEWORK_DLL ezAssetProcessor
{
  EZ_DECLARE_SINGLETON(ezAssetProcessor);

public:
  enum class ProcessorState : ezUInt8
  {
    Stopped,  ///< No EditorProcessor or the process thread is running.
    Running,  ///< Everything is active.
    Stopping, ///< Everything is still running but no new tasks are put into the EditorProcessors.
  };

  ezAssetProcessor();
  ~ezAssetProcessor();

  // used to temporarily not process assets, usually because currently assets get imported
  ezAtomicInteger32 m_iPauseProcessing;

  void StartProcessor();
  void StopProcessor(bool bForce);

  /// \brief Returns whether the asset processor is running, stopped or stopping.
  ProcessorState GetProcessorState() const
  {
    return m_ProcessorState;
  }

  /// \brief Returns how many ezEditorProcessor processes are managed by the ezAssetProcessor.
  ezUInt32 GetProcessCount() const;
  /// \brief Returns the state of one of the ezEditorProcessor processes.
  /// \param uiProcessIndex The index of the process. Must be smaller than GetProcessCount.
  ezEditorProcessorState GetProcessState(ezUInt32 uiProcessIndex) const;

  /// \brief Requests a restart of a crashed processor.
  /// This is safe to call from any thread. The restart will be handled by the worker thread.
  /// \param uiProcessIndex The index of the crashed process to restart. Must be smaller than GetProcessCount.
  void RequestRestartProcess(ezUInt32 uiProcessIndex);

  void AddLogWriter(ezLoggingEvent::Handler handler);
  void RemoveLogWriter(ezLoggingEvent::Handler handler);
  void UpdateProcessStates();

public:
  // Can be called from worker threads!
  ezCopyOnBroadcastEvent<const ezAssetProcessorEvent&, ezMutex> m_Events;
  ezCopyOnBroadcastEvent<const ezAssetProcessorProgressEvent&, ezMutex> m_ProgressEvents;

private:
  friend class ezEditorProcessorProcess;
  friend class ezAssetProcessorThread;
  friend class ezAssetCurator;

  void Run();

private:
  ezAssetProcessorLog m_CuratorLog;

  // Process thread and its state
  ezThreadSignal m_NewWorkSignal;
  ezUniquePtr<ezAssetProcessorThread> m_pThread;
  std::atomic<bool> m_bForceStop = false; ///< If set, background processes will be killed when stopping without waiting for their current task to finish.

  // Locks writes to m_ProcessTaskState to make sure the state machine does not go from running to stopped before having fired stopping.
  mutable ezMutex m_ProcessorMutex;
  std::atomic<ProcessorState> m_ProcessorState = ProcessorState::Stopped;
  ezDynamicArray<ezEditorProcessorState> m_EditorProcessorStates;
  ezDynamicArray<ezAtomicBool> m_RestartRequests; ///< Set by main thread, read by worker thread

  // Data owned by the process thread.
  ezDynamicArray<ezEditorProcessorProcess> m_Processes;
};
