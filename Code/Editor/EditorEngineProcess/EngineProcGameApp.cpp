#include <EditorEngineProcessPCH.h>

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <EditorEngineProcess/EngineProcGameApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <GameEngine/Prefabs/PrefabReferenceComponent.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

ezEngineProcessGameApplication::ezEngineProcessGameApplication()
  : ezGameApplication("ezEditorEngineProcess", nullptr)
{
  m_LongOpWorkerManager.Startup(&m_IPC);
}

ezResult ezEngineProcessGameApplication::BeforeCoreSystemsStartup()
{
  m_pApp = CreateEngineProcessApp();
  ezStartup::AddApplicationTag("editorengineprocess");

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  // Make sure to disable the fileserve plugin
  ezCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-fs_off");
#endif

  return SUPER::BeforeCoreSystemsStartup();
}

void ezEngineProcessGameApplication::AfterCoreSystemsStartup()
{
  // skip project creation at this point
  // SUPER::AfterCoreSystemsStartup();

#if EZ_DISABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  {
    // on all 'mobile' platforms, we assume we are in remote mode
    ezEditorEngineProcessApp::GetSingleton()->SetRemoteMode();
  }
#else
  if (ezCommandLineUtils::GetGlobalInstance()->GetBoolOption("-remote", false))
  {
    ezEditorEngineProcessApp::GetSingleton()->SetRemoteMode();
  }
#endif

  WaitForDebugger();

  DisableErrorReport();

  ezTaskSystem::SetTargetFrameTime(ezTime::Seconds(1.0 / 20.0));

  ConnectToHost();
}


void ezEngineProcessGameApplication::ConnectToHost()
{
  EZ_VERIFY(m_IPC.ConnectToHostProcess().Succeeded(), "Could not connect to host");

  m_IPC.m_Events.AddEventHandler(ezMakeDelegate(&ezEngineProcessGameApplication::EventHandlerIPC, this));

  // wait indefinitely (not necessary anymore, should work regardless)
  // m_IPC.WaitForMessage(ezGetStaticRTTI<ezSetupProjectMsgToEngine>(), ezTime());
}

void ezEngineProcessGameApplication::DisableErrorReport()
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  // Setting this flags prevents Windows from showing a dialog when the Engine process crashes
  // this also speeds up process termination significantly (down to less than a second)
  DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
  SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
#endif
}

void ezEngineProcessGameApplication::WaitForDebugger()
{
  if (ezCommandLineUtils::GetGlobalInstance()->GetBoolOption("-debug"))
  {
    while (!IsDebuggerPresent())
    {
      ezThreadUtils::Sleep(ezTime::Milliseconds(10));
    }
  }
}

void ezEngineProcessGameApplication::BeforeCoreSystemsShutdown()
{
  m_pApp = nullptr;

  m_LongOpWorkerManager.Shutdown();

  m_IPC.m_Events.RemoveEventHandler(ezMakeDelegate(&ezEngineProcessGameApplication::EventHandlerIPC, this));

  SUPER::BeforeCoreSystemsShutdown();
}

ezApplication::ApplicationExecution ezEngineProcessGameApplication::Run()
{
  ezRenderWorld::ClearMainViews();

  bool bPendingOpInProgress = ezEngineProcessDocumentContext::PendingOperationsInProgress();
  if (ProcessIPCMessages(bPendingOpInProgress))
  {
    ezEngineProcessDocumentContext::UpdateDocumentContexts();
  }

  return SUPER::Run();
}

void ezEngineProcessGameApplication::LogWriter(const ezLoggingEventData& e)
{
  ezLogMsgToEditor msg;
  msg.m_Entry = ezLogEntry(e);
  m_IPC.SendMessage(&msg);
}

static bool EmptyAssertHandler(
  const char* szSourceFile, ezUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  return false;
}

bool ezEngineProcessGameApplication::ProcessIPCMessages(bool bPendingOpInProgress)
{
  if (!m_IPC.IsHostAlive()) // check whether the host crashed
  {
    // The problem here is, that the editor process crashed (or was terminated through Visual Studio),
    // but our process depends on it for cleanup!
    // That means, this process created rendering resources through a device that is bound to a window handle, which belonged to the editor
    // process. So now we can't clean up, and therefore we can only crash. Therefore we try to crash as silently as possible.

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    // Make sure that Windows doesn't show a default message box when we call abort
    _set_abort_behavior(0, _WRITE_ABORT_MSG);
#endif

    // The OS will still call destructors for our objects (even though we called abort ... what a pointless design).
    // Our code might assert on destruction, so make sure our assert handler doesn't show anything.
    ezSetAssertHandler(EmptyAssertHandler);
    std::abort();

    RequestQuit();
    return false;
  }
  else
  {
    // if an operation is still pending or this process is a remote process, we do NOT want to block
    // remote processes shall run as fast as they can
    if (bPendingOpInProgress || ezEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    {
      m_IPC.ProcessMessages();
    }
    else
    {
      // Only suspend and wait if no more pending ops need to be done.
      m_IPC.WaitForMessages();
    }
    return true;
  }
}

void ezEngineProcessGameApplication::SendProjectReadyMessage()
{
  ezProjectReadyMsgToEditor msg;
  m_IPC.SendMessage(&msg);
}

void ezEngineProcessGameApplication::SendReflectionInformation()
{
  if (ezEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    return;

  ezSet<const ezRTTI*> types;

  ezReflectionUtils::GatherTypesDerivedFromClass(ezGetStaticRTTI<ezReflectedClass>(), types, true);

  ezDynamicArray<const ezRTTI*> sortedTypes;
  ezReflectionUtils::CreateDependencySortedTypeArray(types, sortedTypes);

  for (auto type : sortedTypes)
  {
    ezUpdateReflectionTypeMsgToEditor TypeMsg;
    ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(type, TypeMsg.m_desc);
    m_IPC.SendMessage(&TypeMsg);
  }
}

void ezEngineProcessGameApplication::EventHandlerIPC(const ezEngineProcessCommunicationChannel::Event& e)
{
  if (const auto* pMsg = ezDynamicCast<const ezSyncWithProcessMsgToEngine*>(e.m_pMessage))
  {
    ezSyncWithProcessMsgToEditor msg;
    msg.m_DocumentGuid = pMsg->m_DocumentGuid;
    msg.m_uiRedrawCount = pMsg->m_uiRedrawCount;
    m_IPC.SendMessage(&msg);
    return;
  }

  if (const auto* pMsg = ezDynamicCast<const ezShutdownProcessMsgToEngine*>(e.m_pMessage))
  {
    // in non-remote mode, the process needs to be properly killed, to prevent error messages
    // this is taken care of by the editor process
    if (ezEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
      RequestQuit();

    return;
  }

  // Project Messages:
  if (const auto* pMsg = ezDynamicCast<const ezSetupProjectMsgToEngine*>(e.m_pMessage))
  {
    if (!m_sProjectDirectory.IsEmpty())
    {
      // ignore this message, if it is for the same project
      if (m_sProjectDirectory == pMsg->m_sProjectDir)
        return;

      ezLog::Error("Engine Process must restart to switch to another project ('{0}' -> '{1}').", m_sProjectDirectory, pMsg->m_sProjectDir);
      return;
    }

    m_sProjectDirectory = pMsg->m_sProjectDir;
    m_CustomFileSystemConfig = pMsg->m_FileSystemConfig;
    m_CustomPluginConfig = pMsg->m_PluginConfig;

    if (!pMsg->m_sAssetProfile.IsEmpty())
    {
      ezCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-profile");
      ezCommandLineUtils::GetGlobalInstance()->InjectCustomArgument(pMsg->m_sAssetProfile);
    }

    if (!pMsg->m_sFileserveAddress.IsEmpty())
    {
      // we have no link dependency on the fileserve plugin here, it might not be loaded (yet / at all)
      // but we can pass the address to the command line, then it will pick it up, if necessary
      ezCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-fs_server");
      ezCommandLineUtils::GetGlobalInstance()->InjectCustomArgument(pMsg->m_sFileserveAddress);
    }

    // now that we know which project to initialize, do the delayed project setup
    {
      ExecuteInitFunctions();

      ezStartup::StartupHighLevelSystems();

      ezRenderContext::GetDefaultInstance()->SetAllowAsyncShaderLoading(true);
    }

    // after the ezSetupProjectMsgToEngine was processed, all dynamic plugins should be loaded and we can finally send the reflection
    // information over
    SendReflectionInformation();

    // Project setup, we are now ready to accept document messages.
    SendProjectReadyMessage();
    return;
  }
  else if (const auto* pMsg = ezDynamicCast<const ezSimpleConfigMsgToEngine*>(e.m_pMessage))
  {
    if (pMsg->m_sWhatToDo == "ChangeActivePlatform")
    {
      ezStringBuilder sRedirFile("AssetCache/", pMsg->m_sPayload, ".ezAidlt");

      ezDataDirectory::FolderType::s_sRedirectionFile = sRedirFile;

      ezFileSystem::ReloadAllExternalDataDirectoryConfigs();

      m_PlatformProfile.m_sName = pMsg->m_sPayload;
      Init_PlatformProfile_LoadForRuntime();

      ezResourceManager::ReloadAllResources(false);
      ezRenderWorld::DeleteAllCachedRenderData();
    }
    else if (pMsg->m_sWhatToDo == "ReloadAssetLUT")
    {
      ezFileSystem::ReloadAllExternalDataDirectoryConfigs();
    }
    else if (pMsg->m_sWhatToDo == "ReloadResources")
    {
      ezResourceManager::ReloadAllResources(false);
      ezRenderWorld::DeleteAllCachedRenderData();
    }
    else
      ezLog::Warning("Unknown ezSimpleConfigMsgToEngine '{0}'", pMsg->m_sWhatToDo);
  }
  else if (const auto* pMsg = ezDynamicCast<const ezResourceUpdateMsgToEngine*>(e.m_pMessage))
  {
    HandleResourceUpdateMsg(*pMsg);
  }
  else if (const auto* pMsg = ezDynamicCast<const ezRestoreResourceMsgToEngine*>(e.m_pMessage))
  {
    HandleResourceRestoreMsg(*pMsg);
  }
  else if (const auto* pMsg = ezDynamicCast<const ezChangeCVarMsgToEngine*>(e.m_pMessage))
  {
    if (ezCVar* pCVar = ezCVar::FindCVarByName(pMsg->m_sCVarName))
    {
      if (pCVar->GetType() == ezCVarType::Int && pMsg->m_NewValue.CanConvertTo<ezInt32>())
      {
        *static_cast<ezCVarInt*>(pCVar) = pMsg->m_NewValue.ConvertTo<ezInt32>();
      }
      else if (pCVar->GetType() == ezCVarType::Float && pMsg->m_NewValue.CanConvertTo<float>())
      {
        *static_cast<ezCVarFloat*>(pCVar) = pMsg->m_NewValue.ConvertTo<float>();
      }
      else if (pCVar->GetType() == ezCVarType::Bool && pMsg->m_NewValue.CanConvertTo<bool>())
      {
        *static_cast<ezCVarBool*>(pCVar) = pMsg->m_NewValue.ConvertTo<bool>();
      }
      else if (pCVar->GetType() == ezCVarType::String && pMsg->m_NewValue.CanConvertTo<ezString>())
      {
        *static_cast<ezCVarString*>(pCVar) = pMsg->m_NewValue.ConvertTo<ezString>();
      }
      else
      {
        ezLog::Warning("ezChangeCVarMsgToEngine: New value for CVar '{0}' is incompatible with CVar type", pMsg->m_sCVarName);
      }
    }
    else
      ezLog::Warning("ezChangeCVarMsgToEngine: Unknown CVar '{0}'", pMsg->m_sCVarName);
  }

  // Document Messages:
  if (!e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<ezEditorEngineDocumentMsg>())
    return;

  const ezEditorEngineDocumentMsg* pDocMsg = (const ezEditorEngineDocumentMsg*)e.m_pMessage;

  ezEngineProcessDocumentContext* pDocumentContext = ezEngineProcessDocumentContext::GetDocumentContext(pDocMsg->m_DocumentGuid);

  if (const auto* pMsg = ezDynamicCast<const ezDocumentOpenMsgToEngine*>(e.m_pMessage)) // Document was opened or closed
  {
    if (pMsg->m_bDocumentOpen)
    {
      pDocumentContext = CreateDocumentContext(pMsg);
      // EZ_ASSERT_DEV(pDocumentContext != nullptr, "Could not create a document context for document type '{0}'", pMsg->m_sDocumentType);
    }
    else
    {
      ezEngineProcessDocumentContext::DestroyDocumentContext(pDocMsg->m_DocumentGuid);
    }

    return;
  }

  if (const auto* pMsg = ezDynamicCast<const ezDocumentClearMsgToEngine*>(e.m_pMessage))
  {
    ezEngineProcessDocumentContext* pDocumentContext = ezEngineProcessDocumentContext::GetDocumentContext(pMsg->m_DocumentGuid);
    pDocumentContext->ClearExistingObjects();
    return;
  }

  // can be null if the asset was deleted on disk manually
  if (pDocumentContext)
  {
    pDocumentContext->HandleMessage(pDocMsg);
  }
}

ezEngineProcessDocumentContext* ezEngineProcessGameApplication::CreateDocumentContext(const ezDocumentOpenMsgToEngine* pMsg)
{
  ezDocumentOpenResponseMsgToEditor m;
  m.m_DocumentGuid = pMsg->m_DocumentGuid;
  ezEngineProcessDocumentContext* pDocumentContext = ezEngineProcessDocumentContext::GetDocumentContext(pMsg->m_DocumentGuid);

  if (pDocumentContext == nullptr)
  {
    ezRTTI* pRtti = ezRTTI::GetFirstInstance();
    while (pRtti)
    {
      if (pRtti->IsDerivedFrom<ezEngineProcessDocumentContext>())
      {
        auto* pProp = pRtti->FindPropertyByName("DocumentType");
        if (pProp && pProp->GetCategory() == ezPropertyCategory::Constant)
        {
          const ezStringBuilder sDocTypes(";", static_cast<ezAbstractConstantProperty*>(pProp)->GetConstant().ConvertTo<ezString>(), ";");
          const ezStringBuilder sRequestedType(";", pMsg->m_sDocumentType, ";");

          if (sDocTypes.FindSubString(sRequestedType) != nullptr)
          {
            ezLog::Dev("Created Context of type '{0}' for '{1}'", pRtti->GetTypeName(), pMsg->m_sDocumentType);

            pDocumentContext = pRtti->GetAllocator()->Allocate<ezEngineProcessDocumentContext>();

            ezEngineProcessDocumentContext::AddDocumentContext(pMsg->m_DocumentGuid, pDocumentContext, &m_IPC);
            break;
          }
        }
      }

      pRtti = pRtti->GetNextInstance();
    }
  }
  else
  {
    pDocumentContext->Reset();
  }

  m_IPC.SendMessage(&m);
  return pDocumentContext;
}

void ezEngineProcessGameApplication::Init_LoadProjectPlugins()
{
  m_CustomPluginConfig.SetOnlyLoadManualPlugins(false); // we also want to load editor plugin dependencies
  m_CustomPluginConfig.Apply();
}

ezString ezEngineProcessGameApplication::FindProjectDirectory() const
{
  return m_sProjectDirectory;
}

void ezEngineProcessGameApplication::Init_FileSystem_ConfigureDataDirs()
{
  ezStringBuilder sAppDir = ">sdk/Data/Tools/EditorEngineProcess";
  ezStringBuilder sUserData = ">user/ezEngine Project/EditorEngineProcess";

  // make sure these directories exist
  ezFileSystem::CreateDirectoryStructure(sAppDir);
  ezFileSystem::CreateDirectoryStructure(sUserData);

  ezFileSystem::AddDataDirectory("", "EngineProcess", ":", ezFileSystem::AllowWrites);                   // for absolute paths
  ezFileSystem::AddDataDirectory(">appdir/", "EngineProcess", "bin", ezFileSystem::ReadOnly);            // writing to the binary directory
  ezFileSystem::AddDataDirectory(">appdir/", "EngineProcess", "shadercache", ezFileSystem::AllowWrites); // for shader files
  ezFileSystem::AddDataDirectory(sAppDir.GetData(), "EngineProcess", "app");                             // app specific data
  ezFileSystem::AddDataDirectory(sUserData, "EngineProcess", "appdata", ezFileSystem::AllowWrites);      // for writing app user data

  m_CustomFileSystemConfig.Apply();
}

bool ezEngineProcessGameApplication::Run_ProcessApplicationInput()
{
  // override the escape action to not shut down the app, but instead close the play-the-game window
  if (ezInputManager::GetInputActionState("GameApp", "CloseApp") != ezKeyState::Up)
  {
    if (m_pGameState)
    {
      m_pGameState->RequestQuit();
    }
  }
  else
  {
    return SUPER::Run_ProcessApplicationInput();
  }

  return true;
}

ezUniquePtr<ezEditorEngineProcessApp> ezEngineProcessGameApplication::CreateEngineProcessApp()
{
  return EZ_DEFAULT_NEW(ezEditorEngineProcessApp);
}

void ezEngineProcessGameApplication::BaseInit_ConfigureLogging()
{
  SUPER::BaseInit_ConfigureLogging();

  ezGlobalLog::AddLogWriter(ezMakeDelegate(&ezEngineProcessGameApplication::LogWriter, this));

  // used for sending CVar changes over to the editor
  ezCVar::s_AllCVarEvents.AddEventHandler(ezMakeDelegate(&ezEngineProcessGameApplication::EventHandlerCVar, this));
  ezPlugin::s_PluginEvents.AddEventHandler(ezMakeDelegate(&ezEngineProcessGameApplication::EventHandlerCVarPlugin, this));
}

void ezEngineProcessGameApplication::Deinit_ShutdownLogging()
{
  ezGlobalLog::RemoveLogWriter(ezMakeDelegate(&ezEngineProcessGameApplication::LogWriter, this));

  // used for sending CVar changes over to the editor
  ezCVar::s_AllCVarEvents.RemoveEventHandler(ezMakeDelegate(&ezEngineProcessGameApplication::EventHandlerCVar, this));
  ezPlugin::s_PluginEvents.RemoveEventHandler(ezMakeDelegate(&ezEngineProcessGameApplication::EventHandlerCVarPlugin, this));

  SUPER::Deinit_ShutdownLogging();
}

void ezEngineProcessGameApplication::EventHandlerCVar(const ezCVarEvent& e)
{
  if (e.m_EventType == ezCVarEvent::ValueChanged)
  {
    TransmitCVar(e.m_pCVar);
  }

  if (e.m_EventType == ezCVarEvent::ListOfVarsChanged)
  {
    // currently no way to remove CVars from the editor UI

    for (ezCVar* pCVar = ezCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
    {
      TransmitCVar(pCVar);
    }
  }
}

void ezEngineProcessGameApplication::EventHandlerCVarPlugin(const ezPluginEvent& e)
{
  if (e.m_EventType == ezPluginEvent::Type::AfterLoadingBeforeInit)
  {
    for (ezCVar* pCVar = ezCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
    {
      TransmitCVar(pCVar);
    }
  }
}

void ezEngineProcessGameApplication::TransmitCVar(const ezCVar* pCVar)
{
  ezCVarMsgToEditor msg;
  msg.m_sName = pCVar->GetName();
  msg.m_sPlugin = pCVar->GetPluginName();
  msg.m_sDescription = pCVar->GetDescription();

  switch (pCVar->GetType())
  {
    case ezCVarType::Int:
      msg.m_Value = ((ezCVarInt*)pCVar)->GetValue();
      break;
    case ezCVarType::Float:
      msg.m_Value = ((ezCVarFloat*)pCVar)->GetValue();
      break;
    case ezCVarType::Bool:
      msg.m_Value = ((ezCVarBool*)pCVar)->GetValue();
      break;
    case ezCVarType::String:
      msg.m_Value = ((ezCVarString*)pCVar)->GetValue();
      break;
  }

  m_IPC.SendMessage(&msg);
}

void ezEngineProcessGameApplication::HandleResourceUpdateMsg(const ezResourceUpdateMsgToEngine& msg)
{
  const ezRTTI* pRtti = ezResourceManager::FindResourceForAssetType(msg.m_sResourceType);

  if (pRtti == nullptr)
  {
    ezLog::Error("Resource Type '{}' is unknown.", msg.m_sResourceType);
    return;
  }

  ezTypelessResourceHandle hResource = ezResourceManager::GetExistingResourceByType(pRtti, msg.m_sResourceID);

  if (hResource.IsValid())
  {
    ezStringBuilder sResourceDesc;
    sResourceDesc.Set(msg.m_sResourceType, "LiveUpdate");

    ezUniquePtr<ezResourceLoaderFromMemory> loader(EZ_DEFAULT_NEW(ezResourceLoaderFromMemory));
    loader->m_ModificationTimestamp = ezTimestamp::CurrentTimestamp();
    loader->m_sResourceDescription = sResourceDesc;

    ezMemoryStreamWriter memoryWriter(&loader->m_CustomData);
    memoryWriter.WriteBytes(msg.m_Data.GetData(), msg.m_Data.GetCount());

    ezResourceManager::UpdateResourceWithCustomLoader(hResource, std::move(loader));

    ezResourceManager::ForceLoadResourceNow(hResource);
  }
}

void ezEngineProcessGameApplication::HandleResourceRestoreMsg(const ezRestoreResourceMsgToEngine& msg)
{
  const ezRTTI* pRtti = ezResourceManager::FindResourceForAssetType(msg.m_sResourceType);

  if (pRtti == nullptr)
  {
    ezLog::Error("Resource Type '{}' is unknown.", msg.m_sResourceType);
    return;
  }

  ezTypelessResourceHandle hResource = ezResourceManager::GetExistingResourceByType(pRtti, msg.m_sResourceID);

  if (hResource.IsValid())
  {
    ezResourceManager::RestoreResource(hResource);
  }
}
