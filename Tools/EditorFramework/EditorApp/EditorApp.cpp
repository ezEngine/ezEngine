#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <QProcess>
#include <QTextStream>
#include <QTimer>

EZ_IMPLEMENT_SINGLETON(ezQtEditorApp);

ezQtEditorApp::ezQtEditorApp()
  : m_SingletonRegistrar(this)
  , s_RecentProjects(5)
  , s_RecentDocuments(50)
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
}

ezQtEditorApp::~ezQtEditorApp()
{
  delete m_pTimer;
  m_pTimer = nullptr;
}

ezInt32 ezQtEditorApp::RunEditor()
{
  connect(m_pTimer, SIGNAL(timeout()), this, SLOT(SlotTimedUpdate()), Qt::QueuedConnection);
  m_pTimer->start(1);

  ezInt32 ret = s_pQtApplication->exec();

  ezToolsProject::CloseProject();
  return ret;
}

void ezQtEditorApp::SlotTimedUpdate()
{
  if (ezEditorEngineProcessConnection::GetSingleton())
    ezEditorEngineProcessConnection::GetSingleton()->Update();

  ezAssetCurator::GetSingleton()->MainThreadTick();

  emit IdleEvent();

  m_pTimer->start(1);
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
