#pragma once

#include <EditorEngineProcessFramework/LongOps/Implementation/LongOpManager.h>

class ezLongOpProxy;

/// \brief Events about all known long ops. Broadcast by ezLongOpControllerManager.
struct ezLongOpControllerEvent
{
  enum class Type
  {
    OpAdded,    ///< A new long op has been added / registered.
    OpRemoved,  ///< A long op has been deleted. The GUID is sent, but it cannot be resolved anymore.
    OpProgress, ///< The completion progress of a long op has changed.
  };

  Type m_Type;
  ezUuid m_OperationGuid; ///< Use ezLongOpControllerManager::GetOperation() to resolve the GUID to the actual long op.
};

/// \brief The LongOp controller is active in the editor process and manages which long ops are available, running, etc.
///
/// All available long ops are registered with the controller, typically automatically by the ezLongOpsAdapter,
/// although it is theoretically possible to register additional long ops.
///
/// Through the controller long ops can be started or canceled, which is exposed in the UI by the ezQtLongOpsPanel.
///
/// Through the broadcast ezLongOpControllerEvent, one can track the state of all long ops.
class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOpControllerManager final : public ezLongOpManager
{
  EZ_DECLARE_SINGLETON(ezLongOpControllerManager);

public:
  ezLongOpControllerManager();
  ~ezLongOpControllerManager();

  /// \brief Holds all information about the proxy long op on the editor side
  struct ProxyOpInfo
  {
    ezUniquePtr<ezLongOpProxy> m_pProxyOp;
    ezUuid m_OperationGuid;   ///< Identifies the operation itself.
    ezUuid m_DocumentGuid;    ///< To which document the long op belongs. When the document is closed, all running long ops belonging to it
                              ///< will be canceled.
    ezUuid m_ComponentGuid;   ///< To which component in the scene document the long op is linked. If the component is deleted, the long op
                              ///< disappears as well.
    ezTime m_StartOrDuration; ///< While m_bIsRunning is true, this is the time the long op started, once m_bIsRunning it holds the last
                              ///< duration of the long op execution.
    float m_fCompletion = 0.0f; ///< [0; 1] range for the progress.
    bool m_bIsRunning = false;  ///< Whether the long op is currently being executed.
  };

  /// \brief Events about the state of all available long ops.
  ezEvent<const ezLongOpControllerEvent&> m_Events;

  /// \brief Typically called by ezLongOpsAdapter when a component that has an ezLongOpAttribute is added to a scene
  void RegisterLongOp(const ezUuid& documentGuid, const ezUuid& componentGuid, const char* szLongOpType);

  /// \brief Typically called by ezLongOpsAdapter when a component that has an ezLongOpAttribute is removed from a scene
  void UnregisterLongOp(const ezUuid& documentGuid, const ezUuid& componentGuid, const char* szLongOpType);

  /// \brief Starts executing the given long op. Typically called by the ezQtLongOpsPanel.
  void StartOperation(ezUuid opGuid);

  /// \brief Cancels a given long op. Typically called by the ezQtLongOpsPanel.
  void CancelOperation(ezUuid opGuid);

  /// \brief Cancels and deletes all operations linked to the given document. Makes sure to wait for all canceled ops.
  /// Typically called by the ezLongOpsAdapter when a document is about to be closed.
  void CancelAndRemoveAllOpsForDocument(const ezUuid& documentGuid);

  /// \brief Returns a pointer to the given long op, or null if the GUID does not exist.
  ProxyOpInfo* GetOperation(const ezUuid& guid);

  /// \brief Gives access to all currently available long ops. Make sure the lock m_Mutex (of the ezLongOpManager base class) while accessing this.
  const ezDynamicArray<ezUniquePtr<ProxyOpInfo>>& GetOperations() const { return m_ProxyOps; }

private:
  virtual void ProcessCommunicationChannelEventHandler(const ezProcessCommunicationChannel::Event& e) override;

  void ReplicateToWorkerProcess(ProxyOpInfo& opInfo);
  void BroadcastProgress(ProxyOpInfo& opInfo);
  void RemoveOperation(ezUuid opGuid);

  ezDynamicArray<ezUniquePtr<ProxyOpInfo>> m_ProxyOps;
};
