#pragma once

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Startup.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <QApplication>
#include <QSettings>
#include <QtNetwork/QHostInfo>
#include <TestFramework/Framework/TestBaseClass.h>
#include <Texture/Image/Image.h>

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
  virtual ezResult GetImage(ezImage& img) override;

protected:
  virtual ezResult InitializeTest() override;
  virtual ezResult DeInitializeTest() override;

  ezResult CreateAndLoadProject(const char* name);
  ezResult OpenProject(const char* path);
  ezDocument* OpenDocument(const char* subpath);
  void ExecuteDocumentAction(const char* szActionName, ezDocument* pDocument, const ezVariant& argument = ezVariant());
  ezResult CaptureImage(ezQtDocumentWindow* pWindow, const char* szImageName);

  void CloseCurrentProject();
  void SafeProfilingData();
  void ProcessEvents(ezUInt32 uiIterations = 1);

  ezEditorTestApplication* m_pApplication = nullptr;
  ezString m_sProjectPath;
  ezImage m_CapturedImage;
};
