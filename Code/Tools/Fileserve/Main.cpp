#include <FileservePCH.h>

#include <Fileserve/Main.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Utilities/CommandLineUtils.h>

#ifdef EZ_USE_QT
#include <Gui.moc.h>
#include <QApplication>
#include <Windows.h>
#endif

#ifdef EZ_USE_QT
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
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

  ezRun_Startup(pApp);

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

  delete pApp;

  return iReturnCode;
}

void ezFileserverApp::BeforeCoreSystemsStartup()
{
  ezStartup::AddApplicationTag("tool");
  ezStartup::AddApplicationTag("fileserve");

  ezApplication::BeforeCoreSystemsStartup();
}

void ezFileserverApp::FileserverEventHandler(const ezFileserverEvent& e)
{
  switch (e.m_Type)
  {
    case ezFileserverEvent::Type::ClientConnected:
    case ezFileserverEvent::Type::ClientReconnected:
      ++m_uiConnections;
      m_TimeTillClosing.SetZero();
      break;
    case ezFileserverEvent::Type::ClientDisconnected:
      --m_uiConnections;

      if (m_uiConnections == 0 && m_CloseAppTimeout.GetSeconds() > 0)
      {
        // reset the timer
        m_TimeTillClosing = ezTime::Now() + m_CloseAppTimeout;
      }

      break;
  }
}
