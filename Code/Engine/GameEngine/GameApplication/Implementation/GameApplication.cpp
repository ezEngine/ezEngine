
#include <PCH.h>
#include <GameEngine/Console/Console.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <Core/World/World.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/DefaultTimeStepSmoothing.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Foundation/Image/Formats/TgaFileFormat.h>
#include <Foundation/Image/Image.h>

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
#include <GameEngine/MixedReality/MixedRealityFramework.h>
#endif

ezGameApplication* ezGameApplication::s_pGameApplicationInstance = nullptr;
ezDelegate<ezGALDevice* (const ezGALDeviceCreationDescription&)> ezGameApplication::s_DefaultDeviceCreator;
ezCVarBool CVarEnableVSync("g_VSync", false, ezCVarFlags::Save, "Enables V-Sync");
ezCVarBool CVarShowFPS("g_ShowFPS", false, ezCVarFlags::Save, "Show frames per second counter");

ezGameApplication::ezGameApplication(const char* szAppName, ezGameApplicationType type, const char* szProjectPath /*= nullptr*/)
  : m_sAppProjectPath(szProjectPath)
  , m_UpdateTask("GameApplication.Update", ezMakeDelegate(&ezGameApplication::UpdateWorldsAndExtractViews, this))
  , m_ConFunc_TakeScreenshot("Screenshot", "()", ezMakeDelegate(&ezGameApplication::TakeScreenshot, this))
{
  m_sAppName = szAppName;
  s_pGameApplicationInstance = this;
  m_bWasQuitRequested = false;
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
  ezGALSwapChainCreationDescription desc;
  desc.m_pWindow = pWindow;
  desc.m_BackBufferFormat = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
  desc.m_bAllowScreenshots = true;
  auto hSwapChain = ezGALDevice::GetDefaultDevice()->CreateSwapChain(desc);

  AddWindow(pWindow, hSwapChain);

  return hSwapChain;
}

void ezGameApplication::AddWindow(ezWindowBase* pWindow, ezGALSwapChainHandle hSwapChain)
{
  // make sure not to add the same window twice
  RemoveWindow(pWindow);

  WindowContext& windowContext = m_Windows.ExpandAndGetRef();
  windowContext.m_pWindow = pWindow;
  windowContext.m_hSwapChain = hSwapChain;
  windowContext.m_bFirstFrame = true;
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

// static
void ezGameApplication::SetOverrideDefaultDeviceCreator(ezDelegate<ezGALDevice* (const ezGALDeviceCreationDescription&)> creator)
{
  s_DefaultDeviceCreator = creator;
}

void ezGameApplication::SetSwapChain(const ezWindowBase* pWindow, ezGALSwapChainHandle hSwapChain)
{
  for (auto& windowContext : m_Windows)
  {
    if (windowContext.m_pWindow == pWindow)
    {
      if (!windowContext.m_hSwapChain.IsInvalidated())
        ezGALDevice::GetDefaultDevice()->DestroySwapChain(windowContext.m_hSwapChain);
      windowContext.m_hSwapChain = hSwapChain;
      return;
    }
  }

  EZ_REPORT_FAILURE("Given window is not part of the application!");
}

void ezGameApplication::CreateGameStateForWorld(ezWorld* pWorld)
{
  EZ_LOG_BLOCK("Create Game State");

  ezGameState* pCurState = CreateCustomGameStateForWorld(pWorld);

  if (pCurState == nullptr)
  {
    float fBestPriority = -1.0f;

    for (auto pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
    {
      if (!pRtti->IsDerivedFrom<ezGameState>())
        continue;

      if (!pRtti->GetAllocator()->CanAllocate())
        continue;

      ezGameState* pState = pRtti->GetAllocator()->Allocate<ezGameState>();

      EZ_ASSERT_DEV(pState != nullptr, "Failed to allocate ezGameState object");

      pState->m_pApplication = this;

      float fPriority = (float)pState->DeterminePriority(m_AppType, pWorld);

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
  }

  if (pCurState)
  {
    pCurState->m_pApplication = this;

    GameStateData& gsd = m_GameStates.ExpandAndGetRef();
    gsd.m_pState = pCurState;
    gsd.m_pLinkedToWorld = pWorld;
  }
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


void ezGameApplication::BeforeCoreStartup()
{
  ezStartup::AddApplicationTag("runtime");

  ezApplication::BeforeCoreStartup();

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  if (m_AppType == ezGameApplicationType::StandAloneMixedReality ||
      m_AppType == ezGameApplicationType::EmbeddedInToolMixedReality)
  {
    m_pMixedRealityFramework = EZ_DEFAULT_NEW(ezMixedRealityFramework, nullptr);
  }
#endif
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
  for (ezUInt32 i = 0; i < m_GameStates.GetCount(); ++i)
  {
    if (m_GameStates[i].m_pLinkedToWorld == pWorld)
    {
      if (!m_GameStates[i].m_bStateActive)
      {
        m_GameStates[i].m_bStateActive = true;
        m_GameStates[i].m_pState->OnActivation(pWorld);
      }
    }
  }
}


void ezGameApplication::DeactivateGameStateForWorld(ezWorld* pWorld)
{
  for (ezUInt32 i = 0; i < m_GameStates.GetCount(); ++i)
  {
    if (m_GameStates[i].m_pLinkedToWorld == pWorld)
    {
      if (m_GameStates[i].m_bStateActive)
      {
        m_GameStates[i].m_bStateActive = false;
        m_GameStates[i].m_pState->OnDeactivation();
      }
    }
  }
}

void ezGameApplication::ActivateAllGameStates()
{
  // There is always at least one gamestate, but if it is null, then our application might not use them at all.
  if (m_GameStates.IsEmpty() || m_GameStates[0].m_pState == nullptr)
    return;

  for (ezUInt32 i = 0; i < m_GameStates.GetCount(); ++i)
  {
    if (!m_GameStates[i].m_bStateActive)
    {
      m_GameStates[i].m_bStateActive = true;
      m_GameStates[i].m_pState->OnActivation(m_GameStates[i].m_pLinkedToWorld);
    }
  }
}


void ezGameApplication::DeactivateAllGameStates()
{
  // There is always at least one gamestate, but if it is null, then our application might not use them at all.
  if (m_GameStates.IsEmpty() || m_GameStates[0].m_pState == nullptr)
    return;

  for (ezUInt32 i = 0; i < m_GameStates.GetCount(); ++i)
  {
    if (m_GameStates[i].m_bStateActive)
    {
      m_GameStates[i].m_bStateActive = false;
      m_GameStates[i].m_pState->OnDeactivation();
    }
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

ezWorld* ezGameApplication::CreateWorld(ezWorldDesc& desc)
{
  auto& wd = m_Worlds.ExpandAndGetRef();
  wd.m_pTimeStepSmoothing = EZ_DEFAULT_NEW(ezDefaultTimeStepSmoothing);
  wd.m_pWorld = EZ_DEFAULT_NEW(ezWorld, desc);
  wd.m_pWorld->GetClock().SetTimeStepSmoothing(wd.m_pTimeStepSmoothing);

  ezGameApplicationEvent e;
  e.m_Type = ezGameApplicationEvent::Type::AfterWorldCreated;
  e.m_pData = wd.m_pWorld;
  m_Events.Broadcast(e);

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

  ezGameApplicationEvent e;
  e.m_Type = ezGameApplicationEvent::Type::BeforeWorldDestroyed;
  e.m_pData = wd->m_pWorld;
  m_Events.Broadcast(e);


  wd->m_pWorld->GetClock().SetTimeStepSmoothing(nullptr);
  wd->m_pWorld = nullptr;
  EZ_DEFAULT_DELETE(wd->m_pTimeStepSmoothing);
  wd->m_pTimeStepSmoothing = nullptr;

  EZ_DEFAULT_DELETE(pWorld);
}

void ezGameApplication::AfterCoreStartup()
{
  DoProjectSetup();

  // Create gamestate.
  if (m_AppType == ezGameApplicationType::StandAlone
#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
      || m_AppType == ezGameApplicationType::StandAloneMixedReality
#endif
      )
  {
    CreateGameStateForWorld(nullptr);
  }
  else
  {
    // Special case: Must be handled by custom implementations of ezGameApplication
  }

  // Gamestate determines which graphics device is used, so delay this until we have gamestates.
  DoSetupGraphicsDevice();
  DoSetupDefaultResources();

  ezStartup::StartupEngine();


  // Activate gamestate
  if (m_AppType == ezGameApplicationType::StandAlone
#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
      || m_AppType == ezGameApplicationType::StandAloneMixedReality
#endif
      )
  {
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

  ezResourceManager::ClearAllResourceFallbacks();

  ezResourceManager::FreeUnusedResources(true);

  ezStartup::ShutdownEngine();

  ezFrameAllocator::Reset();
  ezResourceManager::FreeUnusedResources(true);

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  m_pMixedRealityFramework = nullptr;
#endif

  DoShutdownGraphicsDevice();

  DestroyAllGameStates();

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

    // for plugins that need to hook into this without a link dependency on this lib
    EZ_BROADCAST_EVENT(GameApp_BeginAppTick);

    {
      ezGameApplicationEvent e;
      e.m_Type = ezGameApplicationEvent::Type::BeginAppTick;
      m_Events.Broadcast(e);
    }

    if (ezRenderWorld::GetMainViews().GetCount() > 0)
    {
      ezClock::GetGlobalClock()->Update();

      UpdateInput();

      UpdateWorldsAndRender();
    }

    // for plugins that need to hook into this without a link dependency on this lib
    EZ_BROADCAST_EVENT(GameApp_EndAppTick);

    {
      ezGameApplicationEvent e;
      e.m_Type = ezGameApplicationEvent::Type::EndAppTick;
      m_Events.Broadcast(e);
    }

  }

  return m_bWasQuitRequested ? ezApplication::Quit : ezApplication::Continue;
}

void ezGameApplication::UpdateWorldsAndRender()
{
  ezRenderWorld::BeginFrame();

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // On most platforms it doesn't matter that much how early this happens.
  // But on HoloLens this executes something that needs to be done at the right time,
  // for the reprojection to work properly.
  pDevice->BeginFrame();

  ezTaskGroupID updateTaskID;
  if (ezRenderWorld::GetUseMultithreadedRendering())
  {
    updateTaskID = ezTaskSystem::StartSingleTask(&m_UpdateTask, ezTaskPriority::EarlyThisFrame);
  }
  else
  {
    UpdateWorldsAndExtractViews();
  }

  {

    RenderFps();
    RenderConsole();

    ezRenderWorld::Render(ezRenderContext::GetDefaultInstance());

    if (ezRenderWorld::GetUseMultithreadedRendering())
    {
      EZ_PROFILE("Wait for UpdateWorldsAndExtractViews");
      ezTaskSystem::WaitForGroup(updateTaskID);
    }

    ezTelemetry::PerFrameUpdate();
    ezResourceManager::PerFrameUpdate();
    ezTaskSystem::FinishFrameTasks();

    {
      ezGameApplicationEvent e;
      e.m_Type = ezGameApplicationEvent::Type::BeforePresent;
      m_Events.Broadcast(e);
    }

    {
      EZ_PROFILE("GameApplication.Present");
      for (auto& windowContext : m_Windows)
      {
        // Ignore windows without swapchain
        if (windowContext.m_hSwapChain.IsInvalidated())
          continue;

        if (ezRenderWorld::GetFrameCounter() < 10)
          ezLog::Debug("Finishing Frame: {0}", ezRenderWorld::GetFrameCounter());

        if (ezRenderWorld::GetUseMultithreadedRendering() && windowContext.m_bFirstFrame)
        {
          windowContext.m_bFirstFrame = false;
        }
        else
        {
          if (m_bTakeScreenshot)
          {
            ezImage img;
            DoTakeScreenshot(windowContext.m_hSwapChain, img);
            DoSaveScreenshot(img);
          }

          pDevice->Present(windowContext.m_hSwapChain, CVarEnableVSync);
        }
      }
    }

    pDevice->EndFrame();
  }

  ezRenderWorld::EndFrame();

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

  auto mainViews = ezRenderWorld::GetMainViews();
  for (auto hView : mainViews)
  {
    ezView* pView = nullptr;
    if (ezRenderWorld::TryGetView(hView, pView))
    {
      ezWorld* pWorld = pView->GetWorld();

      if (pWorld != nullptr && !worldsToUpdate.Contains(pWorld))
      {
        worldsToUpdate.PushBack(pWorld);
      }
    }
  }

  if (ezRenderWorld::GetUseMultithreadedRendering())
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

    {
      ezGameApplicationEvent e;
      e.m_Type = ezGameApplicationEvent::Type::BeforeWorldUpdates;
      m_Events.Broadcast(e);
    }

    for (ezUInt32 i = 0; i < m_GameStates.GetCount(); ++i)
    {
      /// \todo Pause state ?
      if (m_GameStates[i].m_pState)
      {
        m_GameStates[i].m_pState->AfterWorldUpdate();
      }
    }

    {
      ezGameApplicationEvent e;
      e.m_Type = ezGameApplicationEvent::Type::AfterWorldUpdates;
      m_Events.Broadcast(e);
    }
  }

  {
    ezGameApplicationEvent e;
    e.m_Type = ezGameApplicationEvent::Type::BeforeUpdatePlugins;
    m_Events.Broadcast(e);
  }

  // for plugins that need to hook into this without a link dependency on this lib
  EZ_BROADCAST_EVENT(GameApp_UpdatePlugins);

  {
    ezGameApplicationEvent e;
    e.m_Type = ezGameApplicationEvent::Type::AfterUpdatePlugins;
    m_Events.Broadcast(e);
  }

  ezRenderWorld::ExtractMainViews();
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
  // Do not use ezClock for this, it smooths and clamps the timestep
  const ezTime tNow = ezTime::Now();

  static ezTime fAccumTime;
  static ezTime fElapsedTime;
  static ezTime tLast = tNow;
  const ezTime tDiff = tNow - tLast;
  fAccumTime += tDiff;
  tLast = tNow;

  if (fAccumTime >= ezTime::Seconds(0.5))
  {
    fAccumTime -= ezTime::Seconds(0.5);

    fElapsedTime = tDiff;
  }

  if (CVarShowFPS)
  {
    if (const ezView* pView = ezRenderWorld::GetViewByUsageHint(ezCameraUsageHint::MainView))
    {
      ezStringBuilder sFps;
      sFps.Format("{0} fps, {1} ms", ezArgF(1.0 / fElapsedTime.GetSeconds(), 1, false, 4), ezArgF(fElapsedTime.GetSeconds() * 1000.0, 1, false, 4));

      ezInt32 viewHeight = (ezInt32)(pView->GetViewport().height);

      ezDebugRenderer::DrawText(pView->GetHandle(), sFps, ezVec2I32(10, viewHeight - 10 - 16), ezColor::White);
    }
  }
}

void ezGameApplication::RenderConsole()
{
  if (!m_bShowConsole || !m_pConsole)
    return;

  const ezView* pView = ezRenderWorld::GetViewByUsageHint(ezCameraUsageHint::MainView);
  if (pView == nullptr)
    return;

  ezViewHandle hView = pView->GetHandle();

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
    ezDebugRenderer::Draw2DRectangle(hView, ezRectFloat(0.0f, 0.0f, fViewWidth, fConsoleHeight), 0.0f, backgroundColor);

    ezColor foregroundColor(0.0f, 0.0f, 0.0f, 0.8f);
    ezDebugRenderer::Draw2DRectangle(hView, ezRectFloat(fBorderWidth, 0.0f, fViewWidth - (2.0f * fBorderWidth), fConsoleTextAreaHeight), 0.0f, foregroundColor);
    ezDebugRenderer::Draw2DRectangle(hView, ezRectFloat(fBorderWidth, fConsoleTextAreaHeight + fBorderWidth, fViewWidth - (2.0f * fBorderWidth), fTextHeight), 0.0f, foregroundColor);
  }

  {
    EZ_LOCK(m_pConsole->GetMutex());

    auto& consoleStrings = m_pConsole->GetConsoleStrings();

    ezUInt32 uiNumConsoleLines = (ezUInt32)(ezMath::Ceil(fConsoleTextAreaHeight / fTextHeight));
    ezInt32 iFirstLinePos = (ezInt32)fConsoleTextAreaHeight - uiNumConsoleLines * iTextHeight;
    ezInt32 uiFirstLine = m_pConsole->GetScrollPosition() + uiNumConsoleLines - 1;
    ezInt32 uiSkippedLines = ezMath::Max(uiFirstLine - (ezInt32)consoleStrings.GetCount() + 1, 0);

    for (ezUInt32 i = uiSkippedLines; i < uiNumConsoleLines; ++i)
    {
      auto& consoleString = consoleStrings[uiFirstLine - i];
      ezDebugRenderer::DrawText(hView, consoleString.m_sText, ezVec2I32(iTextLeft, iFirstLinePos + i * iTextHeight), consoleString.m_TextColor);
    }

    ezStringView sInputLine(m_pConsole->GetInputLine());
    ezDebugRenderer::DrawText(hView, sInputLine, ezVec2I32(iTextLeft, (ezInt32)(fConsoleTextAreaHeight + fBorderWidth)), ezColor::White);

    if (ezMath::Fraction(ezClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds()) > 0.5)
    {
      float fCaretPosition = (float)m_pConsole->GetCaretPosition();
      ezColor caretColor(1.0f, 1.0f, 1.0f, 0.5f);
      ezDebugRenderer::Draw2DRectangle(hView, ezRectFloat(fBorderWidth + fCaretPosition * 8.0f + 2.0f, fConsoleTextAreaHeight + fBorderWidth + 1.0f, 2.0f, fTextHeight - 2.0f), 0.0f, caretColor);
    }
  }
}

void ezGameApplication::TakeScreenshot()
{
  m_bTakeScreenshot = true;
}

void ezGameApplication::DoTakeScreenshot(const ezGALSwapChainHandle& swapchain, ezImage& out_Image)
{
  m_bTakeScreenshot = false;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALTextureHandle hBackbuffer = pDevice->GetBackBufferTextureFromSwapChain(swapchain);

  ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->ReadbackTexture(hBackbuffer);

  const ezGALTexture* pBackbuffer = ezGALDevice::GetDefaultDevice()->GetTexture(hBackbuffer);
  const ezUInt32 uiWidth = pBackbuffer->GetDescription().m_uiWidth;
  const ezUInt32 uiHeight = pBackbuffer->GetDescription().m_uiHeight;

  ezDynamicArray<ezUInt8> backbufferData;
  backbufferData.SetCountUninitialized(uiWidth * uiHeight * 4);

  ezGALSystemMemoryDescription MemDesc;
  MemDesc.m_uiRowPitch = 4 * uiWidth;
  MemDesc.m_uiSlicePitch = 4 * uiWidth * uiHeight;

  /// \todo Make this more efficient
  MemDesc.m_pData = backbufferData.GetData();
  ezArrayPtr<ezGALSystemMemoryDescription> SysMemDescsDepth(&MemDesc, 1);
  ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->CopyTextureReadbackResult(hBackbuffer, &SysMemDescsDepth);

  out_Image.SetWidth(uiWidth);
  out_Image.SetHeight(uiHeight);
  out_Image.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);
  out_Image.AllocateImageData();
  ezUInt8* pData = out_Image.GetDataPointer<ezUInt8>();

  ezMemoryUtils::Copy(pData, backbufferData.GetData(), backbufferData.GetCount());
}

void ezGameApplication::DoSaveScreenshot(ezImage& image)
{
  class WriteFileTask : public ezTask
  {
  public:
    ezImage m_Image;

  private:
    virtual void Execute() override
    {
      const ezDateTime dt = ezTimestamp::CurrentTimestamp();

      ezStringBuilder sPath;
      sPath.Format(":appdata/Screenshots/{6} {0}-{1}-{2} {3}-{4}-{5}-{7}.tga",
                   dt.GetYear(), ezArgU(dt.GetMonth(), 2, true), ezArgU(dt.GetDay(), 2, true),
                   ezArgU(dt.GetHour(), 2, true), ezArgU(dt.GetMinute(), 2, true), ezArgU(dt.GetSecond(), 2, true),
                   ezGameApplication::GetGameApplicationInstance()->GetAppName(),
                   ezArgU(dt.GetMicroseconds() / 1000, 3, true));

      /// \todo Get rid of Alpha channel before saving

      if (m_Image.SaveTo(sPath).Succeeded())
      {
        ezLog::Info("Screenshot saved to '{0}'.", sPath);
      }
    }
  };

  WriteFileTask* pWriteTask = EZ_DEFAULT_NEW(WriteFileTask);
  pWriteTask->m_Image = image;
  pWriteTask->SetOnTaskFinished([](ezTask* pTask)
  {
    EZ_DEFAULT_DELETE(pTask);
  });

  // we move the file writing off to another thread to save some time
  // if we moved it to the 'FileAccess' thread, writing a screenshot would block resource loading, which can reduce game performance
  // 'LongRunning' will give it even less priority and let the task system do them in parallel to other things
  ezTaskSystem::StartSingleTask(pWriteTask, ezTaskPriority::LongRunning);
}


bool ezGameApplication::HasAnyActiveGameState() const
{
  for (const auto gs : GetAllGameStates())
  {
    if (gs.m_pState != nullptr)
      return true;
  }

  return false;
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_GameApplication_Implementation_GameApplication);

