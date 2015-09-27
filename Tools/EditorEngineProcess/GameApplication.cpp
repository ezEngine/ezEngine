#include <PCH.h>
#include <EditorEngineProcess/GameApplication.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <Core/ResourceManager/ResourceManager.h>

ezEngineProcessGameApplication::ezEngineProcessGameApplication() : ezGameApplication(m_GameState)
{
  m_pApp = nullptr;
}

void ezEngineProcessGameApplication::AfterEngineInit()
{
  ezPlugin::LoadPlugin("ezEnginePluginScene");

  ezGameApplication::AfterEngineInit();

  if (ezCommandLineUtils::GetInstance()->GetBoolOption("-debug"))
  {
    while (!IsDebuggerPresent())
    {
      ezThreadUtils::Sleep(10);
    }
  }

  ezGlobalLog::AddLogWriter(ezMakeDelegate(&ezEngineProcessGameApplication::LogWriter, this));

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  // Setting this flags prevents Windows from showing a dialog when the Engine process crashes
  // this also speeds up process termination significantly (down to less than a second)
  DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
  SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
#endif

  {
    ezStringBuilder sAppDir = ezOSFile::GetApplicationDirectory();
    sAppDir.AppendPath("../../../Shared/Tools/EditorEngineProcess");

    ezOSFile osf;
    osf.CreateDirectoryStructure(sAppDir);

    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
    ezFileSystem::AddDataDirectory("", ezFileSystem::AllowWrites, "App"); // for absolute paths
    ezFileSystem::AddDataDirectory(sAppDir.GetData(), ezFileSystem::AllowWrites, "App"); // for everything relative
  }

  ezTaskSystem::SetTargetFrameTime(1000.0 / 10.0);

  int argc = GetArgumentCount();
  const char** argv = GetArgumentsArray();
  m_pApp = new QApplication(argc, (char**)argv);

  EZ_VERIFY(m_IPC.ConnectToHostProcess().Succeeded(), "Could not connect to host");

  m_IPC.m_Events.AddEventHandler(ezMakeDelegate(&ezEngineProcessGameApplication::EventHandlerIPC, this));

  SendReflectionInformation();

  // wait indefinitely
  m_IPC.WaitForMessage(ezGetStaticRTTI<ezSetupProjectMsgToEngine>(), ezTime());
}

void ezEngineProcessGameApplication::BeforeEngineShutdown()
{
  m_IPC.m_Events.RemoveEventHandler(ezMakeDelegate(&ezEngineProcessGameApplication::EventHandlerIPC, this));

  delete m_pApp;

  ezGlobalLog::RemoveLogWriter(ezMakeDelegate(&ezEngineProcessGameApplication::LogWriter, this));

  ezGameApplication::BeforeEngineShutdown();
}

ezApplication::ApplicationExecution ezEngineProcessGameApplication::Run()
{
  ezRenderLoop::ClearMainViews();

  ProcessIPCMessages();

  if (ezRenderLoop::GetMainViews().GetCount() > 0)
  {
    UpdateWorldsAndRender();
  }
  else
  {
    /// \todo This is not so good
    //ezThreadUtils::Sleep(1);
  }

  return WasQuitRequested() ? ezApplication::Quit : ezApplication::Continue;
}

void ezEngineProcessGameApplication::LogWriter(const ezLoggingEventData & e)
{
  ezLogMsgToEditor msg;
  msg.m_sText = e.m_szText;
  msg.m_sTag = e.m_szTag;
  msg.m_iMsgType = (ezInt8)e.m_EventType;
  msg.m_uiIndentation = e.m_uiIndentation;
  m_IPC.SendMessage(&msg);
}


void ezEngineProcessGameApplication::ProcessIPCMessages()
{
  m_IPC.ProcessMessages();

  if (!m_IPC.IsHostAlive())
    RequestQuit();
}


void ezEngineProcessGameApplication::SendProjectReadyMessage()
{
  ezProjectReadyMsgToEditor msg;
  m_IPC.SendMessage(&msg);
}


void ezEngineProcessGameApplication::SendReflectionInformation()
{
  ezSet<const ezRTTI*> types;
  ezReflectionUtils::GatherTypesDerivedFromClass(ezGetStaticRTTI<ezComponent>(), types, true);
  ezDynamicArray<const ezRTTI*> sortedTypes;
  ezReflectionUtils::CreateDependencySortedTypeArray(types, sortedTypes);

  for (auto type : sortedTypes)
  {
    ezUpdateReflectionTypeMsgToEditor TypeMsg;
    ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(type, TypeMsg.m_desc);
    m_IPC.SendMessage(&TypeMsg);
  }
}

void ezEngineProcessGameApplication::EventHandlerIPC(const ezProcessCommunication::Event& e)
{
  // Sync 
  if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<ezSyncWithProcessMsgToEngine>())
  {
    ezSyncWithProcessMsgToEditor msg;
    m_IPC.SendMessage(&msg);
    return;
  }

  // Project Messages:
  if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<ezSetupProjectMsgToEngine>())
  {
    const ezSetupProjectMsgToEngine* pSetupMsg = static_cast<const ezSetupProjectMsgToEngine*>(e.m_pMessage);
    ezApplicationConfig::SetProjectDirectory(pSetupMsg->m_sProjectDir);

    const_cast<ezSetupProjectMsgToEngine*>(pSetupMsg)->m_Config.Apply();
    // Project setup, we are now ready to accept document messages.
    SendProjectReadyMessage();
    return;
  }
  else if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<ezSimpleConfigMsgToEngine>())
  {
    const ezSimpleConfigMsgToEngine* pMsg = static_cast<const ezSimpleConfigMsgToEngine*>(e.m_pMessage);

    if (pMsg->m_sWhatToDo == "ReloadAssetLUT")
    {
      ezFileSystem::ReloadAllExternalDataDirectoryConfigs();
    }
    else if (pMsg->m_sWhatToDo == "ReloadResources")
    {
      ezResourceManager::ReloadAllResources();
    }
    else
      ezLog::Warning("Unknown ezSimpleConfigMsgToEngine '%s'", pMsg->m_sWhatToDo.GetData());
  }

  // Document Messages:
  if (!e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<ezEditorEngineDocumentMsg>())
    return;

  const ezEditorEngineDocumentMsg* pDocMsg = (const ezEditorEngineDocumentMsg*)e.m_pMessage;

  ezEngineProcessDocumentContext* pDocumentContext = ezEngineProcessDocumentContext::GetDocumentContext(pDocMsg->m_DocumentGuid);

  if (pDocMsg->GetDynamicRTTI()->IsDerivedFrom<ezDocumentOpenMsgToEngine>()) // Document was opened or closed
  {
    const ezDocumentOpenMsgToEngine* pMsg = static_cast<const ezDocumentOpenMsgToEngine*>(pDocMsg);

    if (pMsg->m_bDocumentOpen)
    {
      pDocumentContext = CreateDocumentContext(pMsg);
    }
    else
    {
      ezEngineProcessDocumentContext::DestroyDocumentContext(pDocMsg->m_DocumentGuid);
    }

    return;
  }

  EZ_ASSERT_DEV(pDocumentContext != nullptr, "Document Context is invalid!");

  pDocumentContext->HandleMessage(pDocMsg);


}

ezEngineProcessDocumentContext* ezEngineProcessGameApplication::CreateDocumentContext(const ezDocumentOpenMsgToEngine* pMsg)
{
  ezDocumentOpenResponseMsgToEditor m;
  m.m_DocumentGuid = pMsg->m_DocumentGuid;
  ezEngineProcessDocumentContext* pDocumentContext = nullptr;

  ezRTTI* pRtti = ezRTTI::GetFirstInstance();
  while (pRtti)
  {
    if (pRtti->IsDerivedFrom<ezEngineProcessDocumentContext>())
    {
      auto* pProp = pRtti->FindPropertyByName("DocumentType");
      if (pProp && pProp->GetCategory() == ezPropertyCategory::Constant)
      {
        if (static_cast<ezAbstractConstantProperty*>(pProp)->GetConstant().ConvertTo<ezString>() == pMsg->m_sDocumentType)
        {
          ezLog::Info("Created Context of type '%s' for '%s'", pRtti->GetTypeName(), pMsg->m_sDocumentType.GetData());

          pDocumentContext = static_cast<ezEngineProcessDocumentContext*>(pRtti->GetAllocator()->Allocate());

          ezEngineProcessDocumentContext::AddDocumentContext(pMsg->m_DocumentGuid, pDocumentContext, &m_IPC);
          break;
        }
      }
    }

    pRtti = pRtti->GetNextInstance();
  }

  m_IPC.SendMessage(&m);
  return pDocumentContext;
}


