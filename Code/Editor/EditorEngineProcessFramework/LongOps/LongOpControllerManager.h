#pragma once

#include <EditorEngineProcessFramework/LongOps/Implementation/LongOpManager.h>

class ezLongOpProxy;

struct ezLongOpControllerEvent
{
  enum class Type
  {
    OpAdded,
    OpRemoved,
    OpProgress,
  };

  Type m_Type;
  ezUuid m_OperationGuid;
};

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOpControllerManager final : public ezLongOpManager
{
  EZ_DECLARE_SINGLETON(ezLongOpControllerManager);

public:
  ezLongOpControllerManager();
  ~ezLongOpControllerManager();

  struct ProxyOpInfo
  {
    ezUniquePtr<ezLongOpProxy> m_pProxyOp;
    ezUuid m_OperationGuid;
    ezUuid m_DocumentGuid;
    ezUuid m_ComponentGuid;
    ezTime m_StartOrDuration;
    float m_fCompletion = 0.0f;
    bool m_bIsRunning = false;
  };

  void RegisterLongOp(const ezUuid& documentGuid, const ezUuid& componentGuid, const char* szLongOpType);
  void UnregisterLongOp(const ezUuid& documentGuid, const ezUuid& componentGuid, const char* szLongOpType);

  void StartOperation(ezUuid opGuid);
  void CancelOperation(ezUuid opGuid);

  void DocumentClosed(const ezUuid& documentGuid);

  ezEvent<const ezLongOpControllerEvent&> m_Events;

  ProxyOpInfo* GetOperation(const ezUuid& guid) const;
  const ezDynamicArray<ezUniquePtr<ProxyOpInfo>>& GetOperations() const { return m_ProxyOps; }

private:
  virtual void ProcessCommunicationChannelEventHandler(const ezProcessCommunicationChannel::Event& e) override;

  void ReplicateToWorkerProcess(ProxyOpInfo& opInfo);
  void BroadcastProgress(ProxyOpInfo& opInfo);
  void RemoveOperation(ezUuid opGuid);

  ezDynamicArray<ezUniquePtr<ProxyOpInfo>> m_ProxyOps;
};

