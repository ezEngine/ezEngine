#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/List.h>
#include <Foundation/Types/UniquePtr.h>

class ezLongOperation;
class ezProcessCommunicationChannel;

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

  void AddLongOperation(ezUniquePtr<ezLongOperation>&& pOperation);

  const ezDynamicArray<ezUniquePtr<ezLongOperation>>& GetActiveOperations() const;

  void Startup(ezProcessCommunicationChannel* pCommunicationChannel);
  void Shutdown();

private:
  void ProcessCommunicationChannelEventHandler(const ezProcessCommunicationChannel::Event& e);

  ezProcessCommunicationChannel* m_pCommunicationChannel = nullptr;
  ReplicationMode m_ReplicationMode;
  ezDynamicArray<ezUniquePtr<ezLongOperation>> m_ActiveOperations;
  ezEvent<const ezProcessCommunicationChannel::Event&>::Unsubscriber m_Unsubscriber;
};
