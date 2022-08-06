#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/ATL/AudioTranslationLayer.h>
#include <AudioSystemPlugin/Core/AudioMiddleware.h>
#include <AudioSystemPlugin/Core/AudioSystem.h>
#include <AudioSystemPlugin/Core/AudioSystemAllocator.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Profiling/Profiling.h>

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
#  include <AudioSystemPlugin/Core/AudioMiddleware.h>
#  include <RendererCore/Debug/DebugRenderer.h>
#  include <RendererCore/Pipeline/View.h>
#  include <RendererCore/RenderWorld/RenderWorld.h>
#endif

ezCVarBool cvar_AudioSystemDebug("Audio.Debugging.Enable", false, ezCVarFlags::None, "Defines if Audio System debug information are displayed.");

ezAudioTranslationLayer::ezAudioTranslationLayer()
  : m_mEntities()
  , m_mTriggers()
{
}

ezAudioTranslationLayer::~ezAudioTranslationLayer() = default;

ezResult ezAudioTranslationLayer::Startup()
{
  auto* pAudioMiddleware = ezSingletonRegistry::GetSingletonInstance<ezAudioMiddleware>();

  if (pAudioMiddleware == nullptr)
  {
    ezLog::Error("Unable to load the ATL, there is no audio middleware implementation found. Make sure you have enabled an audio middleware plugin.");
    return EZ_FAILURE;
  }

  // Load configuration
  {
    const char* szFile = ":project/Sounds/AudioSystemConfig.ddl";

    ezFileReader file;
    EZ_SUCCEED_OR_RETURN(file.Open(szFile));

    ezOpenDdlReader ddl;
    EZ_SUCCEED_OR_RETURN(ddl.ParseDocument(file));

    const ezOpenDdlReaderElement* pRoot = ddl.GetRootElement();
    const ezOpenDdlReaderElement* pChild = pRoot->GetFirstChild();

    while (pChild)
    {
      if (pChild->IsCustomType("Middleware") && pChild->HasName() && ezStringUtils::Compare(pChild->GetName(), pAudioMiddleware->GetMiddlewareName()) == 0)
      {
        ezLog::Debug("Loading audio middleware configuration for {}...", pChild->GetName());

        if (pAudioMiddleware->LoadConfiguration(*pChild).Failed())
          ezLog::Error("Failed to load configuration for audio middleware: {0}.", pChild->GetName());

        break;
      }

      pChild = pChild->GetSibling();
    }
  }

  // Start the audio middleware
  const ezResult result = pAudioMiddleware->Startup();

  if (result.Succeeded())
  {
    ezLog::Success("ATL loaded successfully. Using {0} as the audio middleware.", pAudioMiddleware->GetMiddlewareName());
    return EZ_SUCCESS;
  }

  ezLog::Error("Unable to load the ATL. An error occurred while loading the audio middleware.");
  return EZ_FAILURE;
}

void ezAudioTranslationLayer::Shutdown()
{
  ezLog::Info("ATL unloaded");
}

void ezAudioTranslationLayer::Update()
{
  EZ_PROFILE_SCOPE("AudioSystem");

  const ezTime currentUpdateTime = ezTime::Now();
  m_LastFrameTime = currentUpdateTime - m_LastUpdateTime;
  m_LastUpdateTime = currentUpdateTime;

  auto* pAudioMiddleware = ezSingletonRegistry::GetSingletonInstance<ezAudioMiddleware>();

  if (pAudioMiddleware == nullptr)
    return;

  pAudioMiddleware->Update(m_LastFrameTime);
}

ezAudioSystemDataID ezAudioTranslationLayer::GetTriggerId(const char* szTriggerName) const
{
  const auto uiTriggerId = ezHashHelper<const char*>::Hash(szTriggerName);
  if (const auto it = m_mTriggers.Find(uiTriggerId); it.IsValid())
  {
    ezLog::Info("Found trigger {0}: {1}", szTriggerName, uiTriggerId);
    return uiTriggerId;
  }

  return 0;
}

void ezAudioTranslationLayer::ProcessRequest(ezVariant&& request)
{
  ezLog::Info("ATL received a request");

  auto* pAudioMiddleware = ezSingletonRegistry::GetSingletonInstance<ezAudioMiddleware>();

  if (pAudioMiddleware == nullptr)
    return;

  bool needCallback = false;

  if (request.IsA<ezAudioSystemRequestRegisterEntity>())
  {
    auto& audioRequest = request.GetWritable<ezAudioSystemRequestRegisterEntity>();
    ezAudioSystemEntityData* entity = pAudioMiddleware->CreateEntityData(audioRequest.m_uiEntityId);

    if (entity == nullptr)
    {
      ezLog::Error("Failed to create entity data for entity {0}", audioRequest.m_uiEntityId);
      return;
    }

    m_mEntities[audioRequest.m_uiEntityId] = EZ_AUDIOSYSTEM_NEW(ezATLEntity, audioRequest.m_uiEntityId, entity);
    audioRequest.m_eStatus = pAudioMiddleware->AddEntity(entity, audioRequest.m_sName);

    needCallback = audioRequest.m_Callback.IsValid();
  }

  if (request.IsA<ezAudioSystemRequestUnregisterEntity>())
  {
    auto& audioRequest = request.GetWritable<ezAudioSystemRequestUnregisterEntity>();

    if (!m_mEntities.Contains(audioRequest.m_uiEntityId))
    {
      ezLog::Error("Failed to unregister entity {0}. Make sure it was registered before.", audioRequest.m_uiEntityId);
      return;
    }

    const auto& entity = m_mEntities[audioRequest.m_uiEntityId];
    pAudioMiddleware->RemoveEntity(entity->m_pEntityData).IgnoreResult();
    audioRequest.m_eStatus = pAudioMiddleware->DestroyEntityData(entity->m_pEntityData);

    needCallback = audioRequest.m_Callback.IsValid();
  }

  if (request.IsA<ezAudioSystemRequestLoadTrigger>())
  {
    auto& audioRequest = request.GetWritable<ezAudioSystemRequestLoadTrigger>();

    if (!m_mEntities.Contains(audioRequest.m_uiEntityId))
    {
      ezLog::Error("Failed to load trigger {0}. It references an unregistered entity {1}.", audioRequest.m_uiObjectId, audioRequest.m_uiEntityId);
      return;
    }

    if (!m_mTriggers.Contains(audioRequest.m_uiObjectId))
    {
      ezLog::Error("Failed to load trigger {0}. Make sure it was registered before.", audioRequest.m_uiObjectId);
      return;
    }

    ezAudioSystemEventData* pEventData = pAudioMiddleware->CreateEventData(audioRequest.m_uiEventId);

    if (pEventData == nullptr)
    {
      ezLog::Error("Failed to load trigger {0}. Unable to allocate memory for the linked event with ID {1}.", audioRequest.m_uiObjectId, audioRequest.m_uiEventId);
      return;
    }

    const auto& entity = m_mEntities[audioRequest.m_uiEntityId];
    const auto& trigger = m_mTriggers[audioRequest.m_uiObjectId];
    trigger->AttachEvent(audioRequest.m_uiEventId, pEventData);

    audioRequest.m_eStatus = pAudioMiddleware->LoadTrigger(entity->m_pEntityData, trigger->m_pTriggerData, pEventData);

    needCallback = audioRequest.m_Callback.IsValid();
  }

  if (request.IsA<ezAudioSystemRequestActivateTrigger>())
  {
    auto& audioRequest = request.GetWritable<ezAudioSystemRequestActivateTrigger>();

    if (!m_mEntities.Contains(audioRequest.m_uiEntityId))
    {
      ezLog::Error("Failed to activate trigger {0}. It references an unregistered entity {1}.", audioRequest.m_uiObjectId, audioRequest.m_uiEntityId);
      return;
    }

    if (!m_mTriggers.Contains(audioRequest.m_uiObjectId))
    {
      ezLog::Error("Failed to activate trigger {0}. Make sure it was registered before.", audioRequest.m_uiObjectId);
      return;
    }

    const auto& entity = m_mEntities[audioRequest.m_uiEntityId];
    const auto& trigger = m_mTriggers[audioRequest.m_uiObjectId];

    ezAudioSystemEventData* pEventData = nullptr;
    if (trigger->GetEvent(audioRequest.m_uiEventId, pEventData).Failed())
    {
      ezLog::Error("Failed to activate trigger {0}. Make sure to load the trigger before to activate it.", audioRequest.m_uiObjectId);
      return;
    }

    audioRequest.m_eStatus = pAudioMiddleware->ActivateTrigger(entity->m_pEntityData, trigger->m_pTriggerData, pEventData);

    needCallback = audioRequest.m_Callback.IsValid();
  }

  if (request.IsA<ezAudioSystemRequestStopEvent>())
  {
    auto& audioRequest = request.GetWritable<ezAudioSystemRequestStopEvent>();

    if (!m_mEntities.Contains(audioRequest.m_uiEntityId))
    {
      ezLog::Error("Failed to stop trigger {0}. It references an unregistered entity {1}.", audioRequest.m_uiObjectId, audioRequest.m_uiEntityId);
      return;
    }

    if (!m_mTriggers.Contains(audioRequest.m_uiTriggerId))
    {
      ezLog::Error("Failed to stop trigger {0}. Make sure it was registered before.", audioRequest.m_uiObjectId);
      return;
    }

    const auto& entity = m_mEntities[audioRequest.m_uiEntityId];
    const auto& trigger = m_mTriggers[audioRequest.m_uiTriggerId];

    ezAudioSystemEventData* pEventData = nullptr;
    if (trigger->GetEvent(audioRequest.m_uiObjectId, pEventData).Failed())
    {
      ezLog::Error("Failed to stop trigger {0}. Make sure to load the trigger before to stop it.", audioRequest.m_uiObjectId);
      return;
    }

    audioRequest.m_eStatus = pAudioMiddleware->StopEvent(entity->m_pEntityData, pEventData);

    needCallback = audioRequest.m_Callback.IsValid();
  }

  if (request.IsA<ezAudioSystemRequestShutdown>())
  {
    auto& audioRequest = request.GetWritable<ezAudioSystemRequestShutdown>();
    audioRequest.m_eStatus = pAudioMiddleware->Shutdown();
    needCallback = audioRequest.m_Callback.IsValid();
  }

  if (needCallback)
  {
    ezAudioSystem::GetSingleton()->QueueRequestCallback(std::move(request));
  }
}

void ezAudioTranslationLayer::RegisterTrigger(ezAudioSystemDataID uiId, ezAudioSystemTriggerData* pTriggerData)
{
  if (m_mTriggers.Contains(uiId))
  {
    ezLog::Warning("ATL: Trigger with id {0} already exists. Skipping new registration.", uiId);
    return;
  }

  ezLog::Info("ATL: Registering a trigger with id {0}", uiId);
  m_mTriggers[uiId] = EZ_AUDIOSYSTEM_NEW(ezATLTrigger, uiId, pTriggerData);
}

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
void ezAudioTranslationLayer::DebugRender()
{
  static ezTime tAccumTime;
  static ezTime tDisplayedFrameTime = m_LastFrameTime;
  static ezUInt32 uiFrames = 0;
  static ezUInt32 uiFPS = 0;

  ++uiFrames;
  tAccumTime += m_LastFrameTime;

  if (tAccumTime >= ezTime::Seconds(0.5))
  {
    tAccumTime -= ezTime::Seconds(0.5);
    tDisplayedFrameTime = m_LastFrameTime;

    uiFPS = uiFrames * 2;
    uiFrames = 0;
  }

  if (cvar_AudioSystemDebug)
  {
    const auto* pAudioMiddleware = ezSingletonRegistry::GetSingletonInstance<ezAudioMiddleware>();

    if (pAudioMiddleware == nullptr)
      return;

    if (const ezView* pView = ezRenderWorld::GetViewByUsageHint(ezCameraUsageHint::MainView, ezCameraUsageHint::EditorView))
    {
      ezDebugRenderer::DrawInfoText(pView->GetHandle(), ezDebugRenderer::ScreenPlacement::BottomRight, "AudioSystem", ezFmt("ATL ({0}) - {1} fps, {2} ms", pAudioMiddleware->GetMiddlewareName(), uiFPS, ezArgF(tDisplayedFrameTime.GetMilliseconds(), 1, false, 4)));
    }
  }
}
#endif

EZ_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_ATL_AudioTranslationLayer);
