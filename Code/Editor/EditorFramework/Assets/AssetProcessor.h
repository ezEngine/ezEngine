#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/LogEntry.h>
#include <EditorFramework/IPC/EditorProcessCommunicationChannel.h>

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
  friend class ezAssetCurator;

  void OnProcessTaskFinished(ezTask* pTask);
  void RunNextProcessTask();
  void AssetCuratorEventHandler(const ezAssetCuratorEvent& e);

private:
  mutable ezMutex m_ProcessorMutex;
  ezAssetProcessorLog m_CuratorLog;
  ezAtomicInteger32 m_bRunProcessTask;

  struct TaskAndGroup
  {
    ezProcessTask* m_pTask = nullptr;
    ezTaskGroupID m_GroupID;
  };

  ezDynamicArray<TaskAndGroup> m_ProcessTasks;
  ezAtomicInteger32 m_TicksWithIdleTasks;
};

class ezProcessTask final : public ezTask
{
public:
  ezProcessTask(ezUInt32 uiProcessorID, ezTask::OnTaskFinished onFinished);
  ~ezProcessTask();
  ezAtomicInteger32 m_bDidWork = true;

private:
  void StartProcess();
  void ShutdownProcess();
  void EventHandlerIPC(const ezProcessCommunicationChannel::Event& e);

  bool GetNextAssetToProcess(ezAssetInfo* pInfo, ezUuid& out_guid, ezStringBuilder& out_sAbsPath, ezStringBuilder& out_sRelPath);
  bool GetNextAssetToProcess(ezUuid& out_guid, ezStringBuilder& out_sAbsPath, ezStringBuilder& out_sRelPath);
  void OnProcessCrashed();

  ezUInt32 m_uiProcessorID;

  ezUuid m_assetGuid;
  ezUInt64 m_AssetHash = 0;
  ezUInt64 m_ThumbHash = 0;
  ezStringBuilder m_sAssetPath;
  ezEditorProcessCommunicationChannel* m_pIPC;
  bool m_bProcessShouldBeRunning;
  bool m_bProcessCrashed;
  bool m_bWaiting;
  bool m_bSuccess;
  ezDynamicArray<ezLogEntry> m_LogEntries;
  virtual void Execute() override;
};
