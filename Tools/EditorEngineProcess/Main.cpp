#include <PCH.h>
#include <EditorEngineProcess/GameState.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Time/Clock.h>
#include <RendererCore/RenderLoop/RenderLoop.h>

EZ_APPLICATION_ENTRY_POINT(ezGameApplication, *EZ_DEFAULT_NEW(ezEditorGameState));

ezEditorGameState::ezEditorGameState()
{
  m_pApp = nullptr;
}

void ezEditorGameState::Activate()
{
  //while (!IsDebuggerPresent());

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  // Setting this flags prevents Windows from showing a dialog when the Engine process crashes
  // this also speeds up process termination significantly (down to less than a second)
  DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
  SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
#endif

  {
    ezStringBuilder sAppDir = ezOSFile::GetApplicationDirectory();
    sAppDir.AppendPath("../../../Shared/Tools/EditorEngineProcess");

    ezOSFile osf;
    osf.CreateDirectoryStructure(sAppDir);

    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
    ezFileSystem::AddDataDirectory("", ezFileSystem::AllowWrites, "App"); // for absolute paths
    ezFileSystem::AddDataDirectory(sAppDir.GetData(), ezFileSystem::AllowWrites, "App"); // for everything relative
  }

  ezPlugin::LoadPlugin("ezEnginePluginTest");

  int argc = GetApplication()->GetArgumentCount();
  const char** argv = GetApplication()->GetArgumentsArray();
  m_pApp = new QApplication(argc, (char**) argv);

  EZ_VERIFY(m_IPC.ConnectToHostProcess().Succeeded(), "Could not connect to host");

  m_IPC.m_Events.AddEventHandler(ezMakeDelegate(&ezEditorGameState::EventHandlerIPC, this));

  SendReflectionInformation();

  SendProjectReadyMessage();

  ezApplicationConfig::SetProjectDirectory("F:/ezEngine/Trunk/Samples/Test");
}

void ezEditorGameState::Deactivate()
{
  m_IPC.m_Events.RemoveEventHandler(ezMakeDelegate(&ezEditorGameState::EventHandlerIPC, this));

  delete m_pApp;
}

void ezEditorGameState::BeforeWorldUpdate()
{
  ezRenderLoop::ClearMainViews();

  m_IPC.ProcessMessages();
  
  if (!m_IPC.IsHostAlive())
    GetApplication()->RequestQuit();

  ezThreadUtils::Sleep(1);
}



