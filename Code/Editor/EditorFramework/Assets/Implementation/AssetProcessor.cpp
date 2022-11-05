#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Configuration/SubSystem.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

EZ_IMPLEMENT_SINGLETON(ezAssetProcessor);

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, AssetProcessor)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "AssetCurator"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    EZ_DEFAULT_NEW(ezAssetProcessor);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezAssetProcessor* pDummy = ezAssetProcessor::GetSingleton();
    EZ_DEFAULT_DELETE(pDummy);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

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
}


ezAssetProcessor::~ezAssetProcessor()
{
}

void ezAssetProcessor::RestartProcessTask()
{
  EZ_LOCK(m_ProcessorMutex);
  m_bRunProcessTask = true;

  const ezUInt32 uiWorkerCount = ezTaskSystem::GetWorkerThreadCount(ezWorkerThreadType::LongTasks);
  m_ProcessRunning.SetCount(uiWorkerCount, false);
  m_ProcessTasks.SetCount(uiWorkerCount);

  for (ezUInt32 idx = 0; idx < uiWorkerCount; ++idx)
  {
    m_ProcessTasks[idx].m_uiProcessorID = idx;
  }

  m_Thread.Start();

  {
    ezAssetProcessorEvent e;
    e.m_Type = ezAssetProcessorEvent::Type::ProcessTaskStateChanged;
    m_Events.Broadcast(e);
  }
}

void ezAssetProcessor::ShutdownProcessTask()
{
  m_bRunProcessTask = false;
  m_Thread.Join();

  m_ProcessRunning.Clear();
  m_ProcessTasks.Clear();

  {
    ezAssetProcessorEvent e;
    e.m_Type = ezAssetProcessorEvent::Type::ProcessTaskStateChanged;
    m_Events.Broadcast(e);
  }
}

bool ezAssetProcessor::IsProcessTaskRunning() const
{
  return m_bRunProcessTask;
}

void ezAssetProcessor::AddLogWriter(ezLoggingEvent::Handler handler)
{
  m_CuratorLog.AddLogWriter(handler);
}

void ezAssetProcessor::RemoveLogWriter(ezLoggingEvent::Handler handler)
{
  m_CuratorLog.RemoveLogWriter(handler);
}

void ezAssetProcessor::Run()
{
  while (m_bRunProcessTask)
  {
    for (ezUInt32 i = 0; i < m_ProcessTasks.GetCount(); i++)
    {
      if (m_ProcessRunning[i])
      {
        m_ProcessRunning[i] = !m_ProcessTasks[i].FinishExecute();
      }
      else
      {
        m_ProcessRunning[i] = m_ProcessTasks[i].BeginExecute();
      }
    }
    ezThreadUtils::Sleep(ezTime::Milliseconds(100));
  }

  while (true)
  {
    bool bAnyRunning = false;

    for (ezUInt32 i = 0; i < m_ProcessTasks.GetCount(); i++)
    {
      if (m_ProcessRunning[i])
      {
        m_ProcessRunning[i] = !m_ProcessTasks[i].FinishExecute();
        bAnyRunning |= m_ProcessRunning[i];
      }
    }

    if (bAnyRunning)
      ezThreadUtils::Sleep(ezTime::Milliseconds(100));
    else
      break;
  }
}


////////////////////////////////////////////////////////////////////////
// ezProcessTask
////////////////////////////////////////////////////////////////////////

ezProcessTask::ezProcessTask()
  : m_bProcessShouldBeRunning(false)
  , m_bProcessCrashed(false)
  , m_bWaiting(false)
  , m_Status(EZ_SUCCESS)
{
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
  args << ezApplication::GetApplicationInstance()->GetApplicationName().GetData();
  args << "-appid";
  args << QString::number(m_uiProcessorID);
  args << "-project";
  args << ezToolsProject::GetSingleton()->GetProjectFile().GetData();
  args << "-renderer";
  args << ezGameApplication::GetActiveRenderer();

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  const char* EditorProcessorExecutable = "EditorProcessor.exe";
#else
  const char* EditorProcessorExecutable = "EditorProcessor";
#endif

  if (m_pIPC->StartClientProcess(EditorProcessorExecutable, args, false, pFirstAllowedMessageType).Failed())
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
  if (const ezProcessAssetResponseMsg* pMsg = ezDynamicCast<const ezProcessAssetResponseMsg*>(e.m_pMessage))
  {
    m_Status = ezStatus(pMsg->m_bSuccess ? EZ_SUCCESS : EZ_FAILURE, pMsg->m_sStatus);
    m_bWaiting = false;
    m_LogEntries.Swap(pMsg->m_LogEntries);
  }
}

bool ezProcessTask::GetNextAssetToProcess(ezAssetInfo* pInfo, ezUuid& out_guid, ezStringBuilder& out_sAbsPath, ezStringBuilder& out_sRelPath)
{
  bool bComplete = true;

  const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (ezDocumentManager::FindDocumentTypeFromPath(pInfo->m_sAbsolutePath, false, pTypeDesc).Succeeded())
  {
    auto flags = static_cast<const ezAssetDocumentTypeDescriptor*>(pTypeDesc)->m_AssetDocumentFlags;

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

          case ezAssetInfo::TransformState::NeedsImport:
            // the main processor has to do this itself
            continue;

            EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
        }
      }
    }
    return nullptr;
  };

  if (ezAssetInfo* pDepInfo = TestFunc(pInfo->m_Info->m_AssetTransformDependencies))
  {
    return GetNextAssetToProcess(pDepInfo, out_guid, out_sAbsPath, out_sRelPath);
  }

  if (ezAssetInfo* pDepInfo = TestFunc(pInfo->m_Info->m_RuntimeDependencies))
  {
    return GetNextAssetToProcess(pDepInfo, out_guid, out_sAbsPath, out_sRelPath);
  }

  if (bComplete && !ezAssetCurator::GetSingleton()->m_Updating.Contains(pInfo->m_Info->m_DocumentID) &&
      !ezAssetCurator::GetSingleton()->m_TransformStateStale.Contains(pInfo->m_Info->m_DocumentID))
  {
    ezAssetCurator::GetSingleton()->m_Updating.Insert(pInfo->m_Info->m_DocumentID);
    out_guid = pInfo->m_Info->m_DocumentID;
    out_sAbsPath = pInfo->m_sAbsolutePath;
    out_sRelPath = pInfo->m_sDataDirParentRelativePath;
    return true;
  }

  return false;
}

bool ezProcessTask::GetNextAssetToProcess(ezUuid& out_guid, ezStringBuilder& out_sAbsPath, ezStringBuilder& out_sRelPath)
{
  EZ_LOCK(ezAssetCurator::GetSingleton()->m_CuratorMutex);

  for (auto it = ezAssetCurator::GetSingleton()->m_TransformState[ezAssetInfo::TransformState::NeedsTransform].GetIterator(); it.IsValid(); ++it)
  {
    ezAssetInfo* pInfo = ezAssetCurator::GetSingleton()->GetAssetInfo(it.Key());
    if (pInfo)
    {
      bool bRes = GetNextAssetToProcess(pInfo, out_guid, out_sAbsPath, out_sRelPath);
      if (bRes)
        return true;
    }
  }

  for (auto it = ezAssetCurator::GetSingleton()->m_TransformState[ezAssetInfo::TransformState::NeedsThumbnail].GetIterator(); it.IsValid(); ++it)
  {
    ezAssetInfo* pInfo = ezAssetCurator::GetSingleton()->GetAssetInfo(it.Key());
    if (pInfo)
    {
      bool bRes = GetNextAssetToProcess(pInfo, out_guid, out_sAbsPath, out_sRelPath);
      if (bRes)
        return true;
    }
  }

  return false;
}


void ezProcessTask::OnProcessCrashed()
{
  m_Status = ezStatus("Asset processor crashed");
  ezLogEntryDelegate logger([this](ezLogEntry& entry)
    { m_LogEntries.PushBack(std::move(entry)); });
  ezLog::Error(&logger, "AssetProcessor crashed!");
  ezLog::Error(&ezAssetProcessor::GetSingleton()->m_CuratorLog, "AssetProcessor crashed!");
}

bool ezProcessTask::BeginExecute()
{
  ezStringBuilder sAssetRelPath;

  m_LogEntries.Clear();
  m_TransitiveHull.Clear();
  m_Status = ezStatus(EZ_SUCCESS);
  {
    EZ_LOCK(ezAssetCurator::GetSingleton()->m_CuratorMutex);

    if (!GetNextAssetToProcess(m_AssetGuid, m_sAssetPath, sAssetRelPath))
    {
      m_AssetGuid = ezUuid();
      m_sAssetPath.Clear();
      m_bDidWork = false;
      return false;
    }

    m_bDidWork = true;
    ezAssetInfo::TransformState state = ezAssetCurator::GetSingleton()->IsAssetUpToDate(m_AssetGuid, nullptr, nullptr, m_uiAssetHash, m_uiThumbHash);
    EZ_ASSERT_DEV(state == ezAssetInfo::TransformState::NeedsTransform || state == ezAssetInfo::TransformState::NeedsThumbnail, "An asset was selected that is already up to date.");

    ezSet<ezString> dependencies;

    ezStringBuilder sTemp;
    ezAssetCurator::GetSingleton()->GenerateTransitiveHull(ezConversionUtils::ToString(m_AssetGuid, sTemp), &dependencies, &dependencies);

    m_TransitiveHull.Reserve(dependencies.GetCount());
    for (const ezString& str : dependencies)
    {
      m_TransitiveHull.PushBack(str);
    }
  }

  if (!m_bProcessShouldBeRunning)
  {
    StartProcess();
  }

  if (m_bProcessCrashed)
  {
    OnProcessCrashed();
    return false;
  }
  else
  {
    ezLog::Info(&ezAssetProcessor::GetSingleton()->m_CuratorLog, "Processing '{0}'", sAssetRelPath);
    // Send and wait
    ezProcessAssetMsg msg;
    msg.m_AssetGuid = m_AssetGuid;
    msg.m_AssetHash = m_uiAssetHash;
    msg.m_ThumbHash = m_uiThumbHash;
    msg.m_sAssetPath = m_sAssetPath;
    msg.m_DepRefHull.Swap(m_TransitiveHull);
    msg.m_sPlatform = ezAssetCurator::GetSingleton()->GetActiveAssetProfile()->GetConfigName();

    m_pIPC->SendMessage(&msg);
    m_bWaiting = true;
    return true;
  }
}

bool ezProcessTask::FinishExecute()
{
  if (m_bWaiting)
  {
    m_pIPC->ProcessMessages();
    if (!m_pIPC->IsClientAlive())
    {
      m_bProcessCrashed = true;
    }

    if (m_bProcessCrashed)
    {
      m_bWaiting = false;
      OnProcessCrashed();
    }
    if (m_bWaiting)
      return false;
  }

  if (m_Status.Succeeded())
  {
    ezAssetCurator::GetSingleton()->NotifyOfAssetChange(m_AssetGuid);
    ezAssetCurator::GetSingleton()->NeedsReloadResources();
  }
  else
  {
    if (m_Status.m_sMessage == "IMPORT NEEDED!")
    {
      ezAssetCurator::GetSingleton()->UpdateAssetTransformState(m_AssetGuid, ezAssetInfo::TransformState::NeedsImport);
    }
    else
    {
      ezAssetCurator::GetSingleton()->UpdateAssetTransformLog(m_AssetGuid, m_LogEntries);
      ezAssetCurator::GetSingleton()->UpdateAssetTransformState(m_AssetGuid, ezAssetInfo::TransformState::TransformError);
    }
  }

  EZ_LOCK(ezAssetCurator::GetSingleton()->m_CuratorMutex);
  ezAssetCurator::GetSingleton()->m_Updating.Remove(m_AssetGuid);
  return true;
}

ezUInt32 ezProcessThread::Run()
{
  ezAssetProcessor::GetSingleton()->Run();
  return 0;
}
