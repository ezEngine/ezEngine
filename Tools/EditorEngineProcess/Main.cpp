#include <PCH.h>
#include <EditorEngineProcess/GameState.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Time/Clock.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>

EZ_APPLICATION_ENTRY_POINT(ezGameApplication, *EZ_DEFAULT_NEW(ezEngineProcessGameState));

ezEngineProcessGameState* ezEngineProcessGameState::s_pInstance = nullptr;

ezEngineProcessGameState::ezEngineProcessGameState()
{
  EZ_ASSERT_DEV(s_pInstance == nullptr, "ezEngineProcessGameState must be a singleton");

  m_pApp = nullptr;
  s_pInstance = this;
  m_uiNextComponentPickingID = 1;
}

void ezEngineProcessGameState::Activate()
{
  if (ezCommandLineUtils::GetInstance()->GetBoolOption("-debug"))
  {
    while (!IsDebuggerPresent())
    {
      ezThreadUtils::Sleep(10);
    }
  }

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

  m_IPC.m_Events.AddEventHandler(ezMakeDelegate(&ezEngineProcessGameState::EventHandlerIPC, this));

  SendReflectionInformation();

  m_IPC.WaitForMessage(ezGetStaticRTTI<ezSetupProjectMsgToEditor>());
}

void ezEngineProcessGameState::Deactivate()
{
  m_IPC.m_Events.RemoveEventHandler(ezMakeDelegate(&ezEngineProcessGameState::EventHandlerIPC, this));

  delete m_pApp;
}

void ezEngineProcessGameState::BeforeWorldUpdate()
{
  ezRenderLoop::ClearMainViews();

  m_IPC.ProcessMessages();
  
  if (!m_IPC.IsHostAlive())
    GetApplication()->RequestQuit();

  ezEditorEngineSyncObject* pSyncObject = ezEditorEngineSyncObject::GetFirstInstance();

  while (pSyncObject)
  {
    if (pSyncObject->GetDynamicRTTI()->IsDerivedFrom<ezEditorGizmoHandle>())
    {
      ezEditorGizmoHandle* pGizmoHandle = static_cast<ezEditorGizmoHandle*>(pSyncObject);

      if (pSyncObject->GetDocumentGuid().IsValid())
      {
        ezEngineProcessDocumentContext* pContext = ezEngineProcessDocumentContext::GetDocumentContext(pSyncObject->GetDocumentGuid());

        if (pContext)
        {
          EZ_LOCK(pContext->m_pWorld->GetWriteMarker());

          if (pGizmoHandle->SetupForEngine(pContext->m_pWorld, m_uiNextComponentPickingID))
          {
            m_OtherPickingMap.RegisterObject(pGizmoHandle->GetGuid(), m_uiNextComponentPickingID);
            ++m_uiNextComponentPickingID;
          }

          pGizmoHandle->UpdateForEngine(pContext->m_pWorld);
        }
      }
    }


    pSyncObject = pSyncObject->GetNextInstance();
  }

  ezThreadUtils::Sleep(1);
}



