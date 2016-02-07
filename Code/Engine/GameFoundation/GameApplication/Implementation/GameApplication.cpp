
#include <GameFoundation/PCH.h>
#include <GameFoundation/GameApplication/GameApplication.h>
#include <GameFoundation/WorldModule/WorldModule.h>

namespace
{
  ezProfilingId g_BeforeWorldUpdateProfilingId = ezProfilingSystem::CreateId("GameApplication.BeforeWorldUpdate");
  ezProfilingId g_AfterWorldUpdateProfilingId = ezProfilingSystem::CreateId("GameApplication.AfterWorldUpdate");
  ezProfilingId g_PresentProfilingId = ezProfilingSystem::CreateId("GameApplication.Present");
}

ezGameApplication* ezGameApplication::s_pGameApplicationInstance = nullptr;


ezGameApplication::ezGameApplication(ezGameApplicationType type, const char* szProjectPath /*= nullptr*/)
  : m_UpdateTask("GameApplication.Update", ezMakeDelegate(&ezGameApplication::UpdateWorldsAndExtractViews, this))
  , m_sAppProjectPath(szProjectPath)
{
  s_pGameApplicationInstance = this;
  m_bWasQuitRequested = false;
  m_AppType = type;
}

ezGameApplication::~ezGameApplication()
{
  DestroyAllGameStates();

  s_pGameApplicationInstance = nullptr;
}

ezGALSwapChainHandle ezGameApplication::AddWindow(ezWindowBase* pWindow)
{
  WindowContext& windowContext = m_Windows.ExpandAndGetRef();
  windowContext.m_pWindow = pWindow;

  ezGALSwapChainCreationDescription desc;
  desc.m_pWindow = pWindow;
  desc.m_bAllowScreenshots = true;

  windowContext.m_hSwapChain = ezGALDevice::GetDefaultDevice()->CreateSwapChain(desc);
  windowContext.m_bFirstFrame = true;
  return windowContext.m_hSwapChain;
}

void ezGameApplication::RemoveWindow(ezWindowBase* pWindow)
{
  for (ezUInt32 i = 0; i < m_Windows.GetCount(); ++i)
  {
    WindowContext& windowContext = m_Windows[i];
    if (windowContext.m_pWindow == pWindow)
    {
      ezGALDevice::GetDefaultDevice()->DestroySwapChain(windowContext.m_hSwapChain);
      m_Windows.RemoveAt(i);
      break;
    }
  }
}

ezGALSwapChainHandle ezGameApplication::GetSwapChain(const ezWindowBase* pWindow) const
{
  for (auto& windowContext : m_Windows)
  {
    if (windowContext.m_pWindow == pWindow)
    {
      return windowContext.m_hSwapChain;
    }
  }

  return ezGALSwapChainHandle();
}

void ezGameApplication::CreateGameStateForWorld(ezWorld* pWorld)
{
  EZ_LOG_BLOCK("Create Game State");

  ezGameState* pCurState = nullptr;
  float fBestPriority = -1.0f;

  for (auto pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    if (!pRtti->IsDerivedFrom<ezGameState>())
      continue;

    if (!pRtti->GetAllocator()->CanAllocate())
      continue;

    ezGameState* pState = static_cast<ezGameState*>(pRtti->GetAllocator()->Allocate());

    EZ_ASSERT_DEV(pState != nullptr, "Failed to allocate ezGameState object");

    pState->m_pApplication = this;

    float fPriority = pState->CanHandleThis(m_AppType, pWorld);

    if (fPriority < 0.0f)
    {
      pState->GetDynamicRTTI()->GetAllocator()->Deallocate(pState);
      continue;
    }

    if (fPriority > fBestPriority)
    {
      fBestPriority = fPriority;

      if (pCurState)
      {
        pCurState->GetDynamicRTTI()->GetAllocator()->Deallocate(pCurState);
      }

      pCurState = pState;
    }
  }

  GameStateData& gsd = m_GameStates.ExpandAndGetRef();
  gsd.m_pState = pCurState;
  gsd.m_pLinkedToWorld = pWorld;
}


void ezGameApplication::DestroyGameState(ezUInt32 id)
{
  if (m_GameStates[id].m_pState != nullptr)
  {
    m_GameStates[id].m_pState->GetDynamicRTTI()->GetAllocator()->Deallocate(m_GameStates[id].m_pState);
    m_GameStates[id].m_pState = nullptr;
    m_GameStates[id].m_pLinkedToWorld = nullptr;
  }
}


void ezGameApplication::DestroyGameStateForWorld(ezWorld* pWorld)
{
  for (ezUInt32 id = 0; id < m_GameStates.GetCount(); ++id)
  {
    if (m_GameStates[id].m_pLinkedToWorld == pWorld)
    {
      DestroyGameState(id);
    }
  }
}


ezGameState* ezGameApplication::GetGameStateForWorld(ezWorld* pWorld) const
{
  for (ezUInt32 id = 0; id < m_GameStates.GetCount(); ++id)
  {
    if (m_GameStates[id].m_pLinkedToWorld == pWorld)
    {
      return m_GameStates[id].m_pState;
    }
  }

  return nullptr;
}

void ezGameApplication::DestroyAllGameStates()
{
  for (ezUInt32 i = 0; i < m_GameStates.GetCount(); ++i)
  {
    DestroyGameState(i);
  }
}


void ezGameApplication::ActivateGameStateForWorld(ezWorld* pWorld)
{
  for (ezUInt32 id = 0; id < m_GameStates.GetCount(); ++id)
  {
    if (m_GameStates[id].m_pLinkedToWorld == pWorld)
    {
      /// \todo Already activated?
      m_GameStates[id].m_pState->OnActivation(m_AppType, pWorld);
    }
  }
}


void ezGameApplication::DeactivateGameStateForWorld(ezWorld* pWorld)
{
  for (ezUInt32 id = 0; id < m_GameStates.GetCount(); ++id)
  {
    if (m_GameStates[id].m_pLinkedToWorld == pWorld)
    {
      /// \todo Already deactivated?
      m_GameStates[id].m_pState->OnDeactivation();
    }
  }
}

void ezGameApplication::ActivateAllGameStates()
{
  for (ezUInt32 i = 0; i < m_GameStates.GetCount(); ++i)
  {
      /// \todo Already deactivated?
      m_GameStates[i].m_pState->OnActivation(m_AppType, m_GameStates[i].m_pLinkedToWorld);
  }
}


void ezGameApplication::DeactivateAllGameStates()
{
  for (ezUInt32 i = 0; i < m_GameStates.GetCount(); ++i)
  {
      /// \todo Already deactivated?
      m_GameStates[i].m_pState->OnDeactivation();
  }
}

void ezGameApplication::RequestQuit()
{
  m_bWasQuitRequested = true;
}


ezString ezGameApplication::FindProjectDirectoryForScene(const char* szScene) const
{
  ezStringBuilder sPath = szScene;
  sPath.PathParentDirectory();

  ezStringBuilder sTemp;

  while (!sPath.IsEmpty())
  {
    sTemp = sPath;
    sTemp.AppendPath("ezProject");

    if (ezOSFile::ExistsFile(sTemp))
      return sPath;

    sPath.PathParentDirectory();
  }

  return "";
}

ezWorld* ezGameApplication::CreateWorld(const char* szWorldName, bool bCreateWorldModules)
{
  auto& wd = m_Worlds.ExpandAndGetRef();
  wd.m_pWorld = EZ_DEFAULT_NEW(ezWorld, szWorldName);
  wd.m_pWorld->GetClock().SetTimeStepSmoothing(&wd.m_TimeStepSmoothing);

  if (bCreateWorldModules)
  {
    wd.CreateWorldModules();

    // scene modules will most likely register component managers, so just mark it for write once here
    EZ_LOCK(wd.m_pWorld->GetWriteMarker());

    for (auto pModule : wd.m_WorldModules)
    {
      pModule->Startup(wd.m_pWorld);
    }
  }

  return wd.m_pWorld;
}


void ezGameApplication::DestroyWorld(ezWorld* pWorld)
{
  if (pWorld == nullptr)
    return;

  WorldData* wd = nullptr;

  for (ezUInt32 i = 0; i < m_Worlds.GetCount(); ++i)
  {
    if (m_Worlds[i].m_pWorld == pWorld)
    {
      wd = &m_Worlds[i];
      break;
    }
  }

  if (wd == nullptr)
    return;

  {
    // scene modules will most likely register component managers, so just mark it for write once here
    EZ_LOCK(pWorld->GetWriteMarker());

    for (auto pModule : wd->m_WorldModules)
    {
      pModule->Shutdown();
    }
  }

  wd->DestroyWorldModules();
  wd->m_pWorld = nullptr;

  EZ_DEFAULT_DELETE(pWorld);
}

void ezGameApplication::AfterCoreStartup()
{
  DoProjectSetup();

  ezStartup::StartupEngine();

  if (m_AppType == ezGameApplicationType::StandAlone)
  {
    CreateGameStateForWorld(nullptr);

    /// \todo Check that any state was created?
    /// \todo Fallback state?

    ActivateAllGameStates();
  }
  else
  {
    /// \todo What now?
  }
}

void ezGameApplication::BeforeCoreShutdown()
{
  DeactivateAllGameStates();

  DestroyAllGameStates();

  ezResourceManager::FreeUnusedResources(true);

  ezStartup::ShutdownEngine();

  ezFrameAllocator::Reset();
  ezResourceManager::FreeUnusedResources(true);

  DoShutdownGraphicsDevice();

  ezResourceManager::FreeUnusedResources(true);

  DoUnloadPlugins();

  ezTelemetry::CloseConnection();

  DoShutdownLogWriters();
}


ezString ezGameApplication::FindProjectDirectory(const char* szStartDirectory, const char* szRelPathToProjectFile) const
{
  ezStringBuilder sStartDir = szStartDirectory;
  ezStringBuilder sPath;

  while (true)
  {
    sPath = sStartDir;
    sPath.AppendPath(szRelPathToProjectFile);
    sPath.MakeCleanPath();

    if (!sPath.EndsWith_NoCase("/ezProject"))
      sPath.AppendPath("ezProject");

    if (ezOSFile::ExistsFile(sPath))
    {
      sPath.PathParentDirectory();
      return sPath;
    }

    if (sStartDir.IsEmpty())
      break;

    sStartDir.PathParentDirectory();
  }

  return ezString();
}

ezString ezGameApplication::FindProjectDirectory() const
{
  EZ_ASSERT_RELEASE(!m_sAppProjectPath.IsEmpty(), "Either the project must have a built in project directory passed to the ezGameApplication constructor, or m_sAppProjectPath must be set manually before doing project setup, or ezGameApplication::FindProjectDirectory() must be overridden.");

  return FindProjectDirectory(ezOSFile::GetApplicationDirectory(), m_sAppProjectPath);
}

ezApplication::ApplicationExecution ezGameApplication::Run()
{
  if (!m_bWasQuitRequested)
  {

    for (ezUInt32 i = 0; i < m_Windows.GetCount(); ++i)
    {
      m_Windows[i].m_pWindow->ProcessWindowMessages();
    }

    ezClock::GetGlobalClock()->Update();

    UpdateInput();

    UpdateWorldsAndRender();
  }

  return m_bWasQuitRequested ? ezApplication::Quit : ezApplication::Continue;
}

void ezGameApplication::UpdateWorldsAndRender()
{
  ezRenderLoop::BeginFrame();

  ezTaskGroupID updateTaskID;
  if (ezRenderLoop::GetUseMultithreadedRendering())
  {
    updateTaskID = ezTaskSystem::StartSingleTask(&m_UpdateTask, ezTaskPriority::EarlyThisFrame);
  }
  else
  {
    UpdateWorldsAndExtractViews();
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  {
    pDevice->BeginFrame();

    ezRenderLoop::Render(ezRenderContext::GetDefaultInstance());

    if (ezRenderLoop::GetUseMultithreadedRendering())
    {
      ezTaskSystem::WaitForGroup(updateTaskID);
    }

    ezTelemetry::PerFrameUpdate();
    ezResourceManager::PerFrameUpdate();
    ezTaskSystem::FinishFrameTasks();

    {
      EZ_PROFILE(g_PresentProfilingId);
      for (auto& windowContext : m_Windows)
      {
        if (ezRenderLoop::GetUseMultithreadedRendering() && windowContext.m_bFirstFrame)
        {
          windowContext.m_bFirstFrame = false;
        }
        else
        {
          pDevice->Present(windowContext.m_hSwapChain);
        }
      }
    }

    pDevice->EndFrame();
  }

  ezRenderLoop::FinishFrame();
  ezFrameAllocator::Swap();
}

void ezGameApplication::UpdateWorldsAndExtractViews()
{
  {
    EZ_PROFILE(g_BeforeWorldUpdateProfilingId);

    for (ezUInt32 i = 0; i < m_GameStates.GetCount(); ++i)
    {
      /// \todo Pause state ?
      if (m_GameStates[i].m_pState)
      {
        m_GameStates[i].m_pState->BeforeWorldUpdate();
      }
    }
  }

  static ezHybridArray<ezWorld*, 16> worldsToUpdate;
  worldsToUpdate.Clear();

  auto views = ezRenderLoop::GetMainViews();
  for (ezUInt32 i = 0; i < views.GetCount(); ++i)
  {
    ezWorld* pWorld = views[i]->GetWorld();
    if (!worldsToUpdate.Contains(pWorld))
      worldsToUpdate.PushBack(pWorld);
  }

  for (auto pWorld : worldsToUpdate)
  {
    UpdateWorldModules(pWorld);
  }

  if (ezRenderLoop::GetUseMultithreadedRendering())
  {
    ezTaskGroupID updateWorldsTaskID = ezTaskSystem::CreateTaskGroup(ezTaskPriority::EarlyThisFrame);
    for (ezUInt32 i = 0; i < worldsToUpdate.GetCount(); ++i)
    {
      ezTaskSystem::AddTaskToGroup(updateWorldsTaskID, worldsToUpdate[i]->GetUpdateTask());
    }
    ezTaskSystem::StartTaskGroup(updateWorldsTaskID);
    ezTaskSystem::WaitForGroup(updateWorldsTaskID);
  }
  else
  {
    for (ezUInt32 i = 0; i < worldsToUpdate.GetCount(); ++i)
    {
      ezWorld* pWorld = worldsToUpdate[i];
      EZ_LOCK(pWorld->GetWriteMarker());

      pWorld->Update();
    }
  }

  {
    EZ_PROFILE(g_AfterWorldUpdateProfilingId);

    for (ezUInt32 i = 0; i < m_GameStates.GetCount(); ++i)
    {
      /// \todo Pause state ?
      if (m_GameStates[i].m_pState)
      {
        m_GameStates[i].m_pState->AfterWorldUpdate();
      }
    }
  }

  ezRenderLoop::ExtractMainViews();
}

void ezGameApplication::DoUnloadPlugins()
{
  ezStringBuilder s;
  while (ezPlugin::GetFirstInstance() != nullptr)
  {
    s = ezPlugin::GetFirstInstance()->GetPluginName();
    EZ_VERIFY(ezPlugin::UnloadPlugin(s).Succeeded(), "Failed to unload plugin '%s'", s.GetData());
  }
}

EZ_STATICLINK_FILE(GameFoundation, GameFoundation_GameApplication);

