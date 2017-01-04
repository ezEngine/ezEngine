#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <QTimer>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <QProcess>
#include <QTextStream>

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

ezString ezQtEditorApp::GetExternalToolsFolder(bool bForceUseCustomTools)
{
  ezEditorPreferencesUser* pPref = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();
  return ezApplicationServices::GetSingleton()->GetPrecompiledToolsFolder(bForceUseCustomTools ? false : pPref->m_bUsePrecompiledTools);
}

ezString ezQtEditorApp::FindToolApplication(const char* szToolName)
{
  ezStringBuilder sTool = ezQtEditorApp::GetSingleton()->GetExternalToolsFolder();
  sTool.AppendPath(szToolName);

  if (ezFileSystem::ExistsFile(sTool))
    return sTool;

  sTool = ezQtEditorApp::GetSingleton()->GetExternalToolsFolder(true);
  sTool.AppendPath(szToolName);

  if (ezFileSystem::ExistsFile(sTool))
    return sTool;

  // just try the one in the same folder as the editor
  return szToolName;
}

ezStatus ezQtEditorApp::ExecuteTool(const char* szTool, const QStringList& arguments, ezUInt32 uiSecondsTillTimeout, bool bPipeOutputToLog, bool bOnlyPipeErrors)
{
  QProcess proc;
  QString logoutput;
  proc.setProcessChannelMode(QProcess::MergedChannels);
  proc.setReadChannel(QProcess::StandardOutput);
  QObject::connect(&proc, &QProcess::readyReadStandardOutput, [&proc, &logoutput]() { logoutput.append(proc.readAllStandardOutput()); });
  proc.start(QString::fromUtf8(ezQtEditorApp::GetSingleton()->FindToolApplication(szTool)), arguments);
  auto stat = proc.exitStatus();

  if (!proc.waitForFinished(uiSecondsTillTimeout * 1000))
    return ezStatus(ezFmt("{0} timed out", szTool));

  if (bPipeOutputToLog)
  {
    EZ_LOG_BLOCK("Output", szTool);

    QTextStream logoutputStream(&logoutput);
    while (!logoutputStream.atEnd())
    {
      QString line = logoutputStream.readLine();

      /// \todo This doesn't work because of indentation (startswith fails)
      if (line.startsWith("Error: "))
        ezLog::Error("{0}", &line.toUtf8().data()[7]);
      else if (line.startsWith("Warning: "))
        ezLog::Warning("{0}", &line.toUtf8().data()[9]);
      else if (line.startsWith("Seriously: "))
        ezLog::SeriousWarning("{0}", &line.toUtf8().data()[11]);
      else if (!bOnlyPipeErrors)
        ezLog::Info("{0}", line.toUtf8().data());
    }
  }

  if (proc.exitCode() != 0)
    return ezStatus(ezFmt("{0} returned error code {1}", szTool, proc.exitCode()));

  return ezStatus(EZ_SUCCESS);
}



