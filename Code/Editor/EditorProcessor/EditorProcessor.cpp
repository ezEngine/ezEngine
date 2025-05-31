#include <EditorProcessor/EditorProcessorPCH.h>

#include <Foundation/Platform/Win/Utils/IncludeWindows.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessCommunicationChannel.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessorMessages.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Application/Application.h>
#include <Foundation/System/CrashHandler.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

ezCommandLineOptionPath opt_OutputDir("_EditorProcessor", "-outputDir", "Output directory", "");
ezCommandLineOptionBool opt_SaveProfilingData("_EditorProcessor", "-profiling", "Saves performance profiling information into the output folder.", false);
ezCommandLineOptionPath opt_Project("_EditorProcessor", "-project", "Path to the project folder.", "");
ezCommandLineOptionBool opt_Resave("_EditorProcessor", "-resave", "If specified, assets will be resaved.", false);
ezCommandLineOptionString opt_Transform("_EditorProcessor", "-transform", "If specified, assets will be transformed for the given platform profile.\n\
\n\
Example:\n\
  -transform Default\n\
",
  "");
ezCommandLineOptionBool opt_Compile("_EditorProcessor", "-compile", "If specified, the C++ project will be generated and compiled.", false);

class ezEditorProcessorApplication : public ezApplication
{
public:
  using SUPER = ezApplication;

  ezEditorProcessorApplication()
    : ezApplication("ezEditor")
  {
    EnableMemoryLeakReporting(true);
    m_pEditorEngineProcessAppDummy = EZ_DEFAULT_NEW(ezEditorEngineProcessApp);

    m_pEditorApp = new ezQtEditorApp;
  }

  virtual ezResult BeforeCoreSystemsStartup() override
  {
    ezStartup::AddApplicationTag("tool");
    ezStartup::AddApplicationTag("editor");
    ezStartup::AddApplicationTag("editorprocessor");

    ezQtEditorApp::GetSingleton()->InitQt(GetArgumentCount(), (char**)GetArgumentsArray());

    ezString sUserDataFolder = ezApplicationServices::GetSingleton()->GetApplicationUserDataFolder();
    ezString sOutputFolder = opt_OutputDir.GetOptionValue(ezCommandLineOption::LogMode::Never);
    ezCrashHandler_WriteMiniDump::g_Instance.SetDumpFilePath(sOutputFolder.IsEmpty() ? sUserDataFolder : sOutputFolder, "EditorProcessor");
    ezCrashHandler::SetCrashHandler(&ezCrashHandler_WriteMiniDump::g_Instance);

    return EZ_SUCCESS;
  }

  virtual void AfterCoreSystemsShutdown() override
  {
    m_pEditorEngineProcessAppDummy = nullptr;

    ezQtEditorApp::GetSingleton()->DeInitQt();

    delete m_pEditorApp;
    m_pEditorApp = nullptr;
  }

  void EventHandlerIPC(const ezProcessCommunicationChannel::Event& e)
  {
    if (const ezProcessAssetMsg* pMsg = ezDynamicCast<const ezProcessAssetMsg*>(e.m_pMessage))
    {
      if (pMsg->m_sAssetPath.HasExtension("ezPrefab") || pMsg->m_sAssetPath.HasExtension("ezScene"))
      {
        ezQtEditorApp::GetSingleton()->RestartEngineProcessIfPluginsChanged(true);
      }

      ezProcessAssetResponseMsg msg;
      {
        ezLogEntryDelegate logger([&msg](ezLogEntry& ref_entry) -> void
          { msg.m_LogEntries.PushBack(std::move(ref_entry)); },
          ezLogMsgType::WarningMsg);
        ezLogSystemScope logScope(&logger);

        const ezUInt32 uiPlatform = ezAssetCurator::GetSingleton()->FindAssetProfileByName(pMsg->m_sPlatform);

        if (uiPlatform == ezInvalidIndex)
        {
          ezLog::Error("Asset platform config '{0}' is unknown", pMsg->m_sPlatform);
        }
        else
        {
          ezUInt64 uiAssetHash = 0;
          ezUInt64 uiThumbHash = 0;
          ezUInt64 uiPackageHash = 0;

          // TODO: there is currently no 'nice' way to switch the active platform for the asset processors
          // it is also not clear whether this is actually safe to execute here
          ezAssetCurator::GetSingleton()->SetActiveAssetProfileByIndex(uiPlatform);
          // First, force checking for file system changes for the asset and the transitive hull of all dependencies and runtime references. This needs to be done as this EditorProcessor instance might not know all the files yet as some might just have been written. We can't rely on the filesystem watcher as it is not instant and also might just miss some events.
          for (const ezString& sDepOrRef : pMsg->m_DepRefHull)
          {
            if (sDepOrRef.IsAbsolutePath())
            {
              ezAssetCurator::GetSingleton()->NotifyOfFileChange(sDepOrRef);
            }
            else
            {
              ezStringBuilder sTemp = sDepOrRef;
              if (ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sTemp))
              {
                ezAssetCurator::GetSingleton()->NotifyOfFileChange(sTemp);
              }
            }
          }
          ezAssetCurator::GetSingleton()->NotifyOfFileChange(pMsg->m_sAssetPath);

          // Next, we force checking that the asset is up to date. This EditorProcessor instance might not have observed the generation of the output files of various dependencies yet and incorrectly assume that some dependencies still need to be transformed. To prevent this, we force checking the asset and all its dependencies via the filesystem, ignoring the caching.
          ezAssetInfo::TransformState state = ezAssetCurator::GetSingleton()->IsAssetUpToDate(pMsg->m_AssetGuid, ezAssetCurator::GetSingleton()->GetAssetProfile(uiPlatform), nullptr, uiAssetHash, uiThumbHash, uiPackageHash, true);

          if ((uiAssetHash != pMsg->m_AssetHash) || (uiThumbHash != pMsg->m_ThumbHash) || (uiPackageHash != pMsg->m_PackageHash))
          {
            ezLog::Warning("Asset '{}' of state '{}' in processor with hashes '{}|{}|{}' differs from the state in the editor with hashes '{}|{}|{}'", pMsg->m_sAssetPath, (int)state, uiAssetHash, uiThumbHash, uiPackageHash, pMsg->m_AssetHash, pMsg->m_ThumbHash, pMsg->m_PackageHash);
          }

          if (state == ezAssetInfo::NeedsThumbnail || state == ezAssetInfo::NeedsTransform)
          {
            msg.m_Status = ezAssetCurator::GetSingleton()->TransformAsset(pMsg->m_AssetGuid, ezTransformFlags::BackgroundProcessing, ezAssetCurator::GetSingleton()->GetAssetProfile(uiPlatform));

            if (msg.m_Status.Failed())
            {
              // make sure the result message ends up in the log
              ezLog::Error("{}", msg.m_Status.m_sMessage);
            }

            // As there is no game loop that would progress frames in the engine process as it only waits for messages, we have to forcefully destroy pending deletion in the GAL after each transform or the process might never free those resources if it doesn't get a thumbnail job to do which has to tick the render loop.
            ezSimpleConfigMsgToEngine msg;
            msg.m_sWhatToDo = "FreeGalResources";
            ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
          }
          else if (state == ezAssetInfo::UpToDate)
          {
            msg.m_Status = ezTransformStatus();
            ezLog::Warning("Asset already up to date: '{}'", pMsg->m_sAssetPath);
          }
          else
          {
            msg.m_Status = ezTransformStatus(ezFmt("Asset {} is in state {}, can't process asset.", pMsg->m_sAssetPath, (int)state)); // TODO nicer state to string
            ezLog::Error("{}", msg.m_Status.m_sMessage);
          }
        }
      }
      m_IPC.SendMessage(&msg);
    }
    else if (const ezFreeAllResourcesMsg* pMsg = ezDynamicCast<const ezFreeAllResourcesMsg*>(e.m_pMessage))
    {
      // We have no more jobs for this processor so let's tell the engine process to free up resources.
      ezSimpleConfigMsgToEngine msg;
      msg.m_sWhatToDo = "FreeAllResources";
      ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
    }
  }

  virtual void Run() override
  {
    {
      ezStringBuilder cmdHelp;
      if (ezCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, ezCommandLineOption::LogAvailableModes::IfHelpRequested, "_EditorProcessor;cvar"))
      {
        ezQtUiServices::GetSingleton()->MessageBoxInformation(cmdHelp);
        RequestApplicationQuit();
        return;
      }
    }

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    // Setting this flags prevents Windows from showing a dialog when the Engine process crashes
    // this also speeds up process termination significantly (down to less than a second)
    DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
    SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
#endif
    const ezString sTransformProfile = opt_Transform.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified);
    const bool bCompile = opt_Compile.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified);
    const bool bResave = opt_Resave.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified);
    const bool bBackgroundMode = sTransformProfile.IsEmpty() && !bResave && !bCompile;
    const ezString sOutputDir = opt_OutputDir.GetOptionValue(ezCommandLineOption::LogMode::Always);
    const ezBitflags<ezQtEditorApp::StartupFlags> startupFlags = bBackgroundMode ? ezQtEditorApp::StartupFlags::Headless | ezQtEditorApp::StartupFlags::Background : ezQtEditorApp::StartupFlags::Headless;
    ezQtEditorApp::GetSingleton()->StartupEditor(startupFlags, sOutputDir);
    ezQtUiServices::SetHeadless(true);

    const ezStringBuilder sProject = opt_Project.GetOptionValue(ezCommandLineOption::LogMode::Always);

    if (!sTransformProfile.IsEmpty() || bCompile)
    {
      if (ezQtEditorApp::GetSingleton()->OpenProject(sProject).Failed())
      {
        SetReturnCode(2);
        RequestApplicationQuit();
        return;
      }

      // before we transform any assets or if specifically asked, make sure the C++ code is built
      {
        ezCppSettings cppSettings;
        if (cppSettings.Load().Succeeded())
        {
          if (ezCppProject::BuildCodeIfNecessary(cppSettings).Failed())
          {
            SetReturnCode(3);
            RequestApplicationQuit();
            return;
          }

          ezQtEditorApp::GetSingleton()->RestartEngineProcessIfPluginsChanged(true);
        }
      }

      if (!sTransformProfile.IsEmpty())
      {
        bool bTransform = true;

        ezQtEditorApp::GetSingleton()->connect(ezQtEditorApp::GetSingleton(), &ezQtEditorApp::IdleEvent, ezQtEditorApp::GetSingleton(), [this, &bTransform, &sTransformProfile]()
          {
          if (!bTransform)
            return;

          bTransform = false;

          const ezUInt32 uiPlatform = ezAssetCurator::GetSingleton()->FindAssetProfileByName(sTransformProfile);

          if (uiPlatform == ezInvalidIndex)
          {
            ezLog::Error("Asset platform config '{0}' is unknown", sTransformProfile);
          }
          else
          {
            ezStatus status = ezAssetCurator::GetSingleton()->TransformAllAssets(ezTransformFlags::TriggeredManually, ezAssetCurator::GetSingleton()->GetAssetProfile(uiPlatform));
            if (status.Failed())
            {
              status.LogFailure();
              SetReturnCode(1);
            }

            if (opt_SaveProfilingData.GetOptionValue(ezCommandLineOption::LogMode::Always))
            {
              ezActionContext context;
              ezActionManager::ExecuteAction("Engine", "Editor.SaveProfiling", context).IgnoreResult();
            }
          }

          QApplication::quit(); });
      }
      else
      {
        ezQtEditorApp::GetSingleton()->connect(ezQtEditorApp::GetSingleton(), &ezQtEditorApp::IdleEvent, ezQtEditorApp::GetSingleton(), [this]()
          { QApplication::quit(); });
      }

      const ezInt32 iReturnCode = ezQtEditorApp::GetSingleton()->RunEditor();
      if (iReturnCode != 0)
        SetReturnCode(iReturnCode);
    }
    else if (opt_Resave.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified))
    {
      ezQtEditorApp::GetSingleton()->OpenProject(sProject).IgnoreResult();

      ezQtEditorApp::GetSingleton()->connect(ezQtEditorApp::GetSingleton(), &ezQtEditorApp::IdleEvent, ezQtEditorApp::GetSingleton(), [this]()
        {
        ezAssetCurator::GetSingleton()->ResaveAllAssets("");

        if (opt_SaveProfilingData.GetOptionValue(ezCommandLineOption::LogMode::Always))
        {
          ezActionContext context;
          ezActionManager::ExecuteAction("Engine", "Editor.SaveProfiling", context).IgnoreResult();
          }

        QApplication::quit(); });

      const ezInt32 iReturnCode = ezQtEditorApp::GetSingleton()->RunEditor();
      if (iReturnCode != 0)
        SetReturnCode(iReturnCode);
    }
    else
    {
      ezResult res = m_IPC.ConnectToHostProcess();
      if (res.Succeeded())
      {
        m_IPC.m_Events.AddEventHandler(ezMakeDelegate(&ezEditorProcessorApplication::EventHandlerIPC, this));

        ezQtEditorApp::GetSingleton()->OpenProject(sProject).IgnoreResult();
        ezQtEditorApp::GetSingleton()->connect(ezQtEditorApp::GetSingleton(), &ezQtEditorApp::IdleEvent, ezQtEditorApp::GetSingleton(), [this]()
          {
          static bool bRecursionBlock = false;
          if (bRecursionBlock)
            return;
          bRecursionBlock = true;

          if (!m_IPC.IsHostAlive())
            QApplication::quit();

          m_IPC.WaitForMessages();

          bRecursionBlock = false; });

        const ezInt32 iReturnCode = ezQtEditorApp::GetSingleton()->RunEditor();
        SetReturnCode(iReturnCode);
      }
      else
      {
        ezLog::Error("Failed to connect with host process");
      }
    }

    ezQtEditorApp::GetSingleton()->ShutdownEditor();
    RequestApplicationQuit();
  }

private:
  ezQtEditorApp* m_pEditorApp;
  ezEngineProcessCommunicationChannel m_IPC;
  ezUniquePtr<ezEditorEngineProcessApp> m_pEditorEngineProcessAppDummy;
};

EZ_APPLICATION_ENTRY_POINT(ezEditorProcessorApplication);
