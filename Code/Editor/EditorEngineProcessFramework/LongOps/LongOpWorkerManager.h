#pragma once

#include <EditorEngineProcessFramework/LongOps/Implementation/LongOpManager.h>

#include <Foundation/Utilities/Progress.h>
#include <Foundation/Threading/Implementation/TaskSystemDeclarations.h>
#include <Foundation/Containers/DynamicArray.h>

class ezLongOpWorker;
struct ezProgressEvent;
typedef ezDynamicArray<ezUInt8> ezDataBuffer;

/// \brief The LongOp worker manager is active in the engine process of the editor.
///
/// This class has no public functionality, it communicates with the ezLongOpControllerManager
/// and executes the ezLongOpWorker's that are named by the respective ezLongOpProxy's.
class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOpWorkerManager final : public ezLongOpManager
{
  EZ_DECLARE_SINGLETON(ezLongOpWorkerManager);

public:
  ezLongOpWorkerManager();
  ~ezLongOpWorkerManager();

private:
  friend class ezLongOpTask;

  struct WorkerOpInfo
  {
    ezUniquePtr<ezLongOpWorker> m_pWorkerOp;
    ezTaskGroupID m_TaskID;
    ezUuid m_DocumentGuid;
    ezUuid m_OperationGuid;
    ezProgress m_Progress;
    ezEvent<const ezProgressEvent&>::Unsubscriber m_ProgressSubscription;
  };

  virtual void ProcessCommunicationChannelEventHandler(const ezProcessCommunicationChannel::Event& e) override;
  WorkerOpInfo* GetOperation(const ezUuid& guid) const;
  void LaunchWorkerOperation(WorkerOpInfo& opInfo, ezStreamReader& config);
  void WorkerProgressBarEventHandler(const ezProgressEvent& e);
  void RemoveOperation(ezUuid opGuid);
  void SendProgress(WorkerOpInfo& opInfo);
  void WorkerOperationFinished(ezUuid operationGuid, ezResult result, ezDataBuffer&& resultData);

  ezDynamicArray<ezUniquePtr<WorkerOpInfo>> m_WorkerOps;
};
