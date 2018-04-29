#include <PCH.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Configuration/SubSystem.h>

EZ_IMPLEMENT_SINGLETON(ezAssetProcessor);

EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, AssetProcessor)

BEGIN_SUBSYSTEM_DEPENDENCIES
"AssetCurator"
END_SUBSYSTEM_DEPENDENCIES

ON_CORE_STARTUP
{
  EZ_DEFAULT_NEW(ezAssetProcessor);
}

ON_CORE_SHUTDOWN
{
  ezAssetProcessor* pDummy = ezAssetProcessor::GetSingleton();
  EZ_DEFAULT_DELETE(pDummy);
}

ON_ENGINE_STARTUP
{
}

ON_ENGINE_SHUTDOWN
{
}

EZ_END_SUBSYSTEM_DECLARATION

////////////////////////////////////////////////////////////////////////
// ezCuratorLog
////////////////////////////////////////////////////////////////////////

void ezAssetProcessorLog::HandleLogMessage(const ezLoggingEventData& le)
{
  const ezLogMsgType::Enum ThisType = le.m_EventType;
  m_LoggingEvent.Broadcast(le);
}

void ezAssetProcessorLog::AddLogWriter(ezLoggingEvent::Handler handler)
{
  m_LoggingEvent.AddEventHandler(handler);
}

void ezAssetProcessorLog::RemoveLogWriter(ezLoggingEvent::Handler handler)
{
  m_LoggingEvent.RemoveEventHandler(handler);
}


////////////////////////////////////////////////////////////////////////
// ezAssetProcessor
////////////////////////////////////////////////////////////////////////

ezAssetProcessor::ezAssetProcessor()
  : m_SingletonRegistrar(this)
{
  ezAssetCurator::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezAssetProcessor::AssetCuratorEventHandler, this));
}


ezAssetProcessor::~ezAssetProcessor()
{
  ezAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezAssetProcessor::AssetCuratorEventHandler, this));
}

void ezAssetProcessor::RestartProcessTask()
{
  EZ_LOCK(m_ProcessorMutex);
  m_bRunProcessTask = 1;

  RunNextProcessTask();

  {
    ezAssetProcessorEvent e;
    e.m_Type = ezAssetProcessorEvent::Type::ProcessTaskStateChanged;
    m_Events.Broadcast(e);
  }
}

void ezAssetProcessor::ShutdownProcessTask()
{
  ezDynamicArray<ezProcessTask*> Tasks;
  {
    EZ_LOCK(m_ProcessorMutex);
    Tasks = m_ProcessTasks;
    m_ProcessTasks.Clear();
    m_bRunProcessTask = 0;
  }

  if (!Tasks.IsEmpty())
  {
    for (ezProcessTask* pTask : Tasks)
    {
      ezTaskSystem::WaitForTask((ezTask*)pTask);

      // Delete and remove under lock.
      EZ_LOCK(m_ProcessorMutex);
      EZ_DEFAULT_DELETE(pTask);
    }
  }

  {
    ezAssetProcessorEvent e;
    e.m_Type = ezAssetProcessorEvent::Type::ProcessTaskStateChanged;
    m_Events.Broadcast(e);
  }
}

bool ezAssetProcessor::IsProcessTaskRunning() const
{
  return m_bRunProcessTask != 0;
}

void ezAssetProcessor::AddLogWriter(ezLoggingEvent::Handler handler)
{
  m_CuratorLog.AddLogWriter(handler);
}

void ezAssetProcessor::RemoveLogWriter(ezLoggingEvent::Handler handler)
{
  m_CuratorLog.RemoveLogWriter(handler);
}

void ezAssetProcessor::OnProcessTaskFinished(ezTask* pTask)
{
  RunNextProcessTask();
}


void ezAssetProcessor::RunNextProcessTask()
{
  ezUInt32 uiNumAssets;
  ezHybridArray<ezUInt32, ezAssetInfo::TransformState::COUNT> sections;
  ezAssetCurator::GetSingleton()->GetAssetTransformStats(uiNumAssets, sections);
  if (m_bRunProcessTask == 0 || sections[ezAssetInfo::TransformState::NeedsTransform] == 0 && sections[ezAssetInfo::TransformState::NeedsThumbnail] == 0)
    return;

  ezLock<ezMutex> ml(m_ProcessorMutex);

  if (m_ProcessTasks.IsEmpty())
  {
    const ezUInt32 uiWorkerCount = ezTaskSystem::GetWorkerThreadCount(ezWorkerThreadType::LongTasks);
    for (ezUInt32 i = 0; i < uiWorkerCount; ++i)
    {
      ezProcessTask* pTask = EZ_DEFAULT_NEW(ezProcessTask, i);
      pTask->SetOnTaskFinished(ezMakeDelegate(&ezAssetProcessor::OnProcessTaskFinished, this));
      m_ProcessTasks.PushBack(pTask);
    }
  }

  // Even if there is work left, it could be that after further examining (that we don't want to do on this thread)
  // it turns out that there was nothing to do (errors, only manual transform assets).
  // If all threads decide that we are done until an asset state is changed which resets m_TicksWithIdleTasks.
  bool bAllIdle = true;
  for (ezUInt32 i = 0; i < m_ProcessTasks.GetCount(); ++i)
  {
    if (m_ProcessTasks[i]->m_bDidWork)
    {
      bAllIdle = false;
    }
  }
  if (bAllIdle)
  {
    if (m_TicksWithIdleTasks > 5)
      return;
    m_TicksWithIdleTasks.Increment();
  }

  for (ezUInt32 i = 0; i < m_ProcessTasks.GetCount(); ++i)
  {
    if (m_ProcessTasks[i]->IsTaskFinished())
    {
      ezTaskSystem::StartSingleTask(m_ProcessTasks[i], ezTaskPriority::LongRunning);
    }
  }
}


void ezAssetProcessor::AssetCuratorEventHandler(const ezAssetCuratorEvent& e)
{
  if (e.m_Type == ezAssetCuratorEvent::Type::AssetUpdated)
  {
    m_TicksWithIdleTasks = 0;
  }
}


////////////////////////////////////////////////////////////////////////
// ezProcessTask
////////////////////////////////////////////////////////////////////////

ezProcessTask::ezProcessTask(ezUInt32 uiProcessorID)
  : m_uiProcessorID(uiProcessorID), m_bProcessShouldBeRunning(false), m_bProcessCrashed(false), m_bWaiting(false), m_bSuccess(true)
{
  SetTaskName("ezProcessTask");
  m_pIPC = EZ_DEFAULT_NEW(ezEditorProcessCommunicationChannel);
  m_pIPC->m_Events.AddEventHandler(ezMakeDelegate(&ezProcessTask::EventHandlerIPC, this));
}

ezProcessTask::~ezProcessTask()
{
  ShutdownProcess();
  m_pIPC->m_Events.RemoveEventHandler(ezMakeDelegate(&ezProcessTask::EventHandlerIPC, this));
  EZ_DEFAULT_DELETE(m_pIPC);
}

void ezProcessTask::StartProcess()
{
  const ezRTTI* pFirstAllowedMessageType = nullptr;
  m_bProcessShouldBeRunning = true;
  m_bProcessCrashed = false;

  QStringList args;
  args << "-appname";
  args << "ezEditor";
  args << "-appid";
  args << QString::number(m_uiProcessorID);
  args << "-project";
  args << ezToolsProject::GetSingleton()->GetProjectFile().GetData();

  if (m_pIPC->StartClientProcess("EditorProcessor.exe", args, false, pFirstAllowedMessageType).Failed())
  {
    m_bProcessCrashed = true;
  }
}

void ezProcessTask::ShutdownProcess()
{
  if (!m_bProcessShouldBeRunning)
    return;

  m_bProcessShouldBeRunning = false;
  m_pIPC->CloseConnection();
}

void ezProcessTask::EventHandlerIPC(const ezProcessCommunicationChannel::Event& e)
{
  if (const ezProcessAssetResponse* pMsg = ezDynamicCast<const ezProcessAssetResponse*>(e.m_pMessage))
  {
    m_bSuccess = pMsg->m_bSuccess;
    m_bWaiting = false;
    m_LogEntries.Swap(pMsg->m_LogEntries);
  }
}

bool ezProcessTask::GetNextAssetToProcess(ezAssetInfo* pInfo, ezUuid& out_guid, ezStringBuilder& out_sAbsPath)
{
  bool bComplete = true;

  const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (ezDocumentManager::FindDocumentTypeFromPath(pInfo->m_sAbsolutePath, false, pTypeDesc).Succeeded())
  {
    auto flags = static_cast<ezAssetDocumentManager*>(pTypeDesc->m_pManager)->GetAssetDocumentTypeFlags(pTypeDesc);
    if (flags.IsAnySet(ezAssetDocumentFlags::OnlyTransformManually | ezAssetDocumentFlags::DisableTransform))
      return false;

  }

  auto TestFunc = [this, &bComplete](const ezSet<ezString>& Files) -> ezAssetInfo*
  {
    for (const auto& sFile : Files)
    {
      if (ezAssetInfo* pFileInfo = ezAssetCurator::GetSingleton()->GetAssetInfo(sFile))
      {
        switch (pFileInfo->m_TransformState)
        {
        case ezAssetInfo::TransformState::Unknown:
        case ezAssetInfo::TransformState::Updating:
        case ezAssetInfo::TransformState::TransformError:
        case ezAssetInfo::TransformState::MissingDependency:
        case ezAssetInfo::TransformState::MissingReference:
          {
            bComplete = false;
            continue;
          }
        case ezAssetInfo::TransformState::NeedsTransform:
        case ezAssetInfo::TransformState::NeedsThumbnail:
          {
            bComplete = false;
            return pFileInfo;
          }
        case ezAssetInfo::TransformState::UpToDate:
          continue;
        }
      }
    }
    return nullptr;
  };

  if (ezAssetInfo* pDepInfo = TestFunc(pInfo->m_Info->m_AssetTransformDependencies))
  {
    return GetNextAssetToProcess(pDepInfo, out_guid, out_sAbsPath);
  }

  if (ezAssetInfo* pDepInfo = TestFunc(pInfo->m_Info->m_RuntimeDependencies))
  {
    return GetNextAssetToProcess(pDepInfo, out_guid, out_sAbsPath);
  }

  if (bComplete)
  {
    out_guid = pInfo->m_Info->m_DocumentID;
    out_sAbsPath = pInfo->m_sAbsolutePath;
    return true;
  }

  return false;
}

bool ezProcessTask::GetNextAssetToProcess(ezUuid& out_guid, ezStringBuilder& out_sAbsPath)
{
  EZ_LOCK(ezAssetCurator::GetSingleton()->m_CuratorMutex);

  for (auto it = ezAssetCurator::GetSingleton()->m_TransformState[ezAssetInfo::TransformState::NeedsTransform].GetIterator(); it.IsValid(); ++it)
  {
    ezAssetInfo* pInfo = ezAssetCurator::GetSingleton()->GetAssetInfo(it.Key());
    if (pInfo)
    {
      bool bRes = GetNextAssetToProcess(pInfo, out_guid, out_sAbsPath);
      if (bRes)
        return true;
    }
  }

  for (auto it = ezAssetCurator::GetSingleton()->m_TransformState[ezAssetInfo::TransformState::NeedsThumbnail].GetIterator(); it.IsValid(); ++it)
  {
    ezAssetInfo* pInfo = ezAssetCurator::GetSingleton()->GetAssetInfo(it.Key());
    if (pInfo)
    {
      bool bRes = GetNextAssetToProcess(pInfo, out_guid, out_sAbsPath);
      if (bRes)
        return true;
    }
  }

  return false;
}


void ezProcessTask::OnProcessCrashed()
{
  m_bSuccess = false;
  ezLogEntryDelegate logger([this](ezLogEntry& entry) { m_LogEntries.PushBack(std::move(entry)); });
  ezLog::Error(&logger, "AssetProcessor crashed!");
  ezLog::Error(&ezAssetProcessor::GetSingleton()->m_CuratorLog, "AssetProcessor crashed!");
}

void ezProcessTask::Execute()
{
  m_LogEntries.Clear();
  m_bSuccess = true;
  {
    EZ_LOCK(ezAssetCurator::GetSingleton()->m_CuratorMutex);

    if (!GetNextAssetToProcess(m_assetGuid, m_sAssetPath))
    {
      m_assetGuid = ezUuid();
      m_sAssetPath.Clear();
      m_bDidWork = false;
      return;
    }
    m_bDidWork = true;
    ezAssetCurator::GetSingleton()->UpdateAssetTransformState(m_assetGuid, ezAssetInfo::TransformState::Updating);
  }

  if (!m_bProcessShouldBeRunning)
  {
    StartProcess();
  }

  if (m_bProcessCrashed)
  {
    OnProcessCrashed();
  }
  else
  {
    ezLog::Info(&ezAssetProcessor::GetSingleton()->m_CuratorLog, "Processing '{0}'", m_sAssetPath);
    // Send and wait
    ezProcessAsset msg;
    msg.m_AssetGuid = m_assetGuid;
    msg.m_sAssetPath = m_sAssetPath;
    m_pIPC->SendMessage(&msg);
    m_bWaiting = true;

    while (m_bWaiting)
    {
      if (m_bProcessCrashed)
      {
        m_bWaiting = false;
        OnProcessCrashed();
        break;
      }
      m_pIPC->ProcessMessages();
      ezThreadUtils::Sleep(ezTime::Milliseconds(10));
    }
  }

  if (m_bSuccess)
  {
    ezAssetCurator::GetSingleton()->NotifyOfAssetChange(m_assetGuid);
    ezAssetCurator::GetSingleton()->m_bNeedToReloadResources = true;
  }
  else
  {
    ezAssetCurator::GetSingleton()->UpdateAssetTransformLog(m_assetGuid, m_LogEntries);
    ezAssetCurator::GetSingleton()->UpdateAssetTransformState(m_assetGuid, ezAssetInfo::TransformState::TransformError);
  }
}
