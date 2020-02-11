#include <EditorTestPCH.h>

#include "TestClass.h"
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <RendererFoundation/Device/Device.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <RendererDX11/Device/DeviceDX11.h>
#endif

ezEditorTestApplication::ezEditorTestApplication()
  : ezApplication("ezEditor")
{
  EnableMemoryLeakReporting(true);

  m_pEditorApp = new ezQtEditorApp;
}

ezResult ezEditorTestApplication::BeforeCoreSystemsStartup()
{
  if (SUPER::BeforeCoreSystemsStartup().Failed())
    return EZ_FAILURE;

  ezStartup::AddApplicationTag("tool");
  ezStartup::AddApplicationTag("editor");
  ezStartup::AddApplicationTag("editorapp");

  ezQtEditorApp::GetSingleton()->InitQt(GetArgumentCount(), (char**)GetArgumentsArray());
  return EZ_SUCCESS;
}

void ezEditorTestApplication::AfterCoreSystemsShutdown()
{
  ezQtEditorApp::GetSingleton()->DeInitQt();

  delete m_pEditorApp;
  m_pEditorApp = nullptr;
}

ezApplication::ApplicationExecution ezEditorTestApplication::Run()
{
  qApp->processEvents();
  return ezApplication::Continue;
}

void ezEditorTestApplication::AfterCoreSystemsStartup()
{
  EZ_PROFILE_SCOPE("AfterCoreSystemsStartup");
  // We override the user data dir to not pollute the editor settings.
  ezStringBuilder userDataDir = ezOSFile::GetUserDataFolder();
  userDataDir.AppendPath("ezEngine Project", "EditorTest");
  userDataDir.MakeCleanPath();

  ezQtEditorApp::GetSingleton()->StartupEditor(ezQtEditorApp::StartupFlags::SafeMode | ezQtEditorApp::StartupFlags::NoRecent | ezQtEditorApp::StartupFlags::UnitTest, userDataDir);
  // Disable msg boxes.
  ezQtUiServices::SetHeadless(true);
  ezFileSystem::SetSpecialDirectory("testout", ezTestFramework::GetInstance()->GetAbsOutputPath());

  ezFileSystem::AddDataDirectory(">eztest/", "ImageComparisonDataDir", "imgout", ezFileSystem::AllowWrites);
}

void ezEditorTestApplication::BeforeHighLevelSystemsShutdown()
{
  EZ_PROFILE_SCOPE("BeforeHighLevelSystemsShutdown");
  ezQtEditorApp::GetSingleton()->ShutdownEditor();
}

//////////////////////////////////////////////////////////////////////////

ezEditorTest::ezEditorTest()
{
  ezQtEngineViewWidget::s_FixedResolution = ezSizeU32(512, 512);
}

ezEditorTest::~ezEditorTest() = default;

ezEditorTestApplication* ezEditorTest::CreateApplication()
{
  return EZ_DEFAULT_NEW(ezEditorTestApplication);
}

ezResult ezEditorTest::GetImage(ezImage& img)
{
  if (!m_CapturedImage.IsValid())
    return EZ_FAILURE;

  img.ResetAndMove(std::move(m_CapturedImage));
  return EZ_SUCCESS;
}

ezResult ezEditorTest::InitializeTest()
{
  m_pApplication = CreateApplication();
  m_sProjectPath.Clear();

  if (m_pApplication == nullptr)
    return EZ_FAILURE;

  ezRun_Startup(m_pApplication);

  static bool s_bCheckedReferenceDriver = false;
  static bool s_bIsReferenceDriver = false;

  if (!s_bCheckedReferenceDriver)
  {
    s_bCheckedReferenceDriver = true;

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    ezUniquePtr<ezGALDevice> pDevice;
    ezGALDeviceCreationDescription DeviceInit;
    DeviceInit.m_bCreatePrimarySwapChain = false;

    pDevice = EZ_DEFAULT_NEW(ezGALDeviceDX11, DeviceInit);

    pDevice->Init();

    if (pDevice->GetCapabilities().m_sAdapterName == "Microsoft Basic Render Driver")
    {
      s_bIsReferenceDriver = true;
    }

    pDevice->Shutdown();
    pDevice.Clear();
#endif
  }

  if (s_bIsReferenceDriver)
  {
    // Use different images for comparison when running the D3D11 Reference Device
    ezTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_D3D11Ref");
  }
  else
  {
    ezTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("");
  }

  return EZ_SUCCESS;
}

ezResult ezEditorTest::DeInitializeTest()
{
  CloseCurrentProject();

  if (m_pApplication)
  {
    ezRun_Shutdown(m_pApplication);

    EZ_DEFAULT_DELETE(m_pApplication);
  }


  return EZ_SUCCESS;
}

ezResult ezEditorTest::CreateAndLoadProject(const char* name)
{
  EZ_PROFILE_SCOPE("CreateAndLoadProject");
  ezStringBuilder relPath;
  relPath = ":APPDATA";
  relPath.AppendPath(name);

  ezStringBuilder absPath;
  if (ezFileSystem::ResolvePath(relPath, &absPath, nullptr).Failed())
  {
    ezLog::Error("Failed to resolve project path '{0}'.", relPath);
    return EZ_FAILURE;
  }
  if (ezOSFile::DeleteFolder(absPath).Failed())
  {
    ezLog::Error("Failed to delete old project folder '{0}'.", absPath);
    return EZ_FAILURE;
  }

  ezStringBuilder projectFile = absPath;
  projectFile.AppendPath("ezProject");
  if (m_pApplication->m_pEditorApp->CreateOrOpenProject(true, projectFile).Failed())
  {
    ezLog::Error("Failed to create project '{0}'.", projectFile);
    return EZ_FAILURE;
  }

  m_sProjectPath = absPath;
  return EZ_SUCCESS;
}

ezResult ezEditorTest::OpenProject(const char* path)
{
  EZ_PROFILE_SCOPE("OpenProject");
  ezStringBuilder relPath;
  relPath = ">sdk";
  relPath.AppendPath(path);

  ezStringBuilder absPath;
  if (ezFileSystem::ResolveSpecialDirectory(relPath, absPath).Failed())
  {
    ezLog::Error("Failed to resolve project path '{0}'.", relPath);
    return EZ_FAILURE;
  }

  ezStringBuilder projectFile = absPath;
  projectFile.AppendPath("ezProject");
  if (m_pApplication->m_pEditorApp->CreateOrOpenProject(false, projectFile).Failed())
  {
    ezLog::Error("Failed to open project '{0}'.", projectFile);
    return EZ_FAILURE;
  }

  m_sProjectPath = absPath;
  return EZ_SUCCESS;
}

ezDocument* ezEditorTest::OpenDocument(const char* subpath)
{
  ezStringBuilder fullpath;
  fullpath = m_sProjectPath;
  fullpath.AppendPath(subpath);

  ezDocument* pDoc = m_pApplication->m_pEditorApp->OpenDocument(fullpath, ezDocumentFlags::RequestWindow);

  if (pDoc)
  {
    ProcessEvents();
  }

  return pDoc;
}

void ezEditorTest::ExecuteDocumentAction(const char* szActionName, ezDocument* pDocument, const ezVariant& argument /*= ezVariant()*/)
{
  EZ_TEST_BOOL(ezActionManager::ExecuteAction(nullptr, szActionName, pDocument, argument).Succeeded());
}

ezResult ezEditorTest::CaptureImage(ezQtDocumentWindow* pWindow, const char* szImageName)
{
  ezStringBuilder sImgPath;
  // TODO: fix this
  sImgPath.Format("D:/{}.tga", szImageName);

  ezOSFile::DeleteFile(sImgPath);

  pWindow->CreateImageCapture(sImgPath);

  // TODO: fix this
  for (int i = 0; i < 10; ++i)
  {
    ezThreadUtils::Sleep(ezTime::Milliseconds(100));
    ProcessEvents();
  }

  if (!ezOSFile::ExistsFile(sImgPath))
    return EZ_FAILURE;

  EZ_SUCCEED_OR_RETURN(m_CapturedImage.LoadFrom(sImgPath));

  return EZ_SUCCESS;
}

void ezEditorTest::CloseCurrentProject()
{
  EZ_PROFILE_SCOPE("CloseCurrentProject");
  m_sProjectPath.Clear();
  m_pApplication->m_pEditorApp->CloseProject();
}

void ezEditorTest::SafeProfilingData()
{
  ezFileWriter fileWriter;
  if (fileWriter.Open(":appdata/profiling.json") == EZ_SUCCESS)
  {
    ezProfilingSystem::ProfilingData profilingData = ezProfilingSystem::Capture();
    profilingData.Write(fileWriter);
  }
}

void ezEditorTest::ProcessEvents(ezUInt32 uiIterations)
{
  EZ_PROFILE_SCOPE("ProcessEvents");
  if (qApp)
  {
    for (ezUInt32 i = 0; i < uiIterations; i++)
    {
      qApp->processEvents();
    }
  }
}
