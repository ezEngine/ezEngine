#include <EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <QProcess>
#include <QTextStream>
#include <QTimer>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <EditorEngineProcessFramework/LongOperation/LongOperation.h>

EZ_IMPLEMENT_SINGLETON(ezQtEditorApp);

ezEventSubscriptionID subId;

ezQtEditorApp::ezQtEditorApp()
  : m_SingletonRegistrar(this)
  , s_RecentProjects(5)
  , s_RecentDocuments(50)
  , m_LongOperationManager(ezLongOperationManager::ReplicationMode::OnlyRemoteOperations)
{
  m_pProgressbar = nullptr;
  m_pQtProgressbar = nullptr;
  m_bSafeMode = false;
  m_bHeadless = false;
  m_bSavePreferencesAfterOpenProject = false;

  ezApplicationServices::GetSingleton()->SetApplicationName("ezEditor");
  s_pQtApplication = nullptr;
  s_pEngineViewProcess = nullptr;

  m_pTimer = new QTimer(nullptr);

  subId = m_LongOperationManager.m_Events.AddEventHandler([&](const ezLongOperationManagerEvent& e) {
    const auto& opInfo = m_LongOperationManager.GetOperations()[e.m_uiOperationIndex];

    if (e.m_Type == ezLongOperationManagerEvent::Type::OpAdded)
    {
      ezLog::Info("Added op '{}'", opInfo.m_pOperation->GetDisplayName());
    }
    else if (e.m_Type == ezLongOperationManagerEvent::Type::OpFinished)
    {
      ezLog::Info("Finished op '{}'", opInfo.m_pOperation->GetDisplayName());
    }
    else if (e.m_Type == ezLongOperationManagerEvent::Type::OpProgress)
    {
      ezLog::Info("Op '{}' at {}%%", opInfo.m_pOperation->GetDisplayName(), opInfo.m_fCompletion * 100.0f);
    }
  });
}

ezQtEditorApp::~ezQtEditorApp()
{
  delete m_pTimer;
  m_pTimer = nullptr;

  m_LongOperationManager.m_Events.RemoveEventHandler(subId);
}

ezInt32 ezQtEditorApp::RunEditor()
{
  ezInt32 ret = s_pQtApplication->exec();
  return ret;
}

void ezQtEditorApp::SlotTimedUpdate()
{
  if (ezEditorEngineProcessConnection::GetSingleton())
    ezEditorEngineProcessConnection::GetSingleton()->Update();

  ezAssetCurator::GetSingleton()->MainThreadTick();
  ezTaskSystem::FinishFrameTasks();

  Q_EMIT IdleEvent();

  m_pTimer->start(1);
}

void ezQtEditorApp::SlotSaveSettings()
{
  SaveSettings();
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

    default:
      return;
  }
}

void ezQtEditorApp::SaveAllOpenDocuments()
{
  for (auto pMan : ezDocumentManager::GetAllDocumentManagers())
  {
    for (auto pDoc : pMan->ezDocumentManager::GetAllDocuments())
    {
      ezQtDocumentWindow* pWnd = ezQtDocumentWindow::FindWindowByDocument(pDoc);
      if (pWnd)
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
