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
    ProcessorStateChanged, ///< ezAssetProcessor::GetProcessorState changed
    ProcessStateChanged, ///< ezAssetProcessor::GetProcessState changed
  };

  Type m_Type;
  ezUInt8 m_uiProcessCount = 0;
  ezUInt8 m_uiProcessorID = 0;
};

struct ezAssetProcessorProgressEvent
{
  enum class Type : ezUInt8
  {
    ProcessingStarted, ///< A processor started working on an asset
    ProcessingFinished ///< A processor finished working on an asset
  };

  Type m_Type;
  ezAssetInfo::TransformState m_TransformState = ezAssetInfo::Unknown;
  ezUInt8 m_uiProcessorID;
  ezUuid m_AssetGuid;
  ezString m_sAssetPath;
  ezTime m_StartTime;
  ezTime m_EndTime;
  ezTransformStatus m_Result; ///< Only valid when m_Type == ProcessingFinished
};


class ezAssetProcessorThread : public ezThread
{
public:
  ezAssetProcessorThread()
    : ezThread("ezProcessThread")
  {
  }


  virtual ezUInt32 Run() override;
};

class ezEditorProcessorProcess
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
  ezEditorProcessorProcess();
  ~ezEditorProcessorProcess();

  ezUInt32 m_uiProcessorID;
  ezTime m_ProcessingStartTime;

  bool Tick(bool bStartNewWork); // returns false, if all processing is done, otherwise call Tick again.

  bool IsConnected() const;
  bool IsRunning() const;
  ezOsProcessID GetProcessId() const;
  bool HasProcessCrashed();

  ezResult StartProcess();
  void ShutdownProcess();

private:
  void EventHandlerIPC(const ezProcessCommunicationChannel::Event& e);

  bool GetNextAssetToProcess(ezAssetInfo* pInfo, ezUuid& out_guid, ezDataDirPath& out_path, ezAssetInfo::TransformState& out_transformState);
  bool GetNextAssetToProcess(ezUuid& out_guid, ezDataDirPath& out_path, ezAssetInfo::TransformState& out_transformState);
  void OnProcessCrashed(ezStringView message);


  State m_State = State::LookingForWork;
  ezUuid m_AssetGuid;
  ezDataDirPath m_AssetPath;
  ezAssetInfo::TransformState m_TransformState = ezAssetInfo::TransformState::Unknown;

  ezUInt64 m_uiAssetHash = 0;
  ezUInt64 m_uiThumbHash = 0;
  ezUInt64 m_uiPackageHash = 0;
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
  ProcessorState GetProcessorState() const
  {
    return m_ProcessorState;
  }
  ezUInt32 GetProcessCount() const;
  ezEditorProcessorState GetProcessState(ezUInt32 uiProcessIndex) const;

  void AddLogWriter(ezLoggingEvent::Handler handler);
  void RemoveLogWriter(ezLoggingEvent::Handler handler);

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
  ezUniquePtr<ezAssetProcessorThread> m_pThread;
  std::atomic<bool> m_bForceStop = false; ///< If set, background processes will be killed when stopping without waiting for their current task to finish.

  // Locks writes to m_ProcessTaskState to make sure the state machine does not go from running to stopped before having fired stopping.
  mutable ezMutex m_ProcessorMutex;
  std::atomic<ProcessorState> m_ProcessorState = ProcessorState::Stopped;
  ezDynamicArray<ezEditorProcessorState> m_EditorProcessorStates;

  // Data owned by the process thread.
  ezDynamicArray<ezEditorProcessorProcess> m_Processes;
};
