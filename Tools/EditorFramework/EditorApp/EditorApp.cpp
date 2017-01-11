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




ezStatus ezQtEditorApp::ExecuteTool(const char* szTool, const QStringList& arguments, ezUInt32 uiSecondsTillTimeout, ezLogInterface* pLogOutput /*= nullptr*/, ezLogMsgType::Enum LogLevel /*= ezLogMsgType::InfoMsg*/)
{
  // this block is supposed to be in the global log, not the given log interface
  EZ_LOG_BLOCK("Executing Tool", szTool);

  QProcess proc;
  QString logoutput;
  proc.setProcessChannelMode(QProcess::MergedChannels);
  proc.setReadChannel(QProcess::StandardOutput);
  QObject::connect(&proc, &QProcess::readyReadStandardOutput, [&proc, &logoutput]() { logoutput.append(proc.readAllStandardOutput()); });
  proc.start(QString::fromUtf8(ezQtEditorApp::GetSingleton()->FindToolApplication(szTool)), arguments);

  if (!proc.waitForStarted(uiSecondsTillTimeout * 1000))
    return ezStatus(ezFmt("{0} could not be started", szTool));

  if (!proc.waitForFinished(uiSecondsTillTimeout * 1000))
    return ezStatus(ezFmt("{0} timed out", szTool));

  if (pLogOutput)
  {
    ezStringBuilder tmp;

    struct LogBlockData
    {
      LogBlockData(ezLogInterface* pInterface, const char* szName)
        : m_Name(szName)
        , m_Block(pInterface, m_Name)
      {
      }

      ezString m_Name;
      ezLogBlock m_Block;
    };

    ezHybridArray<ezUniquePtr<LogBlockData>, 8> blocks;

    QTextStream logoutputStream(&logoutput);
    while (!logoutputStream.atEnd())
    {
      tmp = logoutputStream.readLine().toUtf8().data();
      tmp.Trim(" \n");

      const char* szMsg = nullptr;
      ezLogMsgType::Enum msgType = ezLogMsgType::None;

      if (tmp.StartsWith("Error: "))
      {
        szMsg = &tmp.GetData()[7];
        msgType = ezLogMsgType::ErrorMsg;
      }
      else if (tmp.StartsWith("Warning: "))
      {
        szMsg = &tmp.GetData()[9];
        msgType = ezLogMsgType::WarningMsg;
      }
      else if (tmp.StartsWith("Seriously: "))
      {
        szMsg = &tmp.GetData()[11];
        msgType = ezLogMsgType::SeriousWarningMsg;
      }
      else if (tmp.StartsWith("Success: "))
      {
        szMsg = &tmp.GetData()[9];
        msgType = ezLogMsgType::SuccessMsg;
      }
      else if (tmp.StartsWith("+++++ "))
      {
        tmp.Trim("+ ");
        if (tmp.EndsWith("()"))
          tmp.Trim("() ");

        szMsg = tmp.GetData();
        blocks.PushBack(EZ_DEFAULT_NEW(LogBlockData, pLogOutput, szMsg));
        continue;
      }
      else if (tmp.StartsWith("----- "))
      {
        if (!blocks.IsEmpty())
          blocks.PopBack();

        continue;
      }
      else
      {
        szMsg = &tmp.GetData()[0];
        msgType = ezLogMsgType::InfoMsg;
      }

      if (msgType > LogLevel || szMsg == nullptr)
        continue;

      ezLog::BroadcastLoggingEvent(pLogOutput, msgType, szMsg);
    }

    blocks.Clear();
  }

  if (proc.exitStatus() == QProcess::ExitStatus::CrashExit)
  {
    return ezStatus(ezFmt("{0} crashed during execution", szTool));
  }
  else if (proc.exitCode() != 0)
  {
    return ezStatus(ezFmt("{0} returned error code {1}", szTool, proc.exitCode()));
  }

  return ezStatus(EZ_SUCCESS);
}



