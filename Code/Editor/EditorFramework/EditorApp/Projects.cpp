#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
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
  CreateOrOpenProject(false, sProject.toUtf8().data()).IgnoreResult();
}


ezResult ezQtEditorApp::CreateOrOpenProject(bool bCreate, const char* szFile)
{
  // check that we don't attempt to open a project from a different repository, due to code changes this often doesn't work too well
  {
    ezStringBuilder sdkDir;
    if (ezFileSystem::FindFolderWithSubPath(sdkDir, szFile, "Data/Base", "ezSdkRoot.txt").Succeeded())
    {
      sdkDir.MakeCleanPath();
      if (sdkDir != ezFileSystem::GetSdkRootDirectory())
      {
        if (ezQtUiServices::MessageBoxQuestion(ezFmt("You are attempting to open a project that's located in a different SDK directory.\n\nSDK location: '{}'\nProject path: '{}'\n\nThis may make problems.\n\nContinue anyway?", ezFileSystem::GetSdkRootDirectory(), szFile), QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) != QMessageBox::StandardButton::Yes)
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

  ezStringBuilder sFile = szFile;
  sFile.MakeCleanPath();

  if (bCreate == false && !sFile.EndsWith_NoCase("/ezProject"))
  {
    sFile.AppendPath("ezProject");
  }

  if (ezToolsProject::IsProjectOpen() && ezToolsProject::GetSingleton()->GetProjectFile() == sFile)
  {
    ezQtUiServices::MessageBoxInformation("The selected project is already open");
    return EZ_FAILURE;
  }

  if (!ezToolsProject::CanCloseProject())
    return EZ_FAILURE;

  ezStatus res;
  if (bCreate)
    res = ezToolsProject::CreateProject(sFile);
  else
    res = ezToolsProject::OpenProject(sFile);

  if (res.m_Result.Failed())
  {
    ezStringBuilder s;
    s.Format("Failed to open project:\n'{0}'", sFile);

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
      m_bSavePreferencesAfterOpenProject = true;
      break;

    case ezToolsProjectEvent::Type::ProjectOpened:
    {
      EZ_PROFILE_SCOPE("ProjectOpened");
      ezDynamicStringEnum::s_RequestUnknownCallback = ezMakeDelegate(&ezQtEditorApp::OnDemandDynamicStringEnumLoad, this);
      LoadProjectPreferences();
      SetupDataDirectories();
      ReadEnginePluginConfig();
      ReadTagRegistry();
      UpdateInputDynamicEnumValues();

      // add project specific translations
      // (these are currently never removed)
      {
        m_pTranslatorFromFiles->AddTranslationFilesFromFolder(":project/Editor/Localization/en");
      }

      // tell the engine process which file system and plugin configuration to use
      ezEditorEngineProcessConnection::GetSingleton()->SetFileSystemConfig(m_FileSystemConfig);
      ezEditorEngineProcessConnection::GetSingleton()->SetPluginConfig(m_EnginePluginConfig);

      ezAssetCurator::GetSingleton()->StartInitialize(m_FileSystemConfig);
      if (ezEditorEngineProcessConnection::GetSingleton()->RestartProcess().Failed())
      {
        EZ_PROFILE_SCOPE("ErrorLog");
        ezLog::Error("Failed to start the engine process. Project loading incomplete.");
      }
      ezAssetCurator::GetSingleton()->WaitForInitialize();

      m_sLastDocumentFolder = ezToolsProject::GetSingleton()->GetProjectFile();
      m_sLastProjectFolder = ezToolsProject::GetSingleton()->GetProjectFile();

      s_RecentProjects.Insert(ezToolsProject::GetSingleton()->GetProjectFile(), 0);

      ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();

      if (m_StartupFlags.AreNoneSet(ezQtEditorApp::StartupFlags::Headless | ezQtEditorApp::StartupFlags::SafeMode | ezQtEditorApp::StartupFlags::UnitTest) && pPreferences->m_bBackgroundAssetProcessing)
      {
        QTimer::singleShot(1000, this, [this]() { ezAssetProcessor::GetSingleton()->RestartProcessTask(); });
      }

      // Make sure preferences are saved, this is important when the project was just created.
      if (m_bSavePreferencesAfterOpenProject)
      {
        m_bSavePreferencesAfterOpenProject = false;
        SaveSettings();
      }
      break;
    }

    case ezToolsProjectEvent::Type::ProjectClosing:
    {
      s_RecentProjects.Insert(ezToolsProject::GetSingleton()->GetProjectFile(), 0);
      SaveSettings();

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

      s_ReloadProjectRequiredReasons.Clear();
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
      SaveSettings();
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
        r.m_sAbsDocumentPath = pSubAsset->m_pAssetInfo->m_sAbsolutePath;
      }
    }
    break;
  }
}
