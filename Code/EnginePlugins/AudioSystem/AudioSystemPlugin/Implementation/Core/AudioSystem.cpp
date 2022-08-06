#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Core/AudioMiddleware.h>
#include <AudioSystemPlugin/Core/AudioSystem.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>

ezThreadID gMainThreadId = static_cast<ezThreadID>(0);

ezCVarInt cvar_AudioSystemMemoryEntitiesPoolSize("Audio.Memory.EntitiesPoolSize", 1024, ezCVarFlags::Save, "Specify the pre-allocated number of entities in the pool.");
ezCVarFloat cvar_AudioSystemFPS("Audio.FPS", 60, ezCVarFlags::Save, "The maximum number of frames to process within one second in the audio system.");

// clang-format off
EZ_IMPLEMENT_SINGLETON(ezAudioSystem);
// clang-format on

void ezAudioSystem::LoadConfiguration(const char* szFile)
{
}

void ezAudioSystem::SetOverridePlatform(const char* szPlatform)
{
}

void ezAudioSystem::UpdateSound()
{
  // TODO: Seems that this function is not called from the thread that has started the audio system. It is a bit obvious, but have to check later if this assert is relevant...
  // EZ_ASSERT_ALWAYS(gMainThreadId == ezThreadUtils::GetCurrentThreadID(), "AudioSystem::UpdateSound not called from main thread.");

  if (!m_bInitialized)
    return;

  // Process callbacks
  {
    ezAudioSystemRequestsQueue callbacks{};
    {
      EZ_LOCK(m_PendingRequestCallbacksMutex);
      callbacks.Swap(m_PendingRequestCallbacksQueue);
    }

    while (!callbacks.IsEmpty())
    {
      ezVariant callback(callbacks.PeekFront());

      CallRequestCallbackFunc func(callback);

      ezVariant::DispatchTo(func, callback.GetType());

      callbacks.PopFront();
    }
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  m_AudioTranslationLayer.DebugRender();
#endif
}

void ezAudioSystem::SetMasterChannelVolume(float volume)
{
}

float ezAudioSystem::GetMasterChannelVolume() const
{
  return 0.0f;
}

void ezAudioSystem::SetMasterChannelMute(bool mute)
{
}

bool ezAudioSystem::GetMasterChannelMute() const
{
  return false;
}

void ezAudioSystem::SetMasterChannelPaused(bool paused)
{
}

bool ezAudioSystem::GetMasterChannelPaused() const
{
  return false;
}

void ezAudioSystem::SetSoundGroupVolume(const char* szVcaGroupGuid, float volume)
{
}

float ezAudioSystem::GetSoundGroupVolume(const char* szVcaGroupGuid) const
{
  return 0.0f;
}

ezUInt8 ezAudioSystem::GetNumListeners()
{
  return 0;
}

void ezAudioSystem::SetListenerOverrideMode(bool enabled)
{
}

void ezAudioSystem::SetListener(ezInt32 iIndex, const ezVec3& vPosition, const ezVec3& vForward, const ezVec3& vUp, const ezVec3& vVelocity)
{
}

ezAudioSystem::ezAudioSystem()
  : m_SingletonRegistrar(this)
  , m_AudioTranslationLayer()
  , m_MainEvent()
  , m_ProcessingEvent()
  , m_bInitialized(false)
{
  gMainThreadId = ezThreadUtils::GetCurrentThreadID();
}

ezAudioSystem::~ezAudioSystem()
{
  EZ_ASSERT_DEV(!m_bInitialized, "You should shutdown the AudioSystem before to call the dtor.");
}

bool ezAudioSystem::Startup()
{
  EZ_ASSERT_ALWAYS(gMainThreadId == ezThreadUtils::GetCurrentThreadID(), "AudioSystem::Startup not called from main thread.");

  if (m_bInitialized)
    return true;

  const auto* pAudioMiddleware = ezSingletonRegistry::GetSingletonInstance<ezAudioMiddleware>();
  if (pAudioMiddleware == nullptr)
  {
    ezLog::Error("Could not find an active audio middleware. The AudioSystem will not start.");
    return false;
  }

  StopAudioThread();

  if (m_MainEvent.Create().Succeeded() && m_ProcessingEvent.Create().Succeeded() && m_AudioTranslationLayer.Startup().Succeeded())
  {
    // Start audio thread
    StartAudioThread();
    m_bInitialized = true;
  }

  return m_bInitialized;
}

void ezAudioSystem::Shutdown()
{
  EZ_ASSERT_ALWAYS(gMainThreadId == ezThreadUtils::GetCurrentThreadID(), "AudioSystem::Shutdown not called from main thread.");

  ezAudioSystemRequestShutdown shutdownRequest;
  SendRequestSync(shutdownRequest);

  StopAudioThread();
  m_AudioTranslationLayer.Shutdown();
  m_bInitialized = false;
}

void ezAudioSystem::SendRequest(ezVariant&& request)
{
  EZ_LOCK(m_PendingRequestsMutex);
  m_PendingRequestsQueue.PushBack(std::move(request));
}

void ezAudioSystem::SendRequests(ezAudioSystemRequestsQueue& requests)
{
  EZ_LOCK(m_PendingRequestsMutex);
  for (auto& request : requests)
  {
    m_PendingRequestsQueue.PushBack(std::move(request));
  }
}

void ezAudioSystem::SendRequestSync(ezVariant&& request)
{
  {
    EZ_LOCK(m_BlockingRequestsMutex);
    m_BlockingRequestsQueue.PushBack(std::move(request));
  }

  m_ProcessingEvent.ReturnToken();
  m_MainEvent.AcquireToken();
}

void ezAudioSystem::QueueRequestCallback(ezVariant&& request)
{
  EZ_LOCK(m_PendingRequestCallbacksMutex);
  m_PendingRequestCallbacksQueue.PushBack(std::move(request));
}

void ezAudioSystem::RegisterTrigger(const char* szTriggerName, const char* szControlFile)
{
  if (!m_bInitialized)
    return;

  ezFileReader file;
  if (!file.Open(szControlFile, 256).Succeeded())
  {
    ezLog::Error("Unable to register a trigger in the audio system: Could not open trigger control file '{0}'", szControlFile);
    return;
  }

  RegisterTrigger(szTriggerName, &file);
}

void ezAudioSystem::RegisterTrigger(const char* szTriggerName, ezStreamReader* pStreamReader)
{
  auto* pAudioMiddleware = ezSingletonRegistry::GetSingletonInstance<ezAudioMiddleware>();
  if (pAudioMiddleware == nullptr)
  {
    ezLog::Error("Unable to register a trigger in the audio system: No audio middleware currently running.");
    return;
  }

  ezEnum<ezAudioSystemControlType> type;
  *pStreamReader >> type;

  if (type != ezAudioSystemControlType::Trigger)
  {
    ezLog::Error("Unable to register a trigger in the audio system: The control have an invalid file.");
    return;
  }

  ezAudioSystemTriggerData* pTriggerData = pAudioMiddleware->DeserializeTriggerEntry(pStreamReader);
  if (pTriggerData == nullptr)
  {
    ezLog::Error("Unable to register a trigger in the audio system: Could not deserialize trigger control.");
    return;
  }

  const ezUInt32 uiTriggerId = ezHashHelper<const char*>::Hash(szTriggerName);
  m_AudioTranslationLayer.RegisterTrigger(uiTriggerId, pTriggerData);
}

ezAudioSystemDataID ezAudioSystem::GetTriggerId(const char* szTriggerName) const
{
  return m_AudioTranslationLayer.GetTriggerId(szTriggerName);
}

ezAudioSystemDataID ezAudioSystem::GetRtpcId(const char* szRtpcName) const
{
  return 0;
}

ezAudioSystemDataID ezAudioSystem::GetSwitchId(const char* szSwitchName) const
{
  return 0;
}

ezAudioSystemDataID ezAudioSystem::GetSwitchStateId(ezAudioSystemDataID uiSwitchId, const char* szSwitchStateName) const
{
  return 0;
}

ezAudioSystemDataID ezAudioSystem::GetEnvironmentId(const char* szEnvironmentName) const
{
  return 0;
}

ezAudioSystemDataID ezAudioSystem::GetBankId(const char* szBankName) const
{
  return 0;
}

const char* ezAudioSystem::GetControlsPath() const
{
  return "noop";
}

void ezAudioSystem::UpdateControlsPath()
{
}

void ezAudioSystem::UpdateInternal()
{
  EZ_ASSERT_ALWAYS(m_pAudioThread->GetThreadID() == ezThreadUtils::GetCurrentThreadID(), "AudioSystem::UpdateInternal not called from audio thread.");

  EZ_PROFILE_SCOPE("AudioSystem");

  if (!m_bInitialized)
    return;

  const ezTime startTime = ezTime::Now();

  // Process a single synchronous request, if any
  bool handleBlockingRequest = false;
  ezVariant blockingRequest;

  {
    EZ_LOCK(m_BlockingRequestsMutex);
    handleBlockingRequest = !m_BlockingRequestsQueue.IsEmpty();
    if (handleBlockingRequest)
    {
      blockingRequest = std::move(m_BlockingRequestsQueue.PeekFront());
      m_BlockingRequestsQueue.PopFront();
    }
  }

  if (handleBlockingRequest)
  {
    m_AudioTranslationLayer.ProcessRequest(std::move(blockingRequest));
    m_MainEvent.ReturnToken();
  }

  if (!handleBlockingRequest)
  {
    // Normal request processing: lock and swap the pending requests queue
    // so that the queue can be opened for new requests while the current set
    // of requests can be processed.
    ezAudioSystemRequestsQueue requestsToProcess{};
    {
      EZ_LOCK(m_PendingRequestsMutex);
      requestsToProcess.Swap(m_PendingRequestsQueue);
    }

    while (!requestsToProcess.IsEmpty())
    {
      // Normal request...
      ezVariant& request(requestsToProcess.PeekFront());
      m_AudioTranslationLayer.ProcessRequest(std::move(request));
      requestsToProcess.PopFront();
    }
  }

  m_AudioTranslationLayer.Update();

  if (!handleBlockingRequest)
  {
    const ezTime endTime = ezTime::Now(); // stamp the end time

    const ezTime elapsedTime = endTime - startTime;
    const ezTime frameTime = ezTime::Seconds(1.0f / cvar_AudioSystemFPS);

    if (frameTime > elapsedTime)
    {
      const ezTime timeOut = frameTime - elapsedTime;
      EZ_PROFILE_SCOPE_WITH_TIMEOUT("AudioSystem", timeOut);
      m_ProcessingEvent.TryAcquireToken(timeOut).IgnoreResult();
    }
  }
}

void ezAudioSystem::StartAudioThread()
{
  StopAudioThread();

  if (m_pAudioThread == nullptr)
  {
    m_pAudioThread = EZ_AUDIOSYSTEM_NEW(ezAudioThread);
    m_pAudioThread->m_pAudioSystem = this;
    m_pAudioThread->Start();

    ezLog::Success("Audio thread started.");
  }
}

void ezAudioSystem::StopAudioThread()
{
  if (m_pAudioThread != nullptr)
  {
    m_pAudioThread->m_bKeepRunning = false;
    m_pAudioThread->Join();

    EZ_AUDIOSYSTEM_DELETE(m_pAudioThread);

    ezLog::Success("Audio thread stopped.");
  }
}

void ezAudioSystem::GameApplicationEventHandler(const ezGameApplicationExecutionEvent& e)
{
  if (e.m_Type == ezGameApplicationExecutionEvent::Type::AfterWorldUpdates)
  {
    ezAudioSystem::GetSingleton()->UpdateSound();
  }
}

EZ_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_AudioSystem);
