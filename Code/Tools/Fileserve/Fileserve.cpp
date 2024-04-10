#include <Fileserve/FileservePCH.h>

#include <Fileserve/Fileserve.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>

#ifdef EZ_USE_QT
#  include <Fileserve/Gui.moc.h>
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <QApplication>
#endif

#ifdef EZ_USE_QT
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int iCmdShow)
{
#else
int main(int argc, const char** argv)
{
#endif
  ezFileserverApp* pApp = new ezFileserverApp();

#ifdef EZ_USE_QT
  ezCommandLineUtils::GetGlobalInstance()->SetCommandLine();

  int argc = 0;
  char** argv = nullptr;
  QApplication* pQtApplication = new QApplication(argc, const_cast<char**>(argv));
  pQtApplication->setApplicationName("ezFileserve");
  pQtApplication->setOrganizationDomain("www.ezEngine.net");
  pQtApplication->setOrganizationName("ezEngine Project");
  pQtApplication->setApplicationVersion("1.0.0");

  ezRun_Startup(pApp).IgnoreResult();

  CreateFileserveMainWindow(pApp);
  pQtApplication->exec();
  ezRun_Shutdown(pApp);
#else
  pApp->SetCommandLineArguments((ezUInt32)argc, argv);
  ezRun(pApp);
#endif

  const int iReturnCode = pApp->GetReturnCode();
  if (iReturnCode != 0)
  {

    std::string text = pApp->TranslateReturnCode();
    if (!text.empty())
      printf("Return Code: '%s'\n", text.c_str());
  }

#ifdef EZ_USE_QT
  delete pQtApplication;
#endif

  delete pApp;


  return iReturnCode;
}

ezResult ezFileserverApp::BeforeCoreSystemsStartup()
{
  ezStartup::AddApplicationTag("tool");
  ezStartup::AddApplicationTag("fileserve");

  return SUPER::BeforeCoreSystemsStartup();
}

void ezFileserverApp::FileserverEventHandler(const ezFileserverEvent& e)
{
  switch (e.m_Type)
  {
    case ezFileserverEvent::Type::ClientConnected:
    case ezFileserverEvent::Type::ClientReconnected:
      ++m_uiConnections;
      m_TimeTillClosing = ezTime::MakeZero();
      break;
    case ezFileserverEvent::Type::ClientDisconnected:
      --m_uiConnections;

      if (m_uiConnections == 0 && m_CloseAppTimeout.GetSeconds() > 0)
      {
        // reset the timer
        m_TimeTillClosing = ezTime::Now() + m_CloseAppTimeout;
      }

      break;
    default:
      break;
  }
}

void ezFileserverApp::ShaderMessageHandler(ezFileserveClientContext& ctxt, ezRemoteMessage& msg, ezRemoteInterface& clientChannel, ezDelegate<void(const char*)> logActivity)
{
  if (msg.GetMessageID() == 'CMPL')
  {
    for (auto& dd : ctxt.m_MountedDataDirs)
    {
      ezFileSystem::AddDataDirectory(dd.m_sPathOnServer, "FileServe", dd.m_sRootName, ezFileSystem::AllowWrites).IgnoreResult();
    }

    auto& r = msg.GetReader();

    ezStringBuilder tmp;
    ezStringBuilder file, platform;
    ezUInt32 numPermVars;
    ezHybridArray<ezPermutationVar, 16> permVars;

    r >> file;
    r >> platform;
    r >> numPermVars;
    permVars.SetCount(numPermVars);

    tmp.SetFormat("Compiling Shader '{}' - '{}'", file, platform);

    for (auto& pv : permVars)
    {
      r >> pv.m_sName;
      r >> pv.m_sValue;

      tmp.AppendWithSeparator(" | ", pv.m_sName, "=", pv.m_sValue);
    }

    logActivity(tmp);

    // enable runtime shader compilation and set the shader cache directories (this only works, if the user doesn't change the default values)
    // the 'active platform' value should never be used during shader compilation, because there it is passed in
    ezShaderManager::Configure("FILESERVE_UNUSED", true);

    ezLogSystemToBuffer log;
    ezLogSystemScope ls(&log);

    ezShaderCompiler sc;
    ezResult res = sc.CompileShaderPermutationForPlatforms(file, permVars, ezLog::GetThreadLocalLogSystem(), platform);

    ezFileSystem::RemoveDataDirectoryGroup("FileServe");

    if (res.Succeeded())
    {
      // invalidate read cache to not short-circuit the next file read operation
      ezRemoteMessage msg2('FSRV', 'INVC');
      clientChannel.Send(ezRemoteTransmitMode::Reliable, msg2);
    }
    else
    {
      logActivity("[ERROR] Shader Compilation failed:");

      ezHybridArray<ezStringView, 32> lines;
      log.m_sBuffer.Split(false, lines, "\n", "\r");

      for (auto line : lines)
      {
        tmp.Set(">   ", line);
        logActivity(tmp);
      }
    }

    {
      ezRemoteMessage msg2('SHDR', 'CRES');
      msg2.GetWriter() << (res == EZ_SUCCESS);
      msg2.GetWriter() << log.m_sBuffer;

      clientChannel.Send(ezRemoteTransmitMode::Reliable, msg2);
    }
  }
}
