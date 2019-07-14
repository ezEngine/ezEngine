#include <EditorTestPCH.h>

#include "TestClass.h"
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

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

  ezQtEditorApp::GetSingleton()->StartupEditor(ezQtEditorApp::StartupFlags::SafeMode | ezQtEditorApp::StartupFlags::NoRecent, userDataDir);
  // Disable msg boxes.
  ezQtUiServices::SetHeadless(true);
  ezFileSystem::SetSpecialDirectory("testout", ezTestFramework::GetInstance()->GetAbsOutputPath());
}

void ezEditorTestApplication::BeforeHighLevelSystemsShutdown()
{
  EZ_PROFILE_SCOPE("BeforeHighLevelSystemsShutdown");
  ezQtEditorApp::GetSingleton()->ShutdownEditor();
}

//////////////////////////////////////////////////////////////////////////

ezEditorTest::ezEditorTest() = default;
ezEditorTest::~ezEditorTest() = default;

ezEditorTestApplication* ezEditorTest::CreateApplication()
{
  return EZ_DEFAULT_NEW(ezEditorTestApplication);
}

ezResult ezEditorTest::InitializeTest()
{
  m_pApplication = CreateApplication();
  m_sProjectPath.Clear();

  if (m_pApplication == nullptr)
    return EZ_FAILURE;

  ezRun_Startup(m_pApplication);

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
