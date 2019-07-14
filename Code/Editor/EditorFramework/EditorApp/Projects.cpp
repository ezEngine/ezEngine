#include <EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Profiling/Profiling.h>
#include <GuiFoundation/Dialogs/ModifiedDocumentsDlg.moc.h>
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

void ezQtEditorApp::OpenProject(const char* szProject)
{
  QMetaObject::invokeMethod(this, "SlotQueuedOpenProject", Qt::ConnectionType::QueuedConnection, Q_ARG(QString, szProject));
}

void ezQtEditorApp::SlotQueuedOpenProject(QString sProject)
{
  CreateOrOpenProject(false, sProject.toUtf8().data());
}


ezResult ezQtEditorApp::CreateOrOpenProject(bool bCreate, const char* szFile)
{
  EZ_PROFILE_SCOPE("CreateOrOpenProject");

  if (ezToolsProject::IsProjectOpen() && ezToolsProject::GetSingleton()->GetProjectFile() == szFile)
  {
    ezQtUiServices::MessageBoxInformation("The selected project is already open");
    return EZ_SUCCESS;
  }

  if (!ezToolsProject::CanCloseProject())
    return EZ_FAILURE;

  ezStatus res;
  if (bCreate)
    res = ezToolsProject::CreateProject(szFile);
  else
    res = ezToolsProject::OpenProject(szFile);

  if (res.m_Result.Failed())
  {
    ezStringBuilder s;
    s.Format("Failed to open project:\n'{0}'", szFile);

    ezQtUiServices::MessageBoxStatus(res, s);
    return EZ_FAILURE;
  }

  if (!m_bSafeMode && !m_bHeadless)
  {
    const ezRecentFilesList allDocs = LoadOpenDocumentsList();

    // Unfortunately this crashes in Qt due to the processEvents in the QtProgressBar
    // ezProgressRange range("Restoring Documents", allDocs.GetFileList().GetCount(), true);

    for (auto& doc : allDocs.GetFileList())
    {
      // if (range.WasCanceled())
      // break;

      // range.BeginNextStep(doc.m_File);
      OpenDocumentQueued(doc.m_File);
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

      // make sure preferences are saved, this is important when the project was just created
      // however, if we switch to an existing project, no documents are yet open and saving the preferences now
      // would then store that "no documents were open", so don't save stuff right away in the regular case
      if (m_bSavePreferencesAfterOpenProject)
      {
        m_bSavePreferencesAfterOpenProject = false;
        SaveSettings();
      }
      else
      {
        // instead, save the settings after a short delay, by then all documents should have been opened
        QTimer::singleShot(1000, this, SLOT(SlotSaveSettings()));
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

      break;
    }

    case ezToolsProjectEvent::Type::SaveAll:
    {
      SaveSettings();
      SaveAllOpenDocuments();
      break;
    }
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
          for (ezDocument* pDoc : pMan->GetAllDocuments())
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
