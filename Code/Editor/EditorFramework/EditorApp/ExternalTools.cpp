#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

ezString ezQtEditorApp::FindToolApplication(const char* szToolName)
{
  ezStringBuilder toolExe = szToolName;

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  toolExe.ChangeFileExtension("exe");
#else
  toolExe.RemoveFileExtension();
#endif

  szToolName = toolExe;

  ezEditorPreferencesUser* pPref = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();

  ezHybridArray<ezString, 3> sFolders;
  sFolders.PushBack(ezApplicationServices::GetSingleton()->GetPrecompiledToolsFolder(false));
  sFolders.PushBack(ezApplicationServices::GetSingleton()->GetPrecompiledToolsFolder(true));

  if (pPref->m_bUsePrecompiledTools)
  {
    if (!pPref->m_sCustomPrecompiledToolsFolder.IsEmpty() && ezOSFile::ExistsDirectory(pPref->m_sCustomPrecompiledToolsFolder))
    {
      ezStringBuilder customToolsFolder = pPref->m_sCustomPrecompiledToolsFolder;
      customToolsFolder.MakeCleanPath();
      sFolders.PushBack(customToolsFolder);
      ezMath::Swap(sFolders[0], sFolders[2]);
    }
    else
    {
      ezMath::Swap(sFolders[0], sFolders[1]);
    }
  }

  ezStringBuilder sTool;
  for (auto& folder : sFolders)
  {
    sTool = folder;
    sTool.AppendPath(szToolName);

    if (ezOSFile::ExistsFile(sTool))
      return sTool;
  }

  // just try the one in the same folder as the editor
  return szToolName;
}

ezStatus ezQtEditorApp::ExecuteTool(const char* szTool, const QStringList& arguments, ezUInt32 uiSecondsTillTimeout, ezLogInterface* pLogOutput /*= nullptr*/, ezLogMsgType::Enum logLevel /*= ezLogMsgType::InfoMsg*/, const char* szCWD /*= nullptr*/)
{
  // this block is supposed to be in the global log, not the given log interface
  EZ_LOG_BLOCK("Executing Tool", szTool);

  ezStringBuilder toolExe = szTool;

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  toolExe.ChangeFileExtension("exe");
#else
  toolExe.RemoveFileExtension();
#endif

  szTool = toolExe;

  ezStringBuilder cmd;
  for (ezInt32 i = 0; i < arguments.size(); ++i)
    cmd.Append(" ", arguments[i].toUtf8().data());

  ezLog::Debug("{}{}", szTool, cmd);


  QProcess proc;

  if (szCWD != nullptr)
  {
    proc.setWorkingDirectory(szCWD);
  }

  QString logoutput;
  proc.setProcessChannelMode(QProcess::MergedChannels);
  proc.setReadChannel(QProcess::StandardOutput);
  QObject::connect(&proc, &QProcess::readyReadStandardOutput, [&proc, &logoutput]()
    { logoutput.append(proc.readAllStandardOutput()); });
  ezString toolPath = ezQtEditorApp::GetSingleton()->FindToolApplication(szTool);
  proc.start(QString::fromUtf8(toolPath, toolPath.GetElementCount()), arguments);

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

        // TODO: output all logged data in one big message, if the tool failed
      }

      if (msgType > logLevel || szMsg == nullptr)
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

ezString ezQtEditorApp::BuildFileserveCommandLine() const
{
  const ezStringBuilder sToolPath = ezQtEditorApp::GetSingleton()->FindToolApplication("ezFileserve");
  const ezStringBuilder sProjectDir = ezToolsProject::GetSingleton()->GetProjectDirectory();
  ezStringBuilder params;

  ezStringBuilder cmd;
  cmd.Set(sToolPath, " -specialdirs project \"", sProjectDir, "\" -fs_start");

  return cmd;
}

void ezQtEditorApp::RunFileserve()
{
  const ezStringBuilder sToolPath = ezQtEditorApp::GetSingleton()->FindToolApplication("ezFileserve");
  const ezStringBuilder sProjectDir = ezToolsProject::GetSingleton()->GetProjectDirectory();

  QStringList args;
  args << "-specialdirs"
       << "project" << sProjectDir.GetData() << "-fs_start";

  QProcess::startDetached(sToolPath.GetData(), args);
}

void ezQtEditorApp::RunInspector()
{
  const ezStringBuilder sToolPath = ezQtEditorApp::GetSingleton()->FindToolApplication("ezInspector");
  QStringList args;

  QProcess::startDetached(sToolPath.GetData(), args);
}

void ezQtEditorApp::RunTracy()
{
#if BUILDSYSTEM_ENABLE_TRACY_SUPPORT == 0
  ezQtUiServices::MessageBoxInformation("<html>This build of EZ was compiled without support for Tracy profiling.<br><br>See <a href='https://ezengine.net/pages/docs/debugging/tracy.html'>the documentation</a> for how to enable it.</html>");
#else
  const ezStringBuilder sToolPath = ezQtEditorApp::GetSingleton()->FindToolApplication("tracy-profiler");
  QStringList args;

  QProcess::startDetached(sToolPath.GetData(), args);
#endif
}
