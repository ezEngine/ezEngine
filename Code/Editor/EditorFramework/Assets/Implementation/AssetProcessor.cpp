#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/Assets/AssetProcessorMessages.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Configuration/SubSystem.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Project/ToolsProject.h>

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
  if (m_pThread)
  {
    m_pThread->Join();
    m_pThread.Clear();
  }
  EZ_ASSERT_DEV(m_ProcessorState == ProcessorState::Stopped, "Call StopProcessor first before destroying the ezAssetProcessor.");
}

void ezAssetProcessor::StartProcessor()
{
  EZ_LOCK(m_ProcessorMutex);
  if (m_ProcessorState != ProcessorState::Stopped)
  {
    return;
  }

  // Join old thread.
  if (m_pThread)
  {
    m_pThread->Join();
    m_pThread.Clear();
  }

  m_ProcessorState = ProcessorState::Running;

  ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();

  const ezUInt32 uiWorkerCount = ezMath::Min<ezUInt32>(ezTaskSystem::GetWorkerThreadCount(ezWorkerThreadType::LongTasks), pPreferences->m_uiMaxAssetProcessors);
  m_Processes.SetCount(uiWorkerCount);
  m_EditorProcessorStates.SetCount(uiWorkerCount);
  m_RestartRequests.SetCount(uiWorkerCount);

  for (ezUInt32 idx = 0; idx < uiWorkerCount; ++idx)
  {
    m_Processes[idx].m_uiProcessorID = idx;
    m_Processes[idx].m_pNewWorkSignal = &m_NewWorkSignal;
  }

  m_pThread = EZ_DEFAULT_NEW(ezAssetProcessorThread);
  m_pThread->Start();

  {
    ezAssetProcessorEvent e;
    e.m_Type = ezAssetProcessorEvent::Type::AssetProcessorStateChanged;
    e.m_uiProcessCount = m_EditorProcessorStates.GetCount();
    m_Events.Broadcast(e);
  }
}

void ezAssetProcessor::StopProcessor(bool bForce)
{
  {
    EZ_LOCK(m_ProcessorMutex);
    switch (m_ProcessorState)
    {
      case ProcessorState::Running:
      {
        m_ProcessorState = ProcessorState::Stopping;
        {
          ezAssetProcessorEvent e;
          e.m_Type = ezAssetProcessorEvent::Type::AssetProcessorStateChanged;
          e.m_uiProcessCount = m_EditorProcessorStates.GetCount();
          m_Events.Broadcast(e);

          // Make sure worker thread is woken up.
          m_NewWorkSignal.RaiseSignal();
        }
      }
      break;
      case ProcessorState::Stopping:
        if (!bForce)
          return;
        break;
      default:
      case ProcessorState::Stopped:
        return;
    }
  }

  if (bForce)
  {
    m_bForceStop = true;
    m_pThread->Join();
    m_pThread.Clear();
    EZ_ASSERT_DEV(m_ProcessorState == ProcessorState::Stopped, "Process task shoul have set the state to stopped.");
  }
}

ezUInt32 ezAssetProcessor::GetProcessCount() const
{
  EZ_LOCK(m_ProcessorMutex);
  return m_EditorProcessorStates.GetCount();
}

ezEditorProcessorState ezAssetProcessor::GetProcessState(ezUInt32 uiProcessIndex) const
{
  EZ_LOCK(m_ProcessorMutex);
  if (uiProcessIndex < m_EditorProcessorStates.GetCount())
  {
    return m_EditorProcessorStates[uiProcessIndex];
  }
  return {};
}

void ezAssetProcessor::RequestRestartProcess(ezUInt32 uiProcessIndex)
{
  EZ_LOCK(m_ProcessorMutex);
  if (uiProcessIndex < m_RestartRequests.GetCount())
  {
    m_RestartRequests[uiProcessIndex].Set(true);
    m_NewWorkSignal.RaiseSignal();
  }
}

void ezAssetProcessor::AddLogWriter(ezLoggingEvent::Handler handler)
{
  m_CuratorLog.AddLogWriter(handler);
}

void ezAssetProcessor::RemoveLogWriter(ezLoggingEvent::Handler handler)
{
  m_CuratorLog.RemoveLogWriter(handler);
}

void ezAssetProcessor::UpdateProcessStates()
{
  ezTempHybridArray<ezUInt8, 8> changedProcesses;
  {
    EZ_LOCK(m_ProcessorMutex);
    for (ezUInt32 i = 0; i < m_Processes.GetCount(); i++)
    {
      ezEditorProcessorState state;
      state.m_bConnected = m_Processes[i].IsConnected();
      state.m_bRunning = m_Processes[i].IsRunning();
      state.m_bCrashed = m_Processes[i].IsCrashed();
      state.m_uiProcessID = m_Processes[i].GetProcessId();

      if (m_EditorProcessorStates[i] != state)
      {
        m_EditorProcessorStates[i] = state;
        changedProcesses.PushBack(i);
      }
    }
  }
  for (ezUInt8 uiProcessId : changedProcesses)
  {
    ezAssetProcessorEvent e;
    e.m_Type = ezAssetProcessorEvent::Type::ProcessStateChanged;
    e.m_uiProcessorID = uiProcessId;
    m_Events.Broadcast(e);
  }
}

void ezAssetProcessor::Run()
{
  QEventLoop loop;
  while (m_ProcessorState == ProcessorState::Running)
  {
    loop.processEvents(QEventLoop::AllEvents);
    if (m_iPauseProcessing == 0)
    {
      EZ_PROFILE_SCOPE("ezAssetProcessor::Run");

      // Check for restart requests
      for (ezUInt32 i = 0; i < m_Processes.GetCount(); i++)
      {
        if (m_RestartRequests[i].TestAndSet(true, false))
        {
          m_Processes[i].RequestRestart();
        }
      }

      for (ezUInt32 i = 0; i < m_Processes.GetCount(); i++)
      {
        m_Processes[i].Tick(true);
      }
      UpdateProcessStates();
    }
    m_NewWorkSignal.WaitForSignal(ezTime::MakeFromSeconds(1));
  }

  while (true)
  {
    bool bAnyRunning = false;
    loop.processEvents(QEventLoop::AllEvents);
    for (ezUInt32 i = 0; i < m_Processes.GetCount(); i++)
    {
      if (m_bForceStop)
        m_Processes[i].ShutdownProcess();

      bAnyRunning |= m_Processes[i].Tick(false);
    }

    if (bAnyRunning)
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(100));
    else
      break;
  }

  ezUInt32 uiProcesses = 0;
  {
    EZ_LOCK(m_ProcessorMutex);
    uiProcesses = m_Processes.GetCount();
    m_Processes.Clear();
    m_EditorProcessorStates.Clear();
    m_ProcessorState = ProcessorState::Stopped;
    m_bForceStop = false;
  }

  for (ezUInt8 uiProcessId = 0; uiProcessId < uiProcesses; uiProcessId++)
  {
    ezAssetProcessorEvent e;
    e.m_Type = ezAssetProcessorEvent::Type::ProcessStateChanged;
    e.m_uiProcessorID = uiProcessId;
    m_Events.Broadcast(e);
  }

  {
    ezAssetProcessorEvent e;
    e.m_Type = ezAssetProcessorEvent::Type::AssetProcessorStateChanged;
    e.m_uiProcessCount = m_EditorProcessorStates.GetCount();
    m_Events.Broadcast(e);
  }
}


////////////////////////////////////////////////////////////////////////
// ezProcessTask
////////////////////////////////////////////////////////////////////////

ezEditorProcessorProcess::ezEditorProcessorProcess()
  : m_Status(EZ_SUCCESS)
{
  m_pIPC = EZ_DEFAULT_NEW(ezEditorProcessCommunicationChannel);
  m_pIPC->m_Events.AddEventHandler(ezMakeDelegate(&ezEditorProcessorProcess::EventHandlerIPC, this));
  m_pIPC->m_IpcChannelEvents.AddEventHandler(ezMakeDelegate(&ezEditorProcessorProcess::ChannelEventHandler, this));
}

ezEditorProcessorProcess::~ezEditorProcessorProcess()
{
  ShutdownProcess();
  m_pIPC->m_IpcChannelEvents.RemoveEventHandler(ezMakeDelegate(&ezEditorProcessorProcess::ChannelEventHandler, this));
  m_pIPC->m_Events.RemoveEventHandler(ezMakeDelegate(&ezEditorProcessorProcess::EventHandlerIPC, this));
  EZ_DEFAULT_DELETE(m_pIPC);
}


ezResult ezEditorProcessorProcess::StartProcess()
{
  const ezRTTI* pFirstAllowedMessageType = nullptr;

  ezStringBuilder tmp;

  QStringList args;
  args << "-appname";
  args << ezApplication::GetApplicationInstance()->GetApplicationName().GetData();
  args << "-appid";
  args << QString::number(m_uiProcessorID);
  args << "-project";
  args << ezToolsProject::GetSingleton()->GetProjectFile().GetData();
  args << "-renderer";
  args << ezGameApplication::GetActiveRenderer().GetData(tmp);
  {
    ezStringBuilder sRelativeData;
    sRelativeData = ":APPDATA";

    ezStringBuilder sAbsoluteData;
    ezFileSystem::ResolvePath(sRelativeData, &sAbsoluteData, nullptr).AssertSuccess("Failed to resolve APPDATA dir!");

    args << "-outputDir";
    args << sAbsoluteData.GetData();
  }

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  const char* EditorProcessorExecutable = "ezEditorProcessor.exe";
#else
  const char* EditorProcessorExecutable = "ezEditorProcessor";
#endif

  if (m_pIPC->StartClientProcess(EditorProcessorExecutable, args, false, pFirstAllowedMessageType).Failed())
  {
    return EZ_FAILURE;
  }
  m_CurrentProcessID = m_pIPC->GetProcessId();
  return EZ_SUCCESS;
}

void ezEditorProcessorProcess::ShutdownProcess()
{
  m_pIPC->CloseConnection();
}

void ezEditorProcessorProcess::EventHandlerIPC(const ezProcessCommunicationChannel::Event& e)
{
  if (const ezProcessAssetResponseMsg* pMsg = ezDynamicCast<const ezProcessAssetResponseMsg*>(e.m_pMessage))
  {
    EZ_ASSERT_DEV(m_State == State::Processing, "Message handling should only happen when currently processing");
    m_State = State::ReportResult;
    m_Status = pMsg->m_Status;
    m_LogEntries.Swap(pMsg->m_LogEntries);
    m_MissmatchTransformDependencies.Swap(pMsg->m_MissmatchTransformDependencies);
    m_MissmatchThumbnailDependencies.Swap(pMsg->m_MissmatchThumbnailDependencies);
    m_uiMissmatchAssetHash = pMsg->m_uiMissmatchAssetHash;
    m_uiMissmatchThumbHash = pMsg->m_uiMissmatchThumbHash;
    m_StartedProcessing = pMsg->m_StartedProcessing;
    m_StartedTransform = pMsg->m_StartedTransform;
    m_FinishedProcessing = pMsg->m_FinishedProcessing;
  }
}

void ezEditorProcessorProcess::ChannelEventHandler(const ezIpcChannelEvent& e)
{
  if (m_pNewWorkSignal)
  {
    m_pNewWorkSignal->RaiseSignal();
  }
}

bool ezEditorProcessorProcess::GetNextAssetToProcess(ezAssetInfo* pInfo, ezUuid& out_guid, ezDataDirPath& out_path, ezAssetInfo::TransformState& out_transformState)
{
  bool bComplete = true;

  const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (ezDocumentManager::FindDocumentTypeFromPath(pInfo->m_Path, false, pTypeDesc).Succeeded())
  {
    auto flags = static_cast<const ezAssetDocumentTypeDescriptor*>(pTypeDesc)->m_AssetDocumentFlags;

    if (flags.IsAnySet(ezAssetDocumentFlags::OnlyTransformManually | ezAssetDocumentFlags::DisableTransform))
      return false;
  }

  auto TestFunc = [this, &bComplete](const ezSet<ezString>& files) -> ezAssetInfo*
  {
    for (const auto& sFile : files)
    {
      if (ezAssetInfo* pFileInfo = ezAssetCurator::GetSingleton()->GetAssetInfo(sFile))
      {
        switch (pFileInfo->m_TransformState)
        {
          case ezAssetInfo::TransformState::Unknown:
          case ezAssetInfo::TransformState::TransformError:
          case ezAssetInfo::TransformState::MissingTransformDependency:
          case ezAssetInfo::TransformState::MissingPackageDependency:
          case ezAssetInfo::TransformState::MissingThumbnailDependency:
          case ezAssetInfo::TransformState::CircularDependency:
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

  if (ezAssetInfo* pDepInfo = TestFunc(pInfo->m_Info->m_TransformDependencies))
  {
    return GetNextAssetToProcess(pDepInfo, out_guid, out_path, out_transformState);
  }

  if (ezAssetInfo* pDepInfo = TestFunc(pInfo->m_Info->m_ThumbnailDependencies))
  {
    return GetNextAssetToProcess(pDepInfo, out_guid, out_path, out_transformState);
  }

  // not needed to go through package dependencies here

  if (bComplete && !ezAssetCurator::GetSingleton()->m_Updating.Contains(pInfo->m_Info->m_DocumentID) &&
      !ezAssetCurator::GetSingleton()->m_TransformStateStale.Contains(pInfo->m_Info->m_DocumentID))
  {
    ezAssetCurator::GetSingleton()->m_Updating.Insert(pInfo->m_Info->m_DocumentID);
    out_guid = pInfo->m_Info->m_DocumentID;
    out_path = pInfo->m_Path;
    out_transformState = pInfo->m_TransformState;
    return true;
  }

  return false;
}

bool ezEditorProcessorProcess::GetNextAssetToProcess(ezUuid& out_guid, ezDataDirPath& out_path, ezAssetInfo::TransformState& out_transformState)
{
  EZ_PROFILE_SCOPE("ezEditorProcessorProcess::GetNextAssetToProcess");
  EZ_LOCK(ezAssetCurator::GetSingleton()->m_CuratorMutex);

  for (auto it = ezAssetCurator::GetSingleton()->m_TransformState[ezAssetInfo::TransformState::NeedsTransform].GetIterator(); it.IsValid(); ++it)
  {
    ezAssetInfo* pInfo = ezAssetCurator::GetSingleton()->GetAssetInfo(it.Key());
    if (pInfo)
    {
      bool bRes = GetNextAssetToProcess(pInfo, out_guid, out_path, out_transformState);
      if (bRes)
        return true;
    }
  }

  for (auto it = ezAssetCurator::GetSingleton()->m_TransformState[ezAssetInfo::TransformState::NeedsThumbnail].GetIterator(); it.IsValid(); ++it)
  {
    ezAssetInfo* pInfo = ezAssetCurator::GetSingleton()->GetAssetInfo(it.Key());
    if (pInfo)
    {
      bool bRes = GetNextAssetToProcess(pInfo, out_guid, out_path, out_transformState);
      if (bRes)
        return true;
    }
  }

  return false;
}


void ezEditorProcessorProcess::OnProcessCrashed(ezStringView message)
{
  ShutdownProcess();
  m_Status = ezStatus(message);
  ezLogEntryDelegate logger([this](ezLogEntry& ref_entry)
    { m_LogEntries.PushBack(std::move(ref_entry)); });
  ezLog::Error(&logger, message);
  ezLog::Error(&ezAssetProcessor::GetSingleton()->m_CuratorLog, message);
  ezLog::Error("EditorProcessor with pid '{}' crashed. Right-click on the crashed instance in the curator panel to go to the crashdumps.", m_CurrentProcessID);
}

void ezEditorProcessorProcess::RequestRestart()
{
  if (m_State == State::Crashed)
  {
    ezLog::Info(&ezAssetProcessor::GetSingleton()->m_CuratorLog, "Restarting crashed processor {}", m_uiProcessorID);
    m_State = State::LookingForWork;
  }
}

bool ezEditorProcessorProcess::IsConnected() const
{
  return m_pIPC->IsConnected();
}

bool ezEditorProcessorProcess::IsRunning() const
{
  return m_State == State::Processing;
}

bool ezEditorProcessorProcess::IsCrashed() const
{
  return m_State == State::Crashed;
}

ezOsProcessID ezEditorProcessorProcess::GetProcessId() const
{
  return m_CurrentProcessID;
}

bool ezEditorProcessorProcess::HasProcessCrashed()
{
  return !m_pIPC->IsClientAlive();
}

void ezEditorProcessorProcess::HandleHashMissmatch()
{
  EZ_SCOPE_EXIT(
    {
      m_MissmatchTransformDependencies.Clear();
      m_MissmatchThumbnailDependencies.Clear();
      m_uiMissmatchAssetHash = 0;
      m_uiMissmatchThumbHash = 0;
    });

  ezUInt64 uiAssetHash = 0;
  ezUInt64 uiThumbHash = 0;
  ezUInt64 uiPackageHash = 0;
  ezMap<ezString, ezUInt64> TransformDependencies;
  ezMap<ezString, ezUInt64> ThumbnailDependencies;
  {
    // Check the asset hash again but force evaluation to detect cases where the file system watcher did not trigger.
    const ezUInt32 uiPlatform = ezAssetCurator::GetSingleton()->FindAssetProfileByName(m_sPlatform);
    ezAssetCurator::GetSingleton()->IsAssetUpToDate(m_AssetGuid, ezAssetCurator::GetSingleton()->GetAssetProfile(uiPlatform), nullptr, uiAssetHash, uiThumbHash, uiPackageHash, true);
    if ((uiAssetHash == m_uiMissmatchAssetHash) || (uiThumbHash == m_uiMissmatchThumbHash))
    {
      // The newly computed hash now matches the one computed by the ezEditorProcessor.
      return;
    }

    ezSet<ezString> dependencies;
    ezAssetCurator::GetSingleton()->GenerateTransitiveHull(m_AssetPath.GetAbsolutePath(), dependencies, ezDependencyFlags::Transform);
    ezAssetCurator::GetSingleton()->GenerateSettingsHashMap(dependencies, ezDependencyFlags::Transform, TransformDependencies);

    dependencies.Clear();
    ezAssetCurator::GetSingleton()->GenerateTransitiveHull(m_AssetPath.GetAbsolutePath(), dependencies, ezDependencyFlags::Thumbnail);
    ezAssetCurator::GetSingleton()->GenerateSettingsHashMap(dependencies, ezDependencyFlags::Thumbnail, ThumbnailDependencies);
  }

  auto AddLogMessage = [&](ezStringView sMsg)
  {
    ezLogEntry le;
    le.m_sMsg = sMsg;
    le.m_Type = ezLogMsgType::WarningMsg;
    m_LogEntries.PushBack(le);
  };

  auto CompareDependencies = [&](const ezMap<ezString, ezUInt64>& dependencies, const ezMap<ezString, ezUInt64>& missmatchDependencies, ezStringView sDependencyType)
  {
    ezStringBuilder sMsg;
    for (auto it = TransformDependencies.GetIterator(); it.IsValid(); ++it)
    {
      const ezString& sKey = it.Key();
      const ezUInt64 uiExpected = it.Value();

      auto itOld = m_MissmatchTransformDependencies.Find(sKey);
      if (itOld.IsValid())
      {
        const ezUInt64 uiOld = itOld.Value();
        if (uiOld != uiExpected)
        {
          sMsg.SetFormat("{} dependency hash mismatch for '{}': expected {} != recorded {}", sDependencyType, sKey, uiExpected, uiOld);
          AddLogMessage(sMsg);
        }
      }
      else
      {
        sMsg.SetFormat("{} dependency missing in records: '{0}' (hash {1})", sDependencyType, sKey, uiExpected);
        AddLogMessage(sMsg);
      }
    }

    // Check for dependencies that used to be recorded but are no longer expected
    for (auto it = m_MissmatchTransformDependencies.GetIterator(); it.IsValid(); ++it)
    {
      if (!TransformDependencies.Contains(it.Key()))
      {
        sMsg.SetFormat("{} dependency no longer present: '{0}' (old hash {1})", sDependencyType, it.Key(), it.Value());
        AddLogMessage(sMsg);
      }
    }
  };
  CompareDependencies(TransformDependencies, m_MissmatchTransformDependencies, "Transform");
  CompareDependencies(ThumbnailDependencies, m_MissmatchThumbnailDependencies, "Thumbnail");
}

bool ezEditorProcessorProcess::Tick(bool bStartNewWork)
{
  EZ_PROFILE_SCOPE("ezEditorProcessorProcess::Tick");
  while (true)
  {
    switch (m_State)
    {
      case State::LookingForWork:
      {
        if (!bStartNewWork)
        {
          return false; // don't call later
        }
        m_ProcessingStartTime = ezTime::MakeZero();
        // Clear asset to process
        m_AssetGuid = {};
        m_AssetPath.Clear();
        m_TransformState = ezAssetInfo::TransformState::Unknown;
        m_sPlatform.Clear();
        m_uiAssetHash = 0;
        m_uiThumbHash = 0;
        m_uiPackageHash = 0;
        m_TransitiveHull.Clear();

        // Clear transform result
        m_Status = ezStatus(EZ_SUCCESS);
        m_LogEntries.Clear();
        m_MissmatchTransformDependencies.Clear();
        m_MissmatchThumbnailDependencies.Clear();
        m_uiMissmatchAssetHash = 0;
        m_uiMissmatchThumbHash = 0;
        m_StartedProcessing = {};
        m_StartedTransform = {};
        m_FinishedProcessing = {};
        {
          auto pCurator = ezAssetCurator::GetSingleton();

          EZ_LOCK(pCurator->m_CuratorMutex);

          if (!GetNextAssetToProcess(m_AssetGuid, m_AssetPath, m_TransformState))
          {
            m_AssetGuid = ezUuid();
            m_AssetPath.Clear();
            if (m_pIPC->IsClientAlive() && m_pIPC->IsConnected() && !m_bIsIdle)
            {
              m_bIsIdle = true;
              // If we have nothing else to do, we might as well free some resource memory the process holds.
              ezFreeAllResourcesMsg msg;
              m_pIPC->SendMessage(&msg);
            }

            return bStartNewWork; // call again if we should be looking for new work
          }
          m_bIsIdle = false;


          ezAssetInfo::TransformState state;
          state = pCurator->IsAssetUpToDate(m_AssetGuid, nullptr, nullptr, m_uiAssetHash, m_uiThumbHash, m_uiPackageHash);
          EZ_ASSERT_DEV(state == ezAssetInfo::TransformState::NeedsTransform || state == ezAssetInfo::TransformState::NeedsThumbnail, "An asset was selected that is already up to date.");
          ezSet<ezString> dependencies;
          ezStringBuilder sTemp;
          pCurator->GenerateTransitiveHull(ezConversionUtils::ToString(m_AssetGuid, sTemp), dependencies, ezDependencyFlags::Transform | ezDependencyFlags::Thumbnail);
          m_sPlatform = ezAssetCurator::GetSingleton()->GetActiveAssetProfile()->GetConfigName();
          m_TransitiveHull.Reserve(dependencies.GetCount());
          for (const ezString& str : dependencies)
          {
            if (ezConversionUtils::IsStringUuid(str))
            {
              if (auto pAsset = pCurator->FindSubAsset(str))
              {
                m_TransitiveHull.PushBack(pAsset->m_pAssetInfo->m_Path.GetAbsolutePath());
              }
            }
            else
            {
              m_TransitiveHull.PushBack(str);
            }
          }
        }

        if (!m_pIPC->IsClientAlive() || !m_pIPC->IsConnected())
        {
          if (StartProcess().Failed())
          {
            m_State = State::ReportResult;
            OnProcessCrashed("Asset processor did not launch");
          }
          else
          {
            m_State = State::WaitingForConnection;
            return true; // call again later
          }
        }
        else
        {
          m_State = State::Ready;
        }
      }
      break;
      case State::WaitingForConnection:
      {
        if (!m_pIPC->IsClientAlive())
        {
          m_State = State::ReportResult;
          OnProcessCrashed("Asset processor crashed while waiting for connection");
          break;
        }

        if (m_pIPC->IsConnected())
        {
          m_State = State::Ready;
        }
      }
      break;
      case State::Ready:
      {
        ezLog::Info(&ezAssetProcessor::GetSingleton()->m_CuratorLog, "Processing '{0}'", m_AssetPath.GetDataDirRelativePath());

        // Fire progress event for processing started
        m_ProcessingStartTime = ezTime::Now();
        {
          ezAssetProcessorProgressEvent e;
          e.m_Type = ezAssetProcessorProgressEvent::Type::ProcessingStarted;
          e.m_TransformState = m_TransformState;
          e.m_uiProcessorID = (ezUInt8)m_uiProcessorID;
          e.m_AssetGuid = m_AssetGuid;
          e.m_sAssetPath = m_AssetPath.GetDataDirRelativePath();
          e.m_StartTime = m_ProcessingStartTime;
          ezAssetProcessor::GetSingleton()->m_ProgressEvents.Broadcast(e);
        }

        // Send and wait
        ezProcessAssetMsg msg;
        msg.m_AssetGuid = m_AssetGuid;
        msg.m_AssetHash = m_uiAssetHash;
        msg.m_ThumbHash = m_uiThumbHash;
        msg.m_PackageHash = m_uiPackageHash;
        msg.m_sAssetPath = m_AssetPath;
        msg.m_DepRefHull.Swap(m_TransitiveHull);
        msg.m_sPlatform = m_sPlatform;

        if (m_pIPC->SendMessage(&msg))
        {
          m_State = State::Processing;
          return true; // call again later
        }
        else
        {
          m_State = State::ReportResult;
          OnProcessCrashed("Asset processor crashed, failed to send message");
        }
      }
      break;
      case State::Processing:
      {
        EZ_PROFILE_SCOPE("ezEditorProcessorProcess::Processing");
        m_pIPC->ProcessMessages();
        if (!m_pIPC->IsClientAlive())
        {
          OnProcessCrashed("Asset Processor crashed during processing");
          m_State = State::ReportResult;
        }
        if (m_State == State::Processing)
          return true; // call again later
      }
      break;
      case State::ReportResult:
      {
        const bool bProcessCrashed = !m_pIPC->IsClientAlive();

        if (!m_MissmatchTransformDependencies.IsEmpty())
        {
          HandleHashMissmatch();
        }

        // Fire progress event only if we actually started processing.
        const bool bDidStartWork = !m_ProcessingStartTime.IsZero();
        if (bDidStartWork)
        {
          // The actual start times are only available if we receive a response message. If we crash, fall back to the range of [m_ProcessingStartTime, now()] as an estimate of the work time.
          ezTime processingEndTime = ezTime::Now();

          ezAssetProcessorProgressEvent e;
          e.m_Type = ezAssetProcessorProgressEvent::Type::ProcessingFinished;
          e.m_uiProcessorID = m_uiProcessorID;
          e.m_AssetGuid = m_AssetGuid;
          e.m_sAssetPath = m_AssetPath.GetDataDirRelativePath();
          e.m_StartTime = bProcessCrashed ? m_ProcessingStartTime : m_StartedProcessing;
          e.m_TransformStartTime = bProcessCrashed ? m_ProcessingStartTime : m_StartedTransform;
          e.m_EndTime = bProcessCrashed ? processingEndTime : m_FinishedProcessing;
          e.m_Result = m_Status;
          ezAssetProcessor::GetSingleton()->m_ProgressEvents.Broadcast(e);
        }

        if (m_Status.Succeeded())
        {
          ezAssetCurator::GetSingleton()->NotifyOfAssetChange(m_AssetGuid);
          ezAssetCurator::GetSingleton()->NeedsReloadResources(m_AssetGuid);
          ezLog::Info(&ezAssetProcessor::GetSingleton()->m_CuratorLog, "Finished '{0}'", m_AssetPath.GetDataDirRelativePath());
        }
        else
        {
          if (m_Status.m_Result == ezTransformResult::NeedsImport)
          {
            ezAssetCurator::GetSingleton()->UpdateAssetTransformState(m_AssetGuid, ezAssetInfo::TransformState::NeedsImport);
            ezLog::Warning(&ezAssetProcessor::GetSingleton()->m_CuratorLog, "Needs Import '{0}'", m_AssetPath.GetDataDirRelativePath());
          }
          else
          {
            ezAssetCurator::GetSingleton()->UpdateAssetTransformLog(m_AssetGuid, m_LogEntries);
            ezAssetCurator::GetSingleton()->UpdateAssetTransformState(m_AssetGuid, ezAssetInfo::TransformState::TransformError);
            if (bProcessCrashed)
            {
              ezLog::Error(&ezAssetProcessor::GetSingleton()->m_CuratorLog, "Failed '{0}' (process crashed)", m_AssetPath.GetDataDirRelativePath());
            }
            else
            {
              ezLog::Error(&ezAssetProcessor::GetSingleton()->m_CuratorLog, "Failed '{0}'", m_AssetPath.GetDataDirRelativePath());
            }
          }
        }

        {
          EZ_LOCK(ezAssetCurator::GetSingleton()->m_CuratorMutex);
          ezAssetCurator::GetSingleton()->m_Updating.Remove(m_AssetGuid);
        }

        if (bProcessCrashed)
        {
          m_State = State::Crashed;
        }
        else
        {
          m_State = State::LookingForWork;
        }
      }
      break;
      case State::Crashed:
      {
        return false;
      }
      break;
    }
  }
}

ezUInt32 ezAssetProcessorThread::Run()
{
  ezAssetProcessor::GetSingleton()->Run();
  return 0;
}
