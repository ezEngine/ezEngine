#include <EditorFrameworkPCH.h>

#include <EditorEngineProcessFramework/LongOps/LongOps.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <GuiFoundation/UIServices/QtProgressbar.h>
#include <QProcess>
#include <QTextStream>
#include <QTimer>
#include <ToolsFoundation/Application/ApplicationServices.h>

EZ_IMPLEMENT_SINGLETON(ezQtEditorApp);

ezQtEditorApp::ezQtEditorApp()
  : m_SingletonRegistrar(this)
  , s_RecentProjects(5)
  , s_RecentDocuments(50)
{
  m_pProgressbar = nullptr;
  m_pQtProgressbar = nullptr;
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
    for (auto pDoc : pMan->ezDocumentManager::GetAllOpenDocuments())
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

bool ezQtEditorApp::IsProgressBarProcessingEvents() const
{
  return m_pQtProgressbar != nullptr && m_pQtProgressbar->IsProcessingEvents();
}

void ezQtEditorApp::OnDemandDynamicStringEnumLoad(const char* szEnumName, ezDynamicStringEnum& e)
{
  ezStringBuilder sFile, tmp;
  sFile.Format("Editor/{}.txt", szEnumName);

  ezFileReader file;
  if (file.Open(sFile).Succeeded())
  {
    m_DynamicEnumStringsToClear.Insert(szEnumName);

    sFile.ReadAll(file);

    ezHybridArray<ezStringView, 32> values;

    sFile.Split(false, values, "\n", "\r");

    for (auto val : values)
    {
      e.AddValidValue(val.GetData(tmp));
    }
  }
}

void ezQtEditorApp::ReloadEngineResources()
{
  ezSimpleConfigMsgToEngine msg;
  msg.m_sWhatToDo = "ReloadResources";
  ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}
