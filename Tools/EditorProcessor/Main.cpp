#include <PCH.h>
#include <Core/Application/Application.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <QApplication>
#include <QSettings>
#include <QtNetwork/QHostInfo>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <EditorFramework/IPC/ProcessCommunication.h>

class ezEditorApplication : public ezApplication
{
public:
  ezEditorApplication()
  {
    EnableMemoryLeakReporting(true);

    m_pEditorApp = new ezQtEditorApp;
  }

  virtual void BeforeCoreStartup() override
  {
    ezQtEditorApp::GetSingleton()->InitQt(GetArgumentCount(), (char**)GetArgumentsArray());
  }

  virtual void AfterCoreShutdown() override
  {
    ezQtEditorApp::GetSingleton()->DeInitQt();

    delete m_pEditorApp;
    m_pEditorApp = nullptr;
  }

  void EventHandlerIPC(const ezProcessCommunication::Event& e)
  {
    if (const ezProcessAsset* pMsg = ezDynamicCast<const ezProcessAsset*>(e.m_pMessage))
    {
      ezStatus res = ezAssetCurator::GetSingleton()->TransformAsset(pMsg->m_AssetGuid);
      ezProcessAssetResponse msg;
      msg.m_bSuccess = res.m_Result.Succeeded();
      m_IPC.SendMessage(&msg);
    }
  }

  virtual ApplicationExecution Run() override
  {
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    // Setting this flags prevents Windows from showing a dialog when the Engine process crashes
    // this also speeds up process termination significantly (down to less than a second)
    DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
    SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
#endif

    ezQtEditorApp::GetSingleton()->StartupEditor(true);
    ezQtUiServices::SetHeadless(true);

    {
      ezResult res = m_IPC.ConnectToHostProcess();
      if (res.Succeeded())
      {
        m_IPC.m_Events.AddEventHandler(ezMakeDelegate(&ezEditorApplication::EventHandlerIPC, this));

        ezStringBuilder sProject = ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-project");
        ezQtEditorApp::GetSingleton()->OpenProject(sProject);
        ezQtEditorApp::GetSingleton()->connect(ezQtEditorApp::GetSingleton(), &ezQtEditorApp::IdleEvent, ezQtEditorApp::GetSingleton(),
          [this]()
        {
          static bool bRecursionBlock = false;
          if (bRecursionBlock)
            return;
          bRecursionBlock = true;

          if (!m_IPC.IsHostAlive())
            QApplication::quit();

          m_IPC.ProcessMessages();

          bRecursionBlock = false;
        });

        const ezInt32 iReturnCode = ezQtEditorApp::GetSingleton()->RunEditor();
        SetReturnCode(iReturnCode);
      }
      else
      {

      }
    }

    ezQtEditorApp::GetSingleton()->ShutdownEditor();

    return ezApplication::Quit;
  }

private:
  ezQtEditorApp* m_pEditorApp;
  ezProcessCommunication m_IPC;
};

EZ_APPLICATION_ENTRY_POINT(ezEditorApplication);

