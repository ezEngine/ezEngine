#include <Fileserve/Main.h>
#include <Foundation/Utilities/CommandLineUtils.h>

#ifdef EZ_USE_QT
  #include <QApplication>
  #include <Gui.moc.h>
#endif


#ifdef EZ_USE_QT
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int argc, const char** argv)
#endif
{
  ezFileserverApp* pApp = new ezFileserverApp();


#ifdef EZ_USE_QT
  ezCommandLineUtils::GetGlobalInstance()->SetCommandLine();

  int argc = 0;
  char** argv = nullptr;
  QApplication* pQtApplication = new QApplication(argc, const_cast<char**>(argv));

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
    if (!text.empty()) printf("Return Code: '%s'\n", text.c_str());
  }

  delete pApp;

  return iReturnCode;
}
