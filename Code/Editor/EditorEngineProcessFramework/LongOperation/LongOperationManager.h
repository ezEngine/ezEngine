#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Threading/Implementation/TaskSystemDeclarations.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Utilities/Progress.h>

class ezLongOp;
class ezProcessCommunicationChannel;
struct ezProgressEvent;

struct ezLongOpManagerEvent
{
  EZ_DECLARE_POD_TYPE();

  enum class Type
  {
    OpAdded,
    OpFinished,
    OpProgress,
  };

  Type m_Type;
  ezUInt32 m_uiOperationIndex;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOpManager final
{
  EZ_DECLARE_SINGLETON(ezLongOpManager);

public:
  /// Which operations to replicate to the other process.
  enum class Mode
  {
    Processor, ///< If a worker or proxy operation is added, a proxy or worker one is replicated to the other side. Used on the 'engine
                   ///< process' side.
    Controller ///< If a proxy operation is added, a worker operation is replicated to the other side, but if a worker operation is
                        ///< added, no proxy one is replicated to the other side. Used on the 'editor process' side.
  };

  ezLongOpManager(Mode mode);
  ~ezLongOpManager();

  void AddLongOperation(ezUniquePtr<ezLongOp>&& pOperation, const ezUuid& documentGuid);
  void CancelOperation(ezUInt32 uiOperationIndex);

  void Startup(ezProcessCommunicationChannel* pCommunicationChannel);
  void Shutdown();

  struct LongOpInfo
  {
    ezUniquePtr<ezLongOp> m_pOperation;
    ezTaskGroupID m_TaskID;
    ezUuid m_OperationGuid;
    ezUuid m_DocumentGuid;
    ezTime m_StartOrDuration;
    ezProgress m_Progress;
    ezEvent<const ezProgressEvent&>::Unsubscriber m_ProgressSubscription;
  };

  mutable ezMutex m_Mutex;

  ezEvent<const ezLongOpManagerEvent&> m_Events;

  const ezDynamicArray<ezUniquePtr<LongOpInfo>>& GetOperations() const { return m_Operations; }

private:
  friend class ezLongOpTask;
  friend class ezLongOpWorker;

  void SetCompletion(ezLongOp* pOperation, float fCompletion);
  void FinishOperation(ezUuid operationGuid);
  void ProgressBarEventHandler(const ezProgressEvent& e);

  void ProcessCommunicationChannelEventHandler(const ezProcessCommunicationChannel::Event& e);
  void LaunchWorkerOperation(LongOpInfo& opInfo);


  ezProcessCommunicationChannel* m_pCommunicationChannel = nullptr;
  Mode m_Mode;
  ezEvent<const ezProcessCommunicationChannel::Event&>::Unsubscriber m_Unsubscriber;

  ezDynamicArray<ezUniquePtr<LongOpInfo>> m_Operations;
};
