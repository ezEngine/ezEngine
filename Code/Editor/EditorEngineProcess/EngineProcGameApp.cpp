#include <EditorEngineProcess/EditorEngineProcessPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Logging/ETW.h>
#include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#include <Foundation/Profiling/ProfilingUtils.h>
#include <Foundation/System/CrashHandler.h>
#include <Foundation/System/SystemInformation.h>

#include <Core/Console/QuakeConsole.h>
#include <EditorEngineProcess/EngineProcGameApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoRenderer.h>
#include <Foundation/System/StackTracer.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
#  include <shellscalingapi.h>
#endif

ezCommandLineOptionPath opt_OutputDir("_EditorEngineProcess", "-outputDir", "Output directory", "");
ezCommandLineOptionString opt_LogName("_EditorEngineProcess", "-logName", "Log File Prefix", "LogEngine");

// Will forward assert messages and crash handler messages to the log system and then to the editor.
// Note that this is unsafe as in some crash situation allocating memory will not be possible but it's better to have some logs compared to none.
void EditorPrintFunction(const char* szText)
{
  ezStringBuilder sError = szText;
  sError.Trim();
  ezLog::Error("{}", sError.GetData());
}

static ezAssertHandler g_PreviousAssertHandler = nullptr;

ezEngineProcessGameApplication::ezEngineProcessGameApplication()
  : ezGameApplication("ezEditorEngineProcess", nullptr)
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
#endif

  m_LongOpWorkerManager.Startup(&m_IPC);
}

ezEngineProcessGameApplication::~ezEngineProcessGameApplication() = default;

ezResult ezEngineProcessGameApplication::BeforeCoreSystemsStartup()
{
  m_pApp = CreateEngineProcessApp();
  ezStartup::AddApplicationTag("editorengineprocess");

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP) || EZ_ENABLED(EZ_PLATFORM_LINUX)
  // Make sure to disable the fileserve plugin
  ezCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-fs_off");
#endif

  AddEditorAssertHandler();
  return SUPER::BeforeCoreSystemsStartup();
}

void ezEngineProcessGameApplication::AfterCoreSystemsStartup()
{
  // skip project creation at this point
  // SUPER::AfterCoreSystemsStartup();

#if EZ_DISABLED(EZ_PLATFORM_WINDOWS_DESKTOP) && EZ_DISABLED(EZ_PLATFORM_LINUX)
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

  ezCrashHandler::SetCrashHandler(&ezCrashHandler_WriteMiniDump::g_Instance);

  DisableErrorReport();

  ezTaskSystem::SetTargetFrameTime(ezTime::MakeFromSeconds(1.0 / 20.0));

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
  if (ezCommandLineUtils::GetGlobalInstance()->GetBoolOption("-WaitForDebugger"))
  {
    while (!ezSystemInformation::IsDebuggerAttached())
    {
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(10));
    }
  }
}

bool ezEngineProcessGameApplication::EditorAssertHandler(const char* szSourceFile, ezUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  ezLog::Error("*** Assertion ***:\nFile: \"{}\",\nLine: \"{}\",\nFunction: \"{}\",\nExpression: \"{}\",\nMessage: \"{}\"", szSourceFile, uiLine, szFunction, szExpression, szAssertMsg);

  void* pBuffer[64];
  ezArrayPtr<void*> tempTrace(pBuffer);
  const ezUInt32 uiNumTraces = ezStackTracer::GetStackTrace(tempTrace, nullptr);
  ezStackTracer::ResolveStackTrace(tempTrace.GetSubArray(0, uiNumTraces), &ezLog::Print);
  ezLog::Flush();

  // Wait for flush of IPC messages
  ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(500));

  if (g_PreviousAssertHandler)
    return g_PreviousAssertHandler(szSourceFile, uiLine, szFunction, szExpression, szAssertMsg);

  return true;
}

void ezEngineProcessGameApplication::AddEditorAssertHandler()
{
  g_PreviousAssertHandler = ezGetAssertHandler();
  ezSetAssertHandler(EditorAssertHandler);
}

void ezEngineProcessGameApplication::RemoveEditorAssertHandler()
{
  ezSetAssertHandler(g_PreviousAssertHandler);
  g_PreviousAssertHandler = nullptr;
}

void ezEngineProcessGameApplication::BeforeCoreSystemsShutdown()
{
  RemoveEditorAssertHandler();

  m_pApp = nullptr;

  m_LongOpWorkerManager.Shutdown();

  m_IPC.m_Events.RemoveEventHandler(ezMakeDelegate(&ezEngineProcessGameApplication::EventHandlerIPC, this));

  SUPER::BeforeCoreSystemsShutdown();
}

void ezEngineProcessGameApplication::Run()
{
  bool bPendingOpInProgress = false;
  do
  {
    bPendingOpInProgress = ezEngineProcessDocumentContext::PendingOperationsInProgress();
    if (ProcessIPCMessages(bPendingOpInProgress))
    {
      ezEngineProcessDocumentContext::UpdateDocumentContexts();
    }
  } while (!bPendingOpInProgress && m_uiRedrawCountExecuted == m_uiRedrawCountReceived);

  m_uiRedrawCountExecuted = m_uiRedrawCountReceived;

  // Normally rendering is done in EventHandlerIPC as a response to ezSyncWithProcessMsgToEngine. However, when playing or when pending operations are in progress we need to render even if we didn't receive a draw request.
  SUPER::Run();
  ezRenderWorld::ClearMainViews();
}

void ezEngineProcessGameApplication::LogWriter(const ezLoggingEventData& e)
{
  ezLogMsgToEditor msg;
  msg.m_Entry = ezLogEntry(e);

  // the editor does not care about flushing caches, so no need to send this over
  if (msg.m_Entry.m_Type == ezLogMsgType::Flush)
    return;

  if (msg.m_Entry.m_sTag == "IPC")
    return;

  // Prevent infinite recursion by disabeling logging until we are done sending the message
  EZ_LOG_BLOCK_MUTE();

  m_IPC.SendMessage(&msg);
}

static bool EmptyAssertHandler(const char* szSourceFile, ezUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  return false;
}

bool ezEngineProcessGameApplication::ProcessIPCMessages(bool bPendingOpInProgress)
{
  EZ_PROFILE_SCOPE("ProcessIPCMessages");

  if (!m_IPC.IsHostAlive()) // check whether the host crashed
  {
    // The problem here is, that the editor process crashed (or was terminated through Visual Studio),
    // but our process depends on it for cleanup!
    // That means, this process created rendering resources through a device that is bound to a window handle, which belonged to the editor
    // process. So now we can't clean up, and therefore we can only crash. Therefore we try to crash as silently as possible.

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    // Make sure that Windows doesn't show a default message box when we call abort
    _set_abort_behavior(0, _WRITE_ABORT_MSG);
    TerminateProcess(GetCurrentProcess(), 0);
#endif

    ezLog::SeriousWarning("Host process no longer alive, exiting engine process.");

    // The OS will still call destructors for our objects (even though we called abort ... what a pointless design).
    // Our code might assert on destruction, so make sure our assert handler doesn't show anything.
    ezSetAssertHandler(EmptyAssertHandler);
    std::abort();
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
      EZ_PROFILE_SCOPE("WaitForMessages");
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
  ezRTTI::ForEachType(
    [&](const ezRTTI* pRtti)
    {
      if (pRtti->GetTypeFlags().IsSet(ezTypeFlags::StandardType) == false)
      {
        types.Insert(pRtti);
      }
    });

  ezDynamicArray<const ezRTTI*> sortedTypes;
  ezReflectionUtils::CreateDependencySortedTypeArray(types, sortedTypes).AssertSuccess("Sorting failed");

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
    ezStringBuilder sRedrawScope;
    sRedrawScope.SetFormat("Redraw {}", pMsg->m_uiRedrawCount);
    EZ_PROFILE_SCOPE(sRedrawScope.GetData());

    ezSyncWithProcessMsgToEditor msg;
    msg.m_uiRedrawCount = pMsg->m_uiRedrawCount;
    m_uiRedrawCountReceived = msg.m_uiRedrawCount;

    // We must clear the main views after rendering so that if the editor runs in lock step with the engine we don't render a view twice or request update again without rendering being done.
    RunOneFrame();
    ezRenderWorld::ClearMainViews();

    m_IPC.SendMessage(&msg);
    return;
  }

  if (const auto* pMsg = ezDynamicCast<const ezShutdownProcessMsgToEngine*>(e.m_pMessage))
  {
    // in non-remote mode, the process needs to be properly killed, to prevent error messages
    // this is taken care of by the editor process
    if (ezEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
      RequestApplicationQuit();

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
      ezDebugRenderer::SetTextScale(pMsg->m_fDevicePixelRatio);
    }

    // after the ezSetupProjectMsgToEngine was processed, all dynamic plugins should be loaded and we can finally send the reflection
    // information over
    SendReflectionInformation();

    // Project setup, we are now ready to accept document messages.
    SendProjectReadyMessage();
    return;
  }
  else if (const auto* pMsg1 = ezDynamicCast<const ezReloadResourceMsgToEngine*>(e.m_pMessage))
  {
    EZ_PROFILE_SCOPE("ReloadResource");

    const ezRTTI* pType = ezResourceManager::FindResourceForAssetType(pMsg1->m_sResourceType);
    if (auto hResource = ezResourceManager::GetExistingResourceByType(pType, pMsg1->m_sResourceID); hResource.IsValid())
    {
      ezResourceManager::ReloadResource(pType, hResource, false);
    }
  }
  else if (const auto* pMsg1 = ezDynamicCast<const ezSimpleConfigMsgToEngine*>(e.m_pMessage))
  {
    if (pMsg1->m_sWhatToDo == "ChangeActivePlatform")
    {
      ezStringBuilder sRedirFile("AssetCache/", pMsg1->m_sPayload, ".ezAidlt");

      ezDataDirectory::FolderType::s_sRedirectionFile = sRedirFile;

      ezFileSystem::ReloadAllExternalDataDirectoryConfigs();

      m_PlatformProfile.SetConfigName(pMsg1->m_sPayload);
      Init_PlatformProfile_LoadForRuntime();

      ezResourceManager::ReloadAllResources(false);
      ezRenderWorld::DeleteAllCachedRenderData();
    }
    else if (pMsg1->m_sWhatToDo == "ReloadAssetLUT")
    {
      ezFileSystem::ReloadAllExternalDataDirectoryConfigs();
    }
    else if (pMsg1->m_sWhatToDo == "FreeGalResources")
    {
      ezRenderWorld::ClearMainViews();
      RunOneFrame();
      ezGALDevice::GetDefaultDevice()->WaitIdle();
    }
    else if (pMsg1->m_sWhatToDo == "FreeAllResources")
    {
      ezResourceManager::FreeAllUnusedResources();
      ezRenderWorld::ClearMainViews();
      RunOneFrame();
      ezGALDevice::GetDefaultDevice()->WaitIdle();
    }
    else if (pMsg1->m_sWhatToDo == "ReloadResources")
    {
      if (pMsg1->m_sPayload == "ReloadAllResources")
      {
        ezResourceManager::ReloadAllResources(false);
      }
      ezRenderWorld::DeleteAllCachedRenderData();
    }
    else if (pMsg1->m_sWhatToDo == "SaveProfiling")
    {
      ezProfilingUtils::SaveProfilingCapture(pMsg1->m_sPayload).IgnoreResult();

      ezSaveProfilingResponseToEditor response;
      ezStringBuilder sAbsPath;
      if (ezFileSystem::ResolvePath(pMsg1->m_sPayload, &sAbsPath, nullptr).Succeeded())
      {
        response.m_sProfilingFile = sAbsPath;
      }
      m_IPC.SendMessage(&response);
    }
    else
      ezLog::Warning("Unknown ezSimpleConfigMsgToEngine '{0}'", pMsg1->m_sWhatToDo);
  }
  else if (const auto* pMsg2 = ezDynamicCast<const ezResourceUpdateMsgToEngine*>(e.m_pMessage))
  {
    HandleResourceUpdateMsg(*pMsg2);
  }
  else if (const auto* pMsg2a = ezDynamicCast<const ezRestoreResourceMsgToEngine*>(e.m_pMessage))
  {
    HandleResourceRestoreMsg(*pMsg2a);
  }
  else if (const auto* pMsg2b = ezDynamicCast<const ezGlobalSettingsMsgToEngine*>(e.m_pMessage))
  {
    ezGizmoRenderer::s_fGizmoScale = pMsg2b->m_fGizmoScale;
  }
  else if (const auto* pMsg3 = ezDynamicCast<const ezChangeCVarMsgToEngine*>(e.m_pMessage))
  {
    if (ezCVar* pCVar = ezCVar::FindCVarByName(pMsg3->m_sCVarName))
    {
      if (pCVar->GetType() == ezCVarType::Int && pMsg3->m_NewValue.CanConvertTo<ezInt32>())
      {
        *static_cast<ezCVarInt*>(pCVar) = pMsg3->m_NewValue.ConvertTo<ezInt32>();
      }
      else if (pCVar->GetType() == ezCVarType::Float && pMsg3->m_NewValue.CanConvertTo<float>())
      {
        *static_cast<ezCVarFloat*>(pCVar) = pMsg3->m_NewValue.ConvertTo<float>();
      }
      else if (pCVar->GetType() == ezCVarType::Bool && pMsg3->m_NewValue.CanConvertTo<bool>())
      {
        *static_cast<ezCVarBool*>(pCVar) = pMsg3->m_NewValue.ConvertTo<bool>();
      }
      else if (pCVar->GetType() == ezCVarType::String && pMsg3->m_NewValue.CanConvertTo<ezString>())
      {
        *static_cast<ezCVarString*>(pCVar) = pMsg3->m_NewValue.ConvertTo<ezString>();
      }
      else
      {
        ezLog::Warning("ezChangeCVarMsgToEngine: New value for CVar '{0}' is incompatible with CVar type", pMsg3->m_sCVarName);
      }
    }
    else
      ezLog::Warning("ezChangeCVarMsgToEngine: Unknown CVar '{0}'", pMsg3->m_sCVarName);
  }
  else if (const auto* pMsg4 = ezDynamicCast<const ezConsoleCmdMsgToEngine*>(e.m_pMessage))
  {
    if (m_pConsole->GetCommandInterpreter())
    {
      ezCommandInterpreterState s;
      s.m_sInput = pMsg4->m_sCommand;

      ezStringBuilder tmp;

      if (pMsg4->m_iType == 1)
      {
        m_pConsole->GetCommandInterpreter()->AutoComplete(s);
        tmp.AppendFormat(";;00||<{}", s.m_sInput);
      }
      else
        m_pConsole->GetCommandInterpreter()->Interpret(s);

      for (auto l : s.m_sOutput)
      {
        tmp.AppendFormat(";;{}||{}", ezArgI((int)l.m_Type, 2, true), l.m_sText);
      }

      ezConsoleCmdResultMsgToEditor r;
      r.m_sResult = tmp;

      m_IPC.SendMessage(&r);
    }
  }

  // Document Messages:
  if (!e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<ezEditorEngineDocumentMsg>())
    return;

  const ezEditorEngineDocumentMsg* pDocMsg = (const ezEditorEngineDocumentMsg*)e.m_pMessage;

  ezEngineProcessDocumentContext* pDocumentContext = ezEngineProcessDocumentContext::GetDocumentContext(pDocMsg->m_DocumentGuid);

  if (const auto* pMsg5 = ezDynamicCast<const ezDocumentOpenMsgToEngine*>(e.m_pMessage)) // Document was opened or closed
  {
    if (pMsg5->m_bDocumentOpen)
    {
      pDocumentContext = CreateDocumentContext(pMsg5);
      EZ_ASSERT_DEV(pDocumentContext != nullptr, "Could not create a document context for document type '{0}'", pMsg5->m_sDocumentType);
    }
    else
    {
      ezEngineProcessDocumentContext::DestroyDocumentContext(pDocMsg->m_DocumentGuid);
    }

    return;
  }

  if (const auto* pMsg6 = ezDynamicCast<const ezDocumentClearMsgToEngine*>(e.m_pMessage))
  {
    pDocumentContext = ezEngineProcessDocumentContext::GetDocumentContext(pMsg6->m_DocumentGuid);

    if (pDocumentContext)
    {
      pDocumentContext->ClearExistingObjects();
    }
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
    ezRTTI::ForEachDerivedType<ezEngineProcessDocumentContext>(
      [&](const ezRTTI* pRtti)
      {
        auto* pProp = pRtti->FindPropertyByName("DocumentType");
        if (pProp && pProp->GetCategory() == ezPropertyCategory::Constant)
        {
          const ezStringBuilder sDocTypes(";", static_cast<const ezAbstractConstantProperty*>(pProp)->GetConstant().ConvertTo<ezString>(), ";");
          const ezStringBuilder sRequestedType(";", pMsg->m_sDocumentType, ";");

          if (sDocTypes.FindSubString(sRequestedType) != nullptr)
          {
            ezLog::Dev("Created Context of type '{0}' for '{1}'", pRtti->GetTypeName(), pMsg->m_sDocumentType);
            for (auto pFunc : pRtti->GetFunctions())
            {
              if (ezStringUtils::IsEqual(pFunc->GetPropertyName(), "AllocateContext"))
              {
                ezVariant res;
                ezHybridArray<ezVariant, 1> params;
                params.PushBack(pMsg);
                pFunc->Execute(nullptr, params, res);
                if (res.IsA<ezEngineProcessDocumentContext*>())
                {
                  pDocumentContext = res.Get<ezEngineProcessDocumentContext*>();
                }
                else
                {
                  ezLog::Error("Failed to call custom allocator '{}::{}'.", pRtti->GetTypeName(), pFunc->GetPropertyName());
                }
              }
            }

            if (!pDocumentContext)
            {
              pDocumentContext = pRtti->GetAllocator()->Allocate<ezEngineProcessDocumentContext>();
            }

            ezEngineProcessDocumentContext::AddDocumentContext(pMsg->m_DocumentGuid, pMsg->m_DocumentMetaData, pDocumentContext, &m_IPC, pMsg->m_sDocumentType);
          }
        }
      });
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
  m_CustomPluginConfig.m_Plugins.Sort([](const ezApplicationPluginConfig::PluginConfig& lhs, const ezApplicationPluginConfig::PluginConfig& rhs) -> bool
    {
    const bool isEnginePluginLhs = lhs.m_sAppDirRelativePath.FindSubString_NoCase("EnginePlugin") != nullptr;
    const bool isEnginePluginRhs = rhs.m_sAppDirRelativePath.FindSubString_NoCase("EnginePlugin") != nullptr;

    if (isEnginePluginLhs != isEnginePluginRhs)
    {
      // make sure the "engine plugins" end up at the back of the list
      // the reason for this is, that the engine plugins often have a link dependency on runtime plugins and pull their reflection data in right away
      // but then the ezPlugin system doesn't know that certain reflected types actually come from some runtime plugin
      // by loading the editor engine plugins last, this solves that problem
      return isEnginePluginRhs;
    }

    return lhs.m_sAppDirRelativePath.Compare_NoCase(rhs.m_sAppDirRelativePath) < 0; });

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
  if (opt_OutputDir.IsOptionSpecified(nullptr))
  {
    sUserData = opt_OutputDir.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified);
  }


  // make sure these directories exist
  ezFileSystem::CreateDirectoryStructure(sAppDir).AssertSuccess();
  ezFileSystem::CreateDirectoryStructure(sUserData).AssertSuccess();
  ezFileSystem::CreateDirectoryStructure(">sdk/Output/").AssertSuccess();

  ezFileSystem::AddDataDirectory("", "EngineProcess", ":", ezDataDirUsage::AllowWrites).AssertSuccess();                       // for absolute paths
  ezFileSystem::AddDataDirectory(">appdir/", "EngineProcess", "bin", ezDataDirUsage::ReadOnly).AssertSuccess();                // writing to the binary directory
  ezFileSystem::AddDataDirectory(">sdk/Output/", "EngineProcess", "shadercache", ezDataDirUsage::AllowWrites).AssertSuccess(); // for shader files
  ezFileSystem::AddDataDirectory(sAppDir.GetData(), "EngineProcess", "app").AssertSuccess();                                   // app specific data
  ezFileSystem::AddDataDirectory(sUserData, "EngineProcess", "appdata", ezDataDirUsage::AllowWrites).AssertSuccess();          // for writing app user data

  m_CustomFileSystemConfig.Apply();

  {
    // We need the file system before we can start the html logger.
    ezStringBuilder sLogName = "LogEngine";
    if (opt_LogName.IsOptionSpecified(nullptr))
    {
      sLogName = opt_LogName.GetOptionValue(ezCommandLineOption::LogMode::Never);
    }
    ezOsProcessID uiProcessID = ezProcess::GetCurrentProcessID();
    ezStringBuilder sLogFile;
    sLogFile.SetFormat(":appdata/Logs/{0}_{1}.htm", sLogName, uiProcessID);
    m_LogHTML.BeginLog(sLogFile, "EditorEngineProcess");
  }
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
  ezGlobalLog::AddLogWriter(ezLoggingEvent::Handler(&ezLogWriter::HTML::LogMessageHandler, &m_LogHTML));
  ezGlobalLog::AddLogWriter(ezETW::LogMessageHandler);

  ezLog::SetCustomPrintFunction(&EditorPrintFunction);

  // used for sending CVar changes over to the editor
  ezCVar::s_AllCVarEvents.AddEventHandler(ezMakeDelegate(&ezEngineProcessGameApplication::EventHandlerCVar, this));
  ezPlugin::Events().AddEventHandler(ezMakeDelegate(&ezEngineProcessGameApplication::EventHandlerCVarPlugin, this));
}

void ezEngineProcessGameApplication::Deinit_ShutdownLogging()
{
  ezGlobalLog::RemoveLogWriter(ezLoggingEvent::Handler(&ezLogWriter::HTML::LogMessageHandler, &m_LogHTML));
  m_LogHTML.EndLog();

  ezGlobalLog::RemoveLogWriter(ezMakeDelegate(&ezEngineProcessGameApplication::LogWriter, this));

  // used for sending CVar changes over to the editor
  ezCVar::s_AllCVarEvents.RemoveEventHandler(ezMakeDelegate(&ezEngineProcessGameApplication::EventHandlerCVar, this));
  ezPlugin::Events().RemoveEventHandler(ezMakeDelegate(&ezEngineProcessGameApplication::EventHandlerCVarPlugin, this));

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

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED
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
    memoryWriter.WriteBytes(msg.m_Data.GetData(), msg.m_Data.GetCount()).IgnoreResult();

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
