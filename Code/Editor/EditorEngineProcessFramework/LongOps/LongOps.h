#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>

class ezStringBuilder;
class ezStreamWriter;
class ezProgress;

//////////////////////////////////////////////////////////////////////////

/// \brief Proxy long ops represent a long operation on the editor side.
///
/// Proxy long ops have little functionality other than naming which ezLongOpWorker to execute
/// in the engine process and to feed it with the necessary parameters.
/// Since the proxy long op runs in the editor process, it may access ezDocumentObject's
/// and extract data from them.
class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOpProxy : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOpProxy, ezReflectedClass);

public:
  /// \brief Called once by ezLongOpControllerManager::RegisterLongOp() to inform the proxy
  /// to which ezDocument and component (ezDocumentObject) it is linked.
  virtual void InitializeRegistered(const ezUuid& documentGuid, const ezUuid& componentGuid) {}

  /// \brief Called by the ezQtLongOpsPanel to determine the display string to be shown in the UI.
  virtual const char* GetDisplayName() const = 0;

  /// \brief Called every time the long op shall be executed
  /// \param out_sReplicationOpType must name the ezLongOpWorker that shall be executed in the engine process.
  /// \param config can be optionally written to. The data is transmitted to the ezLongOpWorker on the other side
  /// and fed to it in ezLongOpWorker::InitializeExecution(). 
  virtual void GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& config) = 0;

  /// \brief Called once the corresponding ezLongOpWorker has finished.
  /// \param result Whether the operation succeeded or failed (e.g. via user cancellation).
  /// \param resultData Optional data written by ezLongOpWorker::Execute().
  virtual void Finalize(ezResult result, const ezDataBuffer& resultData) {}
};

//////////////////////////////////////////////////////////////////////////

/// \brief Worker long ops are executed by the editor engine process.
///
/// They typically do the actual long processing. Since they run in the engine process, they have access
/// to the runtime scene graph and resources but not the editor representation of the scene.
///
/// ezLongOpWorker instances are automatically instantiated by ezLongOpWorkerManager when they have
/// been named by a ezLongOpProxy's GetReplicationInfo() function.
class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezLongOpWorker : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOpWorker, ezReflectedClass);

public:
  /// \brief Called within the engine processes main thread.
  /// The function may lock the ezWorld from the given scene document and extract vital information.
  /// It should try to be as quick as possible and leave the heavy lifting to Execute(), which will run on a background thread.
  /// If this function return failure, the long op is canceled right away.
  virtual ezResult InitializeExecution(ezStreamReader& config, const ezUuid& DocumentGuid) { return EZ_SUCCESS; }

  /// \brief Executed in a separete thread after InitializeExecution(). This should do the work that takes a while.
  /// 
  /// This function may write the result data directly to disk. Everything that is written to \a proxydata
  /// will be transmitted back to the proxy long op and given to ezLongOpProxy::Finalize(). Since this requires IPC bandwidth
  /// the amount of data should be kept very small (a few KB at most).
  ///
  /// All updates to \a progress will be automatically synchronized back to the editor process and become visible through
  /// the ezLongOpControllerManager via the ezLongOpControllerEvent.
  /// Use ezProgressRange for convenient progress updates.
  virtual ezResult Execute(ezProgress& progress, ezStreamWriter& proxydata) = 0;
};
