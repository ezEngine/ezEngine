#include <EditorProcessor/EditorProcessorPCH.h>

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessCommunicationChannel.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GuiFoundation/Action/ActionManager.h>

ezCommandLineOptionPath opt_OutputDir("_EditorProcessor", "-outputDir", "Output directory", "");
ezCommandLineOptionBool opt_Debug("_EditorProcessor", "-debug", "Writes various debug logs into the output folder.", false);
ezCommandLineOptionPath opt_Project("_EditorProcessor", "-project", "Path to the project folder.", "");
ezCommandLineOptionString opt_Transform("_EditorProcessor", "-transform", "If specified, assets will be transformed for the given platform profile.\n\
\n\
Example:\n\
  -transform PC\n\
",
  "");

class ezEditorApplication : public ezApplication
{
public:
  typedef ezApplication SUPER;

  ezEditorApplication()
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
      ezProcessAssetResponseMsg msg;
      {
        ezLogEntryDelegate logger([&msg](ezLogEntry& entry) -> void { msg.m_LogEntries.PushBack(std::move(entry)); }, ezLogMsgType::WarningMsg);
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

          // TODO: there is currently no 'nice' way to switch the active platform for the asset processors
          // it is also not clear whether this is actually safe to execute here
          ezAssetCurator::GetSingleton()->SetActiveAssetProfileByIndex(uiPlatform);

          ezAssetInfo::TransformState state = ezAssetCurator::GetSingleton()->IsAssetUpToDate(pMsg->m_AssetGuid, ezAssetCurator::GetSingleton()->GetAssetProfile(uiPlatform), nullptr, uiAssetHash, uiThumbHash);

          // Check if asset matches the state of the editor
          if ((state != ezAssetInfo::NeedsThumbnail && state != ezAssetInfo::NeedsTransform) || uiAssetHash != pMsg->m_AssetHash || uiThumbHash != pMsg->m_ThumbHash)
          {
            // Force update the state. If the asset was created automatically the file might not be known yet.
            ezAssetCurator::GetSingleton()->NotifyOfFileChange(pMsg->m_sAssetPath);
            state = ezAssetCurator::GetSingleton()->IsAssetUpToDate(pMsg->m_AssetGuid, ezAssetCurator::GetSingleton()->GetAssetProfile(uiPlatform), nullptr, uiAssetHash, uiThumbHash, true);
          }

          if (uiAssetHash != pMsg->m_AssetHash || uiThumbHash != pMsg->m_ThumbHash)
          {
            ezLog::Warning("Asset '{}' of state '{}' in processor with hashes '{}{}' differs from the state in the editor with hashes '{}{}'", pMsg->m_sAssetPath, (int)state, uiAssetHash, uiThumbHash, pMsg->m_AssetHash, pMsg->m_ThumbHash);
          }

          if (state == ezAssetInfo::NeedsThumbnail || state == ezAssetInfo::NeedsTransform)
          {
            const ezStatus res = ezAssetCurator::GetSingleton()->TransformAsset(pMsg->m_AssetGuid, ezTransformFlags::None, ezAssetCurator::GetSingleton()->GetAssetProfile(uiPlatform));

            msg.m_bSuccess = res.m_Result.Succeeded();
            if (res.m_Result.Failed())
            {
              // make sure the result message ends up in the log
              ezLog::Error(res.m_sMessage);
            }
          }
          else if (state == ezAssetInfo::UpToDate)
          {
            msg.m_bSuccess = true;
            ezLog::Warning("Asset already up to date: '{}'", pMsg->m_sAssetPath);
          }
          else
          {
            msg.m_bSuccess = false;
            ezLog::Error("Asset {} is in state {}, can't process asset.", pMsg->m_sAssetPath, (int)state);
          }
        }
      }
      m_IPC.SendMessage(&msg);
    }
  }

  virtual Execution Run() override
  {
    {
      ezStringBuilder cmdHelp;
      if (ezCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, ezCommandLineOption::LogAvailableModes::IfHelpRequested, "_EditorProcessor;cvar"))
      {
        ezQtUiServices::GetSingleton()->MessageBoxInformation(cmdHelp);
        return ezApplication::Execution::Quit;
      }
    }

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    // Setting this flags prevents Windows from showing a dialog when the Engine process crashes
    // this also speeds up process termination significantly (down to less than a second)
    DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
    SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
#endif
    const ezString sOutputDir = opt_OutputDir.GetOptionValue(ezCommandLineOption::LogMode::Always);
    ezQtEditorApp::GetSingleton()->StartupEditor(ezQtEditorApp::StartupFlags::Headless, sOutputDir);
    ezQtUiServices::SetHeadless(true);

    const ezStringBuilder sProject = opt_Project.GetOptionValue(ezCommandLineOption::LogMode::Always);

    if (!ezStringUtils::IsNullOrEmpty(opt_Transform.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified)))
    {
      ezQtEditorApp::GetSingleton()->OpenProject(sProject).IgnoreResult();

      bool bTransform = true;

      ezQtEditorApp::GetSingleton()->connect(ezQtEditorApp::GetSingleton(), &ezQtEditorApp::IdleEvent, ezQtEditorApp::GetSingleton(), [this, &bTransform]() {
        if (!bTransform)
          return;

        bTransform = false;

        const char* szPlatform = opt_Transform.GetOptionValue(ezCommandLineOption::LogMode::Never);
        const ezUInt32 uiPlatform = ezAssetCurator::GetSingleton()->FindAssetProfileByName(szPlatform);

        if (uiPlatform == ezInvalidIndex)
        {
          ezLog::Error("Asset platform config '{0}' is unknown", szPlatform);
        }
        else
        {
          ezStatus status = ezAssetCurator::GetSingleton()->TransformAllAssets(ezTransformFlags::TriggeredManually, ezAssetCurator::GetSingleton()->GetAssetProfile(uiPlatform));
          if (status.Failed())
          {
            status.LogFailure();
            SetReturnCode(1);
          }

          if (opt_Debug.GetOptionValue(ezCommandLineOption::LogMode::Always))
          {
            ezActionContext context;
            ezActionManager::ExecuteAction("Engine", "Editor.SaveProfiling", context).IgnoreResult();
          }
        }

        QApplication::quit();
      });

      const ezInt32 iReturnCode = ezQtEditorApp::GetSingleton()->RunEditor();
      if (iReturnCode != 0)
        SetReturnCode(iReturnCode);
    }
    else
    {
      ezResult res = m_IPC.ConnectToHostProcess();
      if (res.Succeeded())
      {
        m_IPC.m_Events.AddEventHandler(ezMakeDelegate(&ezEditorApplication::EventHandlerIPC, this));

        ezQtEditorApp::GetSingleton()->OpenProject(sProject).IgnoreResult();
        ezQtEditorApp::GetSingleton()->connect(ezQtEditorApp::GetSingleton(), &ezQtEditorApp::IdleEvent, ezQtEditorApp::GetSingleton(), [this]() {
          static bool bRecursionBlock = false;
          if (bRecursionBlock)
            return;
          bRecursionBlock = true;

          if (!m_IPC.IsHostAlive())
            QApplication::quit();

          m_IPC.WaitForMessages();

          bRecursionBlock = false;
        });

        const ezInt32 iReturnCode = ezQtEditorApp::GetSingleton()->RunEditor();
        SetReturnCode(iReturnCode);
      }
      else
      {
        ezLog::Error("Failed to connect with host process");
      }
    }

    ezQtEditorApp::GetSingleton()->ShutdownEditor();

    return ezApplication::Execution::Quit;
  }

private:
  ezQtEditorApp* m_pEditorApp;
  ezEngineProcessCommunicationChannel m_IPC;
  ezUniquePtr<ezEditorEngineProcessApp> m_pEditorEngineProcessAppDummy;
};

EZ_APPLICATION_ENTRY_POINT(ezEditorApplication);
