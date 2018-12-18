#include <PCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Image/Formats/TgaFileFormat.h>
#include <Foundation/Image/Image.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/DefaultTimeStepSmoothing.h>
#include <GameEngine/Console/Console.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
#  include <GameEngine/MixedReality/MixedRealityFramework.h>
#endif

ezGameApplication* ezGameApplication::s_pGameApplicationInstance = nullptr;
ezDelegate<ezGALDevice*(const ezGALDeviceCreationDescription&)> ezGameApplication::s_DefaultDeviceCreator;

ezCVarBool CVarEnableVSync("g_VSync", false, ezCVarFlags::Save, "Enables V-Sync");
ezCVarBool CVarShowFPS("g_ShowFPS", false, ezCVarFlags::Save, "Show frames per second counter");

ezGameApplication::ezGameApplication(const char* szAppName, ezGameApplicationType type, const char* szProjectPath /*= nullptr*/)
    : ezGameApplicationBase(szAppName)
    , m_sAppProjectPath(szProjectPath)
    , m_UpdateTask("GameApplication.Update", ezMakeDelegate(&ezGameApplication::UpdateWorldsAndExtractViews, this))
{
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

ezUniquePtr<ezWindowOutputTargetBase> ezGameApplication::CreateWindowOutputTarget(ezWindowBase* pWindow)
{
  ezGALSwapChainCreationDescription desc;
  desc.m_pWindow = pWindow;
  desc.m_BackBufferFormat = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
  desc.m_bAllowScreenshots = true;
  auto hSwapChain = ezGALDevice::GetDefaultDevice()->CreateSwapChain(desc);

  ezUniquePtr<ezWindowOutputTargetGAL> pOutputTarget = EZ_DEFAULT_NEW(ezWindowOutputTargetGAL);
  pOutputTarget->m_hSwapChain = hSwapChain;

  return std::move(pOutputTarget);
}

void ezGameApplication::DestroyWindowOutputTarget(ezUniquePtr<ezWindowOutputTargetBase> pOutputTarget)
{
  if (pOutputTarget != nullptr)
  {
    ezWindowOutputTargetGAL* pOutputGAL = static_cast<ezWindowOutputTargetGAL*>(pOutputTarget.Borrow());

    // do not try to destroy the primary swapchain, that is handled by the device
    if (ezGALDevice::GetDefaultDevice()->GetPrimarySwapChain() != pOutputGAL->m_hSwapChain)
    {
      ezGALDevice::GetDefaultDevice()->DestroySwapChain(pOutputGAL->m_hSwapChain);
    }

    pOutputTarget = nullptr;
  }
}


// static
void ezGameApplication::SetOverrideDefaultDeviceCreator(ezDelegate<ezGALDevice*(const ezGALDeviceCreationDescription&)> creator)
{
  s_DefaultDeviceCreator = creator;
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
  if (m_AppType == ezGameApplicationType::StandAloneMixedReality || m_AppType == ezGameApplicationType::EmbeddedInToolMixedReality)
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


void ezGameApplication::ActivateGameStateForWorld(ezWorld* pWorld, const ezTransform* pStartPosition)
{
  for (ezUInt32 i = 0; i < m_GameStates.GetCount(); ++i)
  {
    if (m_GameStates[i].m_pLinkedToWorld == pWorld)
    {
      if (!m_GameStates[i].m_bStateActive)
      {
        m_GameStates[i].m_bStateActive = true;
        m_GameStates[i].m_pState->OnActivation(pWorld, pStartPosition);
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

void ezGameApplication::ActivateAllGameStates(const ezTransform* pStartPosition)
{
  // There is always at least one gamestate, but if it is null, then our application might not use them at all.
  if (m_GameStates.IsEmpty() || m_GameStates[0].m_pState == nullptr)
    return;

  for (ezUInt32 i = 0; i < m_GameStates.GetCount(); ++i)
  {
    if (!m_GameStates[i].m_bStateActive)
    {
      m_GameStates[i].m_bStateActive = true;
      m_GameStates[i].m_pState->OnActivation(m_GameStates[i].m_pLinkedToWorld, pStartPosition);
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
  wd.m_pWorld = EZ_DEFAULT_NEW(ezWorld, desc);

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

  wd->m_pWorld = nullptr;

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
    ActivateAllGameStates(nullptr);
  }
  else
  {
    // Special case: Must be handled by custom implementations of ezGameApplication
  }
}

void ezGameApplication::BeforeCoreShutdown()
{
  for (auto& w : m_Worlds)
  {
    DestroyWorld(w.m_pWorld);
  }

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
  EZ_ASSERT_RELEASE(!m_sAppProjectPath.IsEmpty(), "Either the project must have a built in project directory passed to the "
                                                  "ezGameApplication constructor, or m_sAppProjectPath must be set manually before doing "
                                                  "project setup, or ezGameApplication::FindProjectDirectory() must be overridden.");

  if (ezPathUtils::IsAbsolutePath(m_sAppProjectPath))
    return m_sAppProjectPath;

  // first check if the path is relative to the Sdk special directory
  {
    ezStringBuilder relToSdk(">sdk/", m_sAppProjectPath);
    ezStringBuilder absToSdk;
    if (ezFileSystem::ResolveSpecialDirectory(relToSdk, absToSdk).Succeeded())
    {
      if (ezOSFile::ExistsDirectory(absToSdk))
        return absToSdk;
    }
  }

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

void ezGameApplication::UpdateWorldsAndRender_Begin()
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

  RenderFps();
  RenderConsole();

  ezRenderWorld::Render(ezRenderContext::GetDefaultInstance());

  if (ezRenderWorld::GetUseMultithreadedRendering())
  {
    EZ_PROFILE_SCOPE("Wait for UpdateWorldsAndExtractViews");
    ezTaskSystem::WaitForGroup(updateTaskID);
  }
}

void ezGameApplication::UpdateWorldsAndRender_Middle() {}

void ezGameApplication::UpdateWorldsAndRender_End()
{
  ezGALDevice::GetDefaultDevice()->EndFrame();
  ezRenderWorld::EndFrame();
}

void ezGameApplication::UpdateWorldsAndRender()
{
  UpdateWorldsAndRender_Begin();

  UpdateWorldsAndRender_Middle();

  {
    ezGameApplicationEvent e;
    e.m_Type = ezGameApplicationEvent::Type::BeforePresent;
    m_Events.Broadcast(e);
  }

  {
    EZ_PROFILE_SCOPE("GameApplication.Present");
    for (auto& windowContext : m_Windows)
    {
      // Ignore windows without an output target
      if (windowContext.m_pOutputTarget == nullptr)
        continue;

      if (ezRenderWorld::GetFrameCounter() < 10)
        ezLog::Debug("Finishing Frame: {0}", ezRenderWorld::GetFrameCounter());

      if (ezRenderWorld::GetUseMultithreadedRendering() && windowContext.m_bFirstFrame)
      {
        windowContext.m_bFirstFrame = false;
      }
      else
      {
        /// \todo This only works for the first window
        ExecuteTakeScreenshot(windowContext.m_pOutputTarget.Borrow());

        windowContext.m_pOutputTarget->Present(CVarEnableVSync);
      }
    }
  }

  {
    ezGameApplicationEvent e;
    e.m_Type = ezGameApplicationEvent::Type::AfterPresent;
    m_Events.Broadcast(e);
  }

  UpdateWorldsAndRender_End();

  {
    ezTelemetry::PerFrameUpdate();
    ezResourceManager::PerFrameUpdate();
    ezTaskSystem::FinishFrameTasks();
  }

  ezFrameAllocator::Swap();
}

void ezGameApplication::UpdateWorldsAndExtractViews()
{
  {
    EZ_PROFILE_SCOPE("GameApplication.BeforeWorldUpdate");

    for (ezUInt32 i = 0; i < m_GameStates.GetCount(); ++i)
    {
      /// \todo Pause state ?
      if (m_GameStates[i].m_pState)
      {
        m_GameStates[i].m_pState->BeforeWorldUpdate();
      }
    }

    {
      ezGameApplicationEvent e;
      e.m_Type = ezGameApplicationEvent::Type::BeforeWorldUpdates;
      m_Events.Broadcast(e);
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
    EZ_PROFILE_SCOPE("GameApplication.AfterWorldUpdate");

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
      sFps.Format("{0} fps, {1} ms", ezArgF(1.0 / fElapsedTime.GetSeconds(), 1, false, 4),
                  ezArgF(fElapsedTime.GetSeconds() * 1000.0, 1, false, 4));

      ezInt32 viewHeight = (ezInt32)(pView->GetViewport().height);

      ezDebugRenderer::Draw2DText(pView->GetHandle(), sFps, ezVec2I32(10, viewHeight - 10 - 16), ezColor::White);
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
    ezDebugRenderer::Draw2DRectangle(hView, ezRectFloat(fBorderWidth, 0.0f, fViewWidth - (2.0f * fBorderWidth), fConsoleTextAreaHeight),
                                     0.0f, foregroundColor);
    ezDebugRenderer::Draw2DRectangle(
        hView, ezRectFloat(fBorderWidth, fConsoleTextAreaHeight + fBorderWidth, fViewWidth - (2.0f * fBorderWidth), fTextHeight), 0.0f,
        foregroundColor);
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
      ezDebugRenderer::Draw2DText(hView, consoleString.m_sText, ezVec2I32(iTextLeft, iFirstLinePos + i * iTextHeight),
                                  consoleString.m_TextColor);
    }

    ezStringView sInputLine(m_pConsole->GetInputLine());
    ezDebugRenderer::Draw2DText(hView, sInputLine, ezVec2I32(iTextLeft, (ezInt32)(fConsoleTextAreaHeight + fBorderWidth)), ezColor::White);

    if (ezMath::Fraction(ezClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds()) > 0.5)
    {
      float fCaretPosition = (float)m_pConsole->GetCaretPosition();
      ezColor caretColor(1.0f, 1.0f, 1.0f, 0.5f);
      ezDebugRenderer::Draw2DRectangle(
          hView,
          ezRectFloat(fBorderWidth + fCaretPosition * 8.0f + 2.0f, fConsoleTextAreaHeight + fBorderWidth + 1.0f, 2.0f, fTextHeight - 2.0f),
          0.0f, caretColor);
    }
  }
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
