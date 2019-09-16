#pragma once

#include <TestFramework/Framework/TestBaseClass.h>
#include <Foundation/Application/Application.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <Foundation/Configuration/Startup.h>
#include <QApplication>
#include <QSettings>
#include <QtNetwork/QHostInfo>

class ezSceneDocument;

class ezEditorTestApplication : public ezApplication
{
public:
  typedef ezApplication SUPER;

  ezEditorTestApplication();
  virtual ezResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsShutdown() override;
  virtual ApplicationExecution Run() override;


  virtual void AfterCoreSystemsStartup() override;


  virtual void BeforeHighLevelSystemsShutdown() override;

public:
  ezQtEditorApp* m_pEditorApp = nullptr;
};

class ezEditorTest : public ezTestBaseClass
{
  typedef ezTestBaseClass SUPER;
public:
  ezEditorTest();
  ~ezEditorTest();

  virtual ezEditorTestApplication* CreateApplication();

protected:
  virtual ezResult InitializeTest() override;
  virtual ezResult DeInitializeTest() override;

  ezResult CreateAndLoadProject(const char* name);
  ezResult OpenProject(const char* path);
  ezDocument* OpenDocument(const char* subpath);

  void CloseCurrentProject();
  void SafeProfilingData();
  void ProcessEvents(ezUInt32 uiIterations = 1);

  ezEditorTestApplication* m_pApplication = nullptr;
  ezString m_sProjectPath;
};


