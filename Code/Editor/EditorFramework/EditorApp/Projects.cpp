#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/System/Window.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GameEngine/Configuration/InputConfig.h>
#include <GuiFoundation/Dialogs/ModifiedDocumentsDlg.moc.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

void UpdateInputDynamicEnumValues();

void ezQtEditorApp::CloseProject()
{
  QMetaObject::invokeMethod(this, "SlotQueuedCloseProject", Qt::ConnectionType::QueuedConnection);
}

void ezQtEditorApp::SlotQueuedCloseProject()
{
  // purge the image loading queue when a project is closed, but keep the existing cache
  ezQtImageCache::GetSingleton()->StopRequestProcessing(false);

  ezToolsProject::CloseProject();

  // enable image loading again, the queue is purged now
  ezQtImageCache::GetSingleton()->EnableRequestProcessing();
}

ezResult ezQtEditorApp::OpenProject(const char* szProject, bool bImmediate /*= false*/)
{
  if (bImmediate)
  {
    return CreateOrOpenProject(false, szProject);
  }
  else
  {
    QMetaObject::invokeMethod(this, "SlotQueuedOpenProject", Qt::ConnectionType::QueuedConnection, Q_ARG(QString, szProject));
    return EZ_SUCCESS;
  }
}

void ezQtEditorApp::SlotQueuedOpenProject(QString sProject)
{
  // Don't try to execute a queued project open if we have already shutdown the editor.
  if (m_bIsRunning)
    CreateOrOpenProject(false, sProject.toUtf8().data()).IgnoreResult();
}

ezResult ezQtEditorApp::CreateOrOpenProject(bool bCreate, ezStringView sFile0)
{
  ezStringBuilder sFile = sFile0;
  if (!bCreate)
  {
    const ezStatus status = MakeRemoteProjectLocal(sFile);
    if (status.Failed())
    {
      // if the message is empty, the user decided not to continue, so don't show an error message in this case
      if (!status.GetMessageString().IsEmpty())
      {
        ezQtUiServices::GetSingleton()->MessageBoxStatus(status, "Opening remote project failed.");
      }

      return EZ_FAILURE;
    }
  }

  // check that we don't attempt to open a project from a different repository, due to code changes this often doesn't work too well
  if (!IsInHeadlessMode() && !m_bAnyProjectOpened)
  {
    ezStringBuilder sdkDirFromProject;
    if (ezFileSystem::FindFolderWithSubPath(sdkDirFromProject, sFile, "Data/Base", "ezSdkRoot.txt").Succeeded())
    {
      sdkDirFromProject.MakeCleanPath();
      sdkDirFromProject.Trim(nullptr, "/");

      ezStringView sdkDir = ezFileSystem::GetSdkRootDirectory();
      sdkDir.Trim(nullptr, "/");

      if (sdkDirFromProject != sdkDir)
      {
        if (ezQtUiServices::MessageBoxQuestion(ezFmt("You are attempting to open a project that's located in a different SDK directory.\n\nSDK location: '{}'\nProject path: '{}'\n\nThis may make problems.\n\nContinue anyway?", sdkDir, sFile), QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) != QMessageBox::StandardButton::Yes)
        {
          return EZ_FAILURE;
        }
      }
    }
  }

  EZ_PROFILE_SCOPE("CreateOrOpenProject");
  m_bLoadingProjectInProgress = true;
  EZ_SCOPE_EXIT(m_bLoadingProjectInProgress = false;);

  CloseSplashScreen();

  ezStringBuilder sProjectFile = sFile;
  sProjectFile.MakeCleanPath();

  if (bCreate == false && !sProjectFile.EndsWith_NoCase("/ezProject"))
  {
    sProjectFile.AppendPath("ezProject");
  }

  if (ezToolsProject::IsProjectOpen() && ezToolsProject::GetSingleton()->GetProjectFile() == sProjectFile)
  {
    ezQtUiServices::MessageBoxInformation("The selected project is already open");
    return EZ_FAILURE;
  }

  if (!ezToolsProject::CanCloseProject())
    return EZ_FAILURE;

  ezToolsProject::CloseProject();

  // create default plugin selection
  if (!ExistsPluginSelectionStateDDL(sProjectFile))
    CreatePluginSelectionDDL(sProjectFile, "General3D");

  ezStatus res(EZ_SUCCESS);

  if (bCreate)
  {
    if (m_bAnyProjectOpened)
    {
      // if we opened any project before, spawn a new editor instance and open the project there
      // this way, a different set of editor plugins can be loaded
      LaunchEditor(sProjectFile, true);

      QApplication::closeAllWindows();
      return EZ_SUCCESS;
    }
    else
    {
      // once we start loading any plugins, we can't reuse the same instance again for another project
      m_bAnyProjectOpened = true;

      LoadPluginBundleDlls(sProjectFile);

      res = ezToolsProject::CreateProject(sProjectFile);
    }
  }
  else
  {
    if (m_bAnyProjectOpened)
    {
      // if we opened any project before, spawn a new editor instance and open the project there
      // this way, a different set of editor plugins can be loaded
      LaunchEditor(sProjectFile, false);

      QApplication::closeAllWindows();
      return EZ_SUCCESS;
    }
    else
    {
      // once we start loading any plugins, we can't reuse the same instance again for another project
      m_bAnyProjectOpened = true;

      ezStringBuilder sTemp = ezOSFile::GetTempDataFolder("ezEditor");
      sTemp.AppendPath("ezEditorCrashIndicator");
      ezOSFile f;
      if (f.Open(sTemp, ezFileOpenMode::Write, ezFileShareMode::Exclusive).Succeeded())
      {
        f.Write(sTemp.GetData(), sTemp.GetElementCount()).IgnoreResult();
        f.Close();
        m_bWroteCrashIndicatorFile = true;
      }

      {
        ezStringBuilder sProjectDir = sProjectFile;
        sProjectDir.PathParentDirectory();

        ezStringBuilder sSettingsFile = sProjectDir;
        sSettingsFile.AppendPath("Editor/CppProject.ddl");

        // first attempt to load project specific plugin bundles
        ezCppSettings cppSettings;
        if (cppSettings.Load(sSettingsFile).Succeeded())
        {
          ezQtEditorApp::GetSingleton()->DetectAvailablePluginBundles(ezCppProject::GetPluginSourceDir(cppSettings, sProjectDir));
        }

        // now load the plugin DLLs
        LoadPluginBundleDlls(sProjectFile);
      }

      res = ezToolsProject::OpenProject(sProjectFile);
    }
  }

  if (res.Failed())
  {
    ezStringBuilder s;
    s.SetFormat("Failed to open project:\n'{0}'", sProjectFile);

    ezQtUiServices::MessageBoxStatus(res, s);
    return EZ_FAILURE;
  }


  if (m_StartupFlags.AreNoneSet(StartupFlags::SafeMode | StartupFlags::Headless))
  {
    ezStringBuilder sAbsPath;

    if (!m_DocumentsToOpen.IsEmpty())
    {
      for (const auto& doc : m_DocumentsToOpen)
      {
        sAbsPath = doc;

        if (MakeDataDirectoryRelativePathAbsolute(sAbsPath))
        {
          SlotQueuedOpenDocument(sAbsPath.GetData(), nullptr);
        }
        else
        {
          ezLog::Error("Document '{}' does not exist in this project.", doc);
        }
      }

      // don't try to open the same documents when the user switches to another project
      m_DocumentsToOpen.Clear();
    }
    else if (!m_StartupFlags.IsSet(StartupFlags::NoRecent))
    {
      const ezRecentFilesList allDocs = LoadOpenDocumentsList();

      // Unfortunately this crashes in Qt due to the processEvents in the QtProgressBar
      // ezProgressRange range("Restoring Documents", allDocs.GetFileList().GetCount(), true);

      for (auto& doc : allDocs.GetFileList())
      {
        // if (range.WasCanceled())
        //    break;

        // range.BeginNextStep(doc.m_File);
        SlotQueuedOpenDocument(doc.m_File.GetData(), nullptr);
      }

      if (allDocs.GetFileList().IsEmpty())
      {
        OpenDemoDocument();
      }
    }

    if (!ezQtEditorApp::GetSingleton()->IsInSafeMode())
    {
      ezQtContainerWindow::GetContainerWindow()->ScheduleRestoreWindowLayout();
    }
  }
  return EZ_SUCCESS;
}

void ezQtEditorApp::ProjectEventHandler(const ezToolsProjectEvent& r)
{
  switch (r.m_Type)
  {
    case ezToolsProjectEvent::Type::ProjectCreated:
      SetupNewProject();
      m_bSavePreferencesAfterOpenProject = true;
      break;

    case ezToolsProjectEvent::Type::ProjectOpened:
    {
      EZ_PROFILE_SCOPE("ProjectOpened");
      ezDynamicStringEnum::s_RequestUnknownCallback = ezMakeDelegate(&ezQtEditorApp::OnDemandDynamicStringEnumLoad, this);
      LoadProjectPreferences();
      SetupDataDirectories();
      ReadTagRegistry();
      UpdateInputDynamicEnumValues();

      // add project specific translations
      // (these are currently never removed)
      {
        m_pTranslatorFromFiles->AddTranslationFilesFromFolder(":project/Editor/Localization/en");
      }

      // tell the engine process which file system and plugin configuration to use
      ezEditorEngineProcessConnection::GetSingleton()->SetFileSystemConfig(m_FileSystemConfig);
      ezEditorEngineProcessConnection::GetSingleton()->SetPluginConfig(GetRuntimePluginConfig(true));

      ezAssetCurator::GetSingleton()->StartInitialize(m_FileSystemConfig);
      if (ezEditorEngineProcessConnection::GetSingleton()->RestartProcess().Failed())
      {
        EZ_PROFILE_SCOPE("ErrorLog");
        ezLog::Error("Failed to start the engine process. Project loading incomplete.");
      }
      ezAssetCurator::GetSingleton()->WaitForInitialize();

      m_sLastDocumentFolder = ezToolsProject::GetSingleton()->GetProjectFile();
      m_sLastProjectFolder = ezToolsProject::GetSingleton()->GetProjectFile();

      m_RecentProjects.Insert(ezToolsProject::GetSingleton()->GetProjectFile(), 0);

      ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();

      // Make sure preferences are saved, this is important when the project was just created.
      if (m_bSavePreferencesAfterOpenProject)
      {
        m_bSavePreferencesAfterOpenProject = false;
        SaveSettings();
      }
      else
      {
        // Save recent project list on project open in case of crashes or stopping the debugger.
        SaveRecentFiles();
      }

      if (m_StartupFlags.AreNoneSet(ezQtEditorApp::StartupFlags::Headless | ezQtEditorApp::StartupFlags::SafeMode | ezQtEditorApp::StartupFlags::UnitTest | ezQtEditorApp::StartupFlags::Background))
      {
        if (ezCppProject::ExistsProjectCMakeListsTxt())
        {
          ezStatus compilerStatus = ezCppProject::TestCompiler();
          if (compilerStatus.Failed())
          {
            ezQtUiServices::MessageBoxWarning(ezFmt("<html>The compiler preferences are invalid.<br><br>\
              This project has <a href='https://ezengine.net/pages/docs/custom-code/cpp/cpp-project-generation.html'>a dedicated C++ plugin</a> with custom code.<br><br>\
              The compiler set in the preferences does not appear to work, as a result the plugin cannot be compiled <br><br><b>Error:</b> {}</html>",
              compilerStatus.GetMessageString()));
            break;
          }
          else if (ezCppProject::IsBuildRequired())
          {
            const auto clicked = ezQtUiServices::MessageBoxQuestion("<html>Compile this project's C++ plugin?<br><br>\
Explanation: This project has <a href='https://ezengine.net/pages/docs/custom-code/cpp/cpp-project-generation.html'>a dedicated C++ plugin</a> with custom code. The plugin is currently not compiled and therefore the project won't fully work and certain assets will fail to transform.<br><br>\
It is advised to compile the plugin now, but you can also do so later.</html>",
              QMessageBox::StandardButton::Apply | QMessageBox::StandardButton::Ignore, QMessageBox::StandardButton::Apply);

            if (clicked == QMessageBox::StandardButton::Ignore)
              break;

            QTimer::singleShot(1000, this, [this]()
              { ezCppProject::EnsureCppPluginReady().IgnoreResult(); });
          }
        }


        ezTimestamp lastTransform = ezAssetCurator::GetSingleton()->GetLastFullTransformDate().GetTimestamp();

        if (pPreferences->m_bBackgroundAssetProcessing)
        {
          QTimer::singleShot(2000, this, [this]()
            { ezAssetProcessor::GetSingleton()->StartProcessTask(); });
        }
        else if (!lastTransform.IsValid() || (ezTimestamp::CurrentTimestamp() - lastTransform).GetHours() > 5 * 24)
        {
          const auto clicked = ezQtUiServices::MessageBoxQuestion("<html>Apply asset transformation now?<br><br>\
Explanation: For assets to work properly, they must be <a href='https://ezengine.net/pages/docs/assets/assets-overview.html#asset-transform'>transformed</a>. Otherwise they don't function as they should or don't even show up.<br>You can manually run the asset transform from the <a href='https://ezengine.net/pages/docs/assets/asset-browser.html#transform-assets'>asset browser</a> at any time.</html>",
            QMessageBox::StandardButton::Apply | QMessageBox::StandardButton::Ignore, QMessageBox::StandardButton::Apply);

          if (clicked == QMessageBox::StandardButton::Ignore)
          {
            ezAssetCurator::GetSingleton()->StoreFullTransformDate();
            break;
          }

          // check whether the project needs to be transformed
          QTimer::singleShot(2000, this, [this]()
            { ezAssetCurator::GetSingleton()->TransformAllAssets().IgnoreResult(); });
        }
      }

      break;
    }

    case ezToolsProjectEvent::Type::ProjectSaveState:
    {
      m_RecentProjects.Insert(ezToolsProject::GetSingleton()->GetProjectFile(), 0);
      SaveSettings();
      break;
    }

    case ezToolsProjectEvent::Type::ProjectClosing:
    {
      ezShutdownProcessMsgToEngine msg;
      ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
      break;
    }

    case ezToolsProjectEvent::Type::ProjectClosed:
    {
      ezEditorEngineProcessConnection::GetSingleton()->ShutdownProcess();

      ezAssetCurator::GetSingleton()->Deinitialize();

      // remove all data directories that were loaded by the project configuration
      ezApplicationFileSystemConfig::Clear();
      ezFileSystem::SetSpecialDirectory("project", nullptr); // removes this directory

      m_ReloadProjectRequiredReasons.Clear();
      UpdateGlobalStatusBarMessage();

      ezPreferences::ClearProjectPreferences();

      // remove all dynamic enums that were dynamically loaded from the project directory
      {
        for (const auto& val : m_DynamicEnumStringsToClear)
        {
          ezDynamicStringEnum::RemoveEnum(val);
        }
        m_DynamicEnumStringsToClear.Clear();
      }

      break;
    }

    case ezToolsProjectEvent::Type::SaveAll:
    {
      ezToolsProject::SaveProjectState();
      SaveAllOpenDocuments();
      break;
    }

    default:
      break;
  }
}

void ezQtEditorApp::ProjectRequestHandler(ezToolsProjectRequest& r)
{
  switch (r.m_Type)
  {
    case ezToolsProjectRequest::Type::CanCloseProject:
    case ezToolsProjectRequest::Type::CanCloseDocuments:
    {
      if (r.m_bCanClose == false)
        return;

      ezHybridArray<ezDocument*, 32> ModifiedDocs;
      if (r.m_Type == ezToolsProjectRequest::Type::CanCloseProject)
      {
        for (ezDocumentManager* pMan : ezDocumentManager::GetAllDocumentManagers())
        {
          for (ezDocument* pDoc : pMan->GetAllOpenDocuments())
          {
            if (pDoc->IsModified())
              ModifiedDocs.PushBack(pDoc);
          }
        }
      }
      else
      {
        for (ezDocument* pDoc : r.m_Documents)
        {
          if (pDoc->IsModified())
            ModifiedDocs.PushBack(pDoc);
        }
      }

      if (!ModifiedDocs.IsEmpty())
      {
        ezQtModifiedDocumentsDlg dlg(QApplication::activeWindow(), ModifiedDocs);
        if (dlg.exec() == 0)
          r.m_bCanClose = false;
      }
    }
    break;
    case ezToolsProjectRequest::Type::SuggestContainerWindow:
    {
      const auto& docs = GetRecentDocumentsList();
      ezStringBuilder sCleanPath = r.m_Documents[0]->GetDocumentPath();
      sCleanPath.MakeCleanPath();

      for (auto& file : docs.GetFileList())
      {
        if (file.m_File == sCleanPath)
        {
          r.m_iContainerWindowUniqueIdentifier = file.m_iContainerWindow;
          break;
        }
      }
    }
    break;
    case ezToolsProjectRequest::Type::GetPathForDocumentGuid:
    {
      if (ezAssetCurator::ezLockedSubAsset pSubAsset = ezAssetCurator::GetSingleton()->GetSubAsset(r.m_documentGuid))
      {
        r.m_sAbsDocumentPath = pSubAsset->m_pAssetInfo->m_Path;
      }
    }
    break;
  }
}

void ezQtEditorApp::SetupNewProject()
{
  ezToolsProject::GetSingleton()->CreateSubFolder("Editor");
  ezToolsProject::GetSingleton()->CreateSubFolder("RuntimeConfigs");

  // write the default window config
  {
    ezStringBuilder sPath = ezToolsProject::GetSingleton()->GetProjectDirectory();
    sPath.AppendPath("RuntimeConfigs/Window.ddl");

    if (!ezFileSystem::ExistsFile(sPath))
    {
      ezWindowCreationDesc desc;
      desc.m_Title = ezToolsProject::GetSingleton()->GetProjectName(false);
      desc.SaveToDDL(sPath).IgnoreResult();
    }
  }

  // write a stub input mapping
  {
    ezStringBuilder sPath = ezToolsProject::GetSingleton()->GetProjectDirectory();
    sPath.AppendPath("RuntimeConfigs/InputConfig.ddl");

    if (!ezFileSystem::ExistsFile(sPath))
    {
      ezDeferredFileWriter file;
      file.SetOutput(sPath);

      ezHybridArray<ezGameAppInputConfig, 4> actions;
      ezGameAppInputConfig& a = actions.ExpandAndGetRef();
      a.m_sInputSet = "Default";
      a.m_sInputAction = "Interact";
      a.m_bApplyTimeScaling = false;
      a.m_sInputSlotTrigger[0] = ezInputSlot_KeySpace;
      a.m_sInputSlotTrigger[1] = ezInputSlot_MouseButton0;
      a.m_sInputSlotTrigger[2] = ezInputSlot_Controller0_ButtonA;

      ezGameAppInputConfig::WriteToDDL(file, actions);

      file.Close().IgnoreResult();
    }
  }
}
