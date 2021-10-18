#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <GuiFoundation/UIServices/QtProgressbar.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

EZ_IMPLEMENT_SINGLETON(ezQtEditorApp);

ezQtEditorApp::ezQtEditorApp()
  : m_SingletonRegistrar(this)
  , s_RecentProjects(5)
  , s_RecentDocuments(50)
{
  m_bSavePreferencesAfterOpenProject = false;

  ezApplicationServices::GetSingleton()->SetApplicationName("ezEditor");

  m_pTimer = new QTimer(nullptr);
}

ezQtEditorApp::~ezQtEditorApp()
{
  delete m_pTimer;
  m_pTimer = nullptr;

  CloseSplashScreen();
}

ezInt32 ezQtEditorApp::RunEditor()
{
  ezInt32 ret = s_pQtApplication->exec();
  return ret;
}

void ezQtEditorApp::SlotTimedUpdate()
{
  if (ezToolsProject::IsProjectOpen())
  {
    if (ezEditorEngineProcessConnection::GetSingleton())
      ezEditorEngineProcessConnection::GetSingleton()->Update();

    ezAssetCurator::GetSingleton()->MainThreadTick(true);
  }
  ezTaskSystem::FinishFrameTasks();

  // Close the splash screen when we get to the first idle event
  CloseSplashScreen();

  Q_EMIT IdleEvent();

  RestartEngineProcessIfPluginsChanged();

  m_pTimer->start(1);
}

void ezQtEditorApp::SlotSaveSettings()
{
  SaveSettings();
}

void ezQtEditorApp::SlotVersionCheckCompleted(bool bNewVersionReleased, bool bForced)
{
  if (bForced || bNewVersionReleased)
  {
    if (m_VersionChecker.IsLatestNewer())
    {
      ezQtUiServices::GetSingleton()->MessageBoxInformation(
        ezFmt("<html>A new version is available: {}<br><br>Your version is: {}<br><br>Please check the <A "
              "href=\"https://github.com/ezEngine/ezEngine/releases\">Releases</A> for details.</html>",
          m_VersionChecker.GetKnownLatestVersion(), m_VersionChecker.GetOwnVersion()));
    }
    else
    {
      ezStringBuilder tmp("You have the latest version: \n");
      tmp.Append(m_VersionChecker.GetOwnVersion());

      ezQtUiServices::GetSingleton()->MessageBoxInformation(tmp);
    }
  }

  if (m_VersionChecker.IsLatestNewer())
  {
    ezQtUiServices::GetSingleton()->ShowGlobalStatusBarMessage(
      ezFmt("New version '{}' available, please update.", m_VersionChecker.GetKnownLatestVersion()));
  }
}

void ezQtEditorApp::EngineProcessMsgHandler(const ezEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
    case ezEditorEngineProcessConnection::Event::Type::ProcessMessage:
    {
      if (e.m_pMsg->GetDynamicRTTI()->IsDerivedFrom<ezUpdateReflectionTypeMsgToEditor>())
      {
        const ezUpdateReflectionTypeMsgToEditor* pMsg = static_cast<const ezUpdateReflectionTypeMsgToEditor*>(e.m_pMsg);
        ezPhantomRttiManager::RegisterType(pMsg->m_desc);
      }
      else if (e.m_pMsg->GetDynamicRTTI()->IsDerivedFrom<ezProjectReadyMsgToEditor>())
      {
        // This message is waited upon (blocking) but does not contain any data.
      }
    }
    break;

    case ezEditorEngineProcessConnection::Event::Type::ProcessRestarted:
      StoreEnginePluginModificationTimes();
      break;

    default:
      return;
  }
}

void ezQtEditorApp::UiServicesEvents(const ezQtUiServices::Event& e)
{
  if (e.m_Type == ezQtUiServices::Event::Type::CheckForUpdates)
  {
    m_VersionChecker.Check(true);
  }
}

void ezQtEditorApp::SaveAllOpenDocuments()
{
  for (auto pMan : ezDocumentManager::GetAllDocumentManagers())
  {
    for (auto pDoc : pMan->ezDocumentManager::GetAllOpenDocuments())
    {
      ezQtDocumentWindow* pWnd = ezQtDocumentWindow::FindWindowByDocument(pDoc);
      // Layers for example will share a window with the scene document and the window will always save the scene.
      if (pWnd && pWnd->GetDocument() == pDoc)
      {
        if (pWnd->SaveDocument().m_Result.Failed())
          return;
      }
      // There might be no window for this document.
      else
      {
        pDoc->SaveDocument();
      }
    }
  }
}

bool ezQtEditorApp::IsProgressBarProcessingEvents() const
{
  return m_pQtProgressbar != nullptr && m_pQtProgressbar->IsProcessingEvents();
}

void ezQtEditorApp::OnDemandDynamicStringEnumLoad(const char* szEnumName, ezDynamicStringEnum& e)
{
  ezStringBuilder sFile;
  sFile.Format(":project/Editor/{}.txt", szEnumName);

  // enums loaded this way are user editable
  e.SetStorageFile(sFile);
  e.ReadFromStorage();

  m_DynamicEnumStringsToClear.Insert(szEnumName);
}

void ezQtEditorApp::ReloadEngineResources()
{
  ezSimpleConfigMsgToEngine msg;
  msg.m_sWhatToDo = "ReloadResources";
  ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}
