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
#include <memory>

class ezSceneDocument;
class QMimeData;
class ezScene2Document;

class ezEditorTestApplication : public ezApplication
{
public:
  using SUPER = ezApplication;

  ezEditorTestApplication(ezStringView sTestName);
  virtual ezResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsShutdown() override;
  virtual void Run() override;


  virtual void AfterCoreSystemsStartup() override;


  virtual void BeforeHighLevelSystemsShutdown() override;

public:
  ezQtEditorApp* m_pEditorApp = nullptr;
  ezString m_sTestName;
};

class ezEditorTest : public ezTestBaseClass
{
  using SUPER = ezTestBaseClass;

public:
  ezEditorTest();
  ~ezEditorTest();

  virtual ezEditorTestApplication* CreateApplication();
  virtual ezResult GetImage(ezImage& ref_img, const ezSubTestEntry& subTest, ezUInt32 uiImageNumber) override;

protected:
  virtual ezResult InitializeTest() override;
  virtual ezResult DeInitializeTest() override;

  ezResult CreateAndLoadProject(const char* name);
  /// \brief Opens a project by copying it to a temp location and opening that one.
  /// This ensures that the tests always work on a clean state.
  ezResult OpenProject(const char* path);
  ezDocument* OpenDocument(const char* subpath);
  void ExecuteDocumentAction(const char* szActionName, ezDocument* pDocument, const ezVariant& argument = ezVariant());
  ezResult CaptureImage(ezQtDocumentWindow* pWindow, const char* szImageName);

  void CloseCurrentProject();
  void SafeProfilingData();
  void ProcessEvents(ezUInt32 uiIterations = 1);

  std::unique_ptr<QMimeData> AssetsToDragMimeData(ezArrayPtr<ezUuid> assetGuids);
  std::unique_ptr<QMimeData> ObjectsDragMimeData(const ezDeque<const ezDocumentObject*>& objects);
  void MoveObjectsToLayer(ezScene2Document* pDoc, const ezDeque<const ezDocumentObject*>& objects, const ezUuid& layer, ezDeque<const ezDocumentObject*>& new_objects);
  const ezDocumentObject* DropAsset(ezScene2Document* pDoc, const char* szAssetGuidOrPath, bool bShift = false, bool bCtrl = false);
  const ezDocumentObject* CreateGameObject(ezScene2Document* pDoc, const ezDocumentObject* pParent = nullptr, ezStringView sName = {});


  ezEditorTestApplication* m_pApplication = nullptr;
  ezString m_sProjectPath;
  ezImage m_CapturedImage;
  ezDynamicArray<ezString> m_CommandLineArguments;
  ezDynamicArray<const char*> m_CommandLineArgumentPointers;
};
