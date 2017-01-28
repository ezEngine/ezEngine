
#include <GameEngine/PCH.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Time/DefaultTimeStepSmoothing.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <GameEngine/Console/Console.h>

ezGameApplication* ezGameApplication::s_pGameApplicationInstance = nullptr;


ezGameApplication::ezGameApplication(const char* szAppName, ezGameApplicationType type, const char* szProjectPath /*= nullptr*/)
  : m_UpdateTask("GameApplication.Update", ezMakeDelegate(&ezGameApplication::UpdateWorldsAndExtractViews, this))
  , m_sAppProjectPath(szProjectPath)
{
  m_sAppName = szAppName;
  s_pGameApplicationInstance = this;
  m_bWasQuitRequested = false;
  m_bShowFps = false;
  m_AppType = type;

  m_pConsole = EZ_DEFAULT_NEW(ezConsole);
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
  desc.m_BackBufferFormat = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
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


void ezGameApplication::ReinitializeInputConfig()
{
  DoConfigureInput(true);
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
      m_GameStates[id].m_pState->OnActivation(pWorld);
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
  // There is always at least one gamestate, but if it is null, then our applicatin might not use them at all.
  if (m_GameStates.IsEmpty() || m_GameStates[0].m_pState == nullptr)
    return;

  for (ezUInt32 i = 0; i < m_GameStates.GetCount(); ++i)
  {
    /// \todo Already deactivated
    m_GameStates[i].m_pState->OnActivation(m_GameStates[i].m_pLinkedToWorld);
  }
}


void ezGameApplication::DeactivateAllGameStates()
{
  // There is always at least one gamestate, but if it is null, then our applicatin might not use them at all.
  if (m_GameStates.IsEmpty() || m_GameStates[0].m_pState == nullptr)
    return;

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

ezWorld* ezGameApplication::CreateWorld(const char* szWorldName)
{
  auto& wd = m_Worlds.ExpandAndGetRef();
  wd.m_pTimeStepSmoothing = EZ_DEFAULT_NEW(ezDefaultTimeStepSmoothing);
  wd.m_pWorld = EZ_DEFAULT_NEW(ezWorld, szWorldName);
  wd.m_pWorld->GetClock().SetTimeStepSmoothing(wd.m_pTimeStepSmoothing);

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

  wd->m_pWorld->GetClock().SetTimeStepSmoothing(nullptr);
  wd->m_pWorld = nullptr;
  EZ_DEFAULT_DELETE(wd->m_pTimeStepSmoothing);
  wd->m_pTimeStepSmoothing = nullptr;

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

    ActivateAllGameStates();
  }
  else
  {
    // Special case: Must be handled by custom implementations of ezGameApplication
  }
}

void ezGameApplication::BeforeCoreShutdown()
{
  // make sure that no textures are continue to be streamed in while the engine shuts down
  ezResourceManager::EngineAboutToShutdown();

  DeactivateAllGameStates();

  DestroyAllGameStates();

  ezResourceManager::ClearAllResourceFallbacks();

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


ezString ezGameApplication::SearchProjectDirectory(const char* szStartDirectory, const char* szRelPathToProjectFile) const
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

  if (ezPathUtils::IsAbsolutePath(m_sAppProjectPath))
    return m_sAppProjectPath;

  return SearchProjectDirectory(ezOSFile::GetApplicationDirectory(), m_sAppProjectPath);
}

ezApplication::ApplicationExecution ezGameApplication::Run()
{
  if (!m_bWasQuitRequested)
  {
    ProcessWindowMessages();

    if (ezRenderLoop::GetMainViews().GetCount() > 0)
    {
      ezClock::GetGlobalClock()->Update();

      UpdateInput();

      UpdateWorldsAndRender();
    }
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

    RenderFps();
    RenderConsole();

    ezRenderLoop::Render(ezRenderContext::GetDefaultInstance());

    if (ezRenderLoop::GetUseMultithreadedRendering())
    {
      EZ_PROFILE("Wait for UpdateWorldsAndExtractViews");
      ezTaskSystem::WaitForGroup(updateTaskID);
    }

    ezTelemetry::PerFrameUpdate();
    ezResourceManager::PerFrameUpdate();
    ezTaskSystem::FinishFrameTasks();

    {
      EZ_PROFILE("GameApplication.Present");
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

  ezRenderLoop::EndFrame();
  ezFrameAllocator::Swap();
}

void ezGameApplication::UpdateWorldsAndExtractViews()
{
  {
    EZ_PROFILE("GameApplication.BeforeWorldUpdate");

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
    {
      worldsToUpdate.PushBack(pWorld);
    }
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
    EZ_PROFILE("GameApplication.AfterWorldUpdate");

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
  ezSet<ezString> ToUnload;

  // if a plugin is linked statically (which happens mostly in an editor context)
  // then it cannot be unloaded and the ezPlugin instance won't ever go away
  // however, ezPlugin::UnloadPlugin will always return that it is already unloaded, so we can just skip it there
  // all other plugins must be unloaded as often as their refcount, though
  ezStringBuilder s;
  ezPlugin* pPlugin = ezPlugin::GetFirstInstance();
  while (pPlugin != nullptr)
  {
    s = ezPlugin::GetFirstInstance()->GetPluginName();
    ToUnload.Insert(s);

    pPlugin = pPlugin->GetNextInstance();
  }

  ezString temp;
  while (!ToUnload.IsEmpty())
  {
    auto it = ToUnload.GetIterator();

    ezInt32 iRefCount = 0;
    EZ_VERIFY(ezPlugin::UnloadPlugin(it.Key(), &iRefCount).Succeeded(), "Failed to unload plugin '{0}'", s);

    if (iRefCount == 0)
      ToUnload.Remove(it);
  }
}

void ezGameApplication::RenderFps()
{
  static float fAccumTime = 0.5f;
  static float fElapsedTime = 0.0f;
  fAccumTime += (float)ezClock::GetGlobalClock()->GetTimeDiff().GetSeconds();

  if (fAccumTime >= 0.5f)
  {
    fAccumTime -= 0.5f;

    fElapsedTime = (float)ezClock::GetGlobalClock()->GetTimeDiff().GetSeconds();
  }

  if (m_bShowFps)
  {
    if (const ezView* pView = ezRenderLoop::GetViewByUsageHint(ezCameraUsageHint::MainView))
    {
      ezStringBuilder sFps;
      sFps.Format("{0} fps, {1} ms", ezArgF(1.0f / fElapsedTime, 4, false, 4), ezArgF(fElapsedTime * 1000.0f, 4, false, 4));

      ezInt32 viewHeight = (ezInt32)(pView->GetViewport().height);

      ezDebugRenderer::DrawText(pView, sFps, ezVec2I32(10, viewHeight - 10 - 16), ezColor::White);
    }
  }
}

void ezGameApplication::RenderConsole()
{
  if (!m_bShowConsole || !m_pConsole)
    return;

  const ezView* pView = ezRenderLoop::GetViewByUsageHint(ezCameraUsageHint::MainView);
  if (pView == nullptr)
    return;

  const float fViewWidth = pView->GetViewport().width;
  const float fViewHeight = pView->GetViewport().height;
  const float fTextHeight = 20.0f;
  const float fConsoleHeight = (fViewHeight / 2.0f);
  const float fBorderWidth = 3.0f;
  const float fConsoleTextAreaHeight = fConsoleHeight - fTextHeight - (2.0f * fBorderWidth);

  const ezInt32 iTextHeight = (ezInt32)fTextHeight;
  const ezInt32 iTextLeft = (ezInt32)(fBorderWidth);

  {
    ezColor backgroundColor(0.3f, 0.3f, 0.3f, 0.7f);
    ezDebugRenderer::Draw2DRectangle(pView, ezRectFloat(0.0f, 0.0f, fViewWidth, fConsoleHeight), 0.0f, backgroundColor);

    ezColor foregroundColor(0.0f, 0.0f, 0.0f, 0.8f);
    ezDebugRenderer::Draw2DRectangle(pView, ezRectFloat(fBorderWidth, 0.0f, fViewWidth - (2.0f * fBorderWidth), fConsoleTextAreaHeight), 0.0f, foregroundColor);
    ezDebugRenderer::Draw2DRectangle(pView, ezRectFloat(fBorderWidth, fConsoleTextAreaHeight + fBorderWidth, fViewWidth - (2.0f * fBorderWidth), fTextHeight), 0.0f, foregroundColor);
  }

  {
    auto& consoleStrings = m_pConsole->GetConsoleStrings();

    ezUInt32 uiNumConsoleLines = (ezUInt32)(ezMath::Ceil(fConsoleTextAreaHeight / fTextHeight));
    ezInt32 iFirstLinePos = (ezInt32)fConsoleTextAreaHeight - uiNumConsoleLines * iTextHeight;
    ezInt32 uiFirstLine = m_pConsole->GetScrollPosition() + uiNumConsoleLines - 1;
    ezInt32 uiSkippedLines = ezMath::Max(uiFirstLine - (ezInt32)consoleStrings.GetCount() + 1, 0);

    for (ezUInt32 i = uiSkippedLines; i < uiNumConsoleLines; ++i)
    {
      auto& consoleString = consoleStrings[uiFirstLine - i];
      ezDebugRenderer::DrawText(pView, consoleString.m_sText, ezVec2I32(iTextLeft, iFirstLinePos + i * iTextHeight), consoleString.m_TextColor);
    }

    ezStringView sInputLine(m_pConsole->GetInputLine());
    ezDebugRenderer::DrawText(pView, sInputLine, ezVec2I32(iTextLeft, (ezInt32)(fConsoleTextAreaHeight + fBorderWidth)), ezColor::White);

    if (ezMath::Fraction(ezClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds()) > 0.5)
    {
      float fCaretPosition = (float)m_pConsole->GetCaretPosition();
      ezColor caretColor(1.0f, 1.0f, 1.0f, 0.5f);
      ezDebugRenderer::Draw2DRectangle(pView, ezRectFloat(fBorderWidth + fCaretPosition * 8.0f + 2.0f, fConsoleTextAreaHeight + fBorderWidth + 1.0f, 2.0f, fTextHeight - 2.0f), 0.0f, caretColor);
    }
  }
}

EZ_STATICLINK_FILE(GameFoundation, GameFoundation_GameApplication);

