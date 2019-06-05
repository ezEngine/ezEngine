#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Threading/Implementation/TaskSystemDeclarations.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Types/Uuid.h>

class ezLongOperation;
class ezProcessCommunicationChannel;

struct ezLongOperationManagerEvent
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

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOperationManager final
{
  EZ_DECLARE_SINGLETON(ezLongOperationManager);

public:
  /// Which operations to replicate to the other process.
  enum class ReplicationMode
  {
    AllOperations, ///< If a local or remote operation is added, a remote or local one is replicated to the other side. Used on the 'engine
                   ///< process' side.
    OnlyRemoteOperations ///< If a remote operation is added, a local operation is replicated to the other side, but if a local operation is
                         ///< added, no remote one is replicated to the other side. Used on the 'editor process' side.
  };

  ezLongOperationManager(ReplicationMode mode);
  ~ezLongOperationManager();

  void AddLongOperation(ezUniquePtr<ezLongOperation>&& pOperation, const ezUuid& documentGuid);

  void Startup(ezProcessCommunicationChannel* pCommunicationChannel);
  void Shutdown();

  struct LongOpInfo
  {
    ezUniquePtr<ezLongOperation> m_pOperation;
    ezTaskGroupID m_TaskID;
    float m_fCompletion = 0.0f;
    ezUuid m_OperationGuid;
    ezUuid m_DocumentGuid;
    ezTime m_StartOrDuration;
  };

  mutable ezMutex m_Mutex;

  ezEvent<const ezLongOperationManagerEvent&> m_Events;

  const ezDynamicArray<LongOpInfo>& GetOperations() const { return m_Operations; }

private:
  friend class ezLongOpTask;
  friend class ezLongOperationLocal;

  void SetCompletion(ezLongOperation* pOperation, float fCompletion);
  void FinishOperation(ezUuid operationGuid);

  void ProcessCommunicationChannelEventHandler(const ezProcessCommunicationChannel::Event& e);
  void LaunchLocalOperation(LongOpInfo& opInfo);

  ezProcessCommunicationChannel* m_pCommunicationChannel = nullptr;
  ReplicationMode m_ReplicationMode;
  ezEvent<const ezProcessCommunicationChannel::Event&>::Unsubscriber m_Unsubscriber;

  ezDynamicArray<LongOpInfo> m_Operations;
};
