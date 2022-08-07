#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Components/AudioTriggerComponent.h>
#include <AudioSystemPlugin/Core/AudioSystem.h>
#include <AudioSystemPlugin/Core/AudioSystemRequests.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

constexpr ezTypeVersion kVersion_AudioTriggerComponent = 1;

/// \brief The last used event ID for all audio trigger components.
static ezAudioSystemDataID s_uiLastEventId = 0;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezAudioTriggerComponent, kVersion_AudioTriggerComponent, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("PlayTrigger", m_sPlayTrigger),
    EZ_MEMBER_PROPERTY("StopTrigger", m_sStopTrigger),
    EZ_ENUM_MEMBER_PROPERTY("SoundObstructionType", ezAudioSystemSoundObstructionType, m_eObstructionType),
    EZ_MEMBER_PROPERTY("LoadOnInit", m_bLoadOnInit),
    EZ_MEMBER_PROPERTY("PlayOnActivate", m_bPlayOnActivate),

    EZ_ACCESSOR_PROPERTY_READ_ONLY("IsLoading", IsLoading)->AddAttributes(new ezHiddenAttribute()),
    EZ_ACCESSOR_PROPERTY_READ_ONLY("IsReady", IsReady)->AddAttributes(new ezHiddenAttribute()),
    EZ_ACCESSOR_PROPERTY_READ_ONLY("IsStarting", IsStarting)->AddAttributes(new ezHiddenAttribute()),
    EZ_ACCESSOR_PROPERTY_READ_ONLY("IsPlaying", IsPlaying)->AddAttributes(new ezHiddenAttribute()),
    EZ_ACCESSOR_PROPERTY_READ_ONLY("IsStopping", IsStopping)->AddAttributes(new ezHiddenAttribute()),
    EZ_ACCESSOR_PROPERTY_READ_ONLY("IsStopped", IsStopped)->AddAttributes(new ezHiddenAttribute()),
    EZ_ACCESSOR_PROPERTY_READ_ONLY("IsUnloading", IsUnloading)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(Play, In, "Sync"),
    EZ_SCRIPT_FUNCTION_PROPERTY(Stop, In, "Sync"),
    EZ_SCRIPT_FUNCTION_PROPERTY(SetPlayTrigger, In, "PlayTrigger"),
    EZ_SCRIPT_FUNCTION_PROPERTY(SetStopTrigger, In, "StopTrigger"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetPlayTrigger),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetStopTrigger),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezAudioTriggerComponent::ezAudioTriggerComponent()
  : ezAudioSystemComponent()
  , m_eState(ezAudioSystemTriggerState::Invalid)
  , m_uiPlayEventId(++s_uiLastEventId)
  , m_uiStopEventId(++s_uiLastEventId)
  , m_sPlayTrigger()
  , m_sStopTrigger()
  , m_eObstructionType(ezAudioSystemSoundObstructionType::SingleRay)
  , m_bLoadOnInit(false)
  , m_bPlayOnActivate(false)
{
}

ezAudioTriggerComponent::~ezAudioTriggerComponent() = default;

void ezAudioTriggerComponent::SetPlayTrigger(ezString sName)
{
  if (sName == m_sPlayTrigger)
    return;

  if (m_eState == ezAudioSystemTriggerState::Playing)
  {
    Stop();
  }

  m_sPlayTrigger = std::move(sName);
}

const ezString& ezAudioTriggerComponent::GetPlayTrigger() const
{
  return m_sPlayTrigger;
}

void ezAudioTriggerComponent::SetStopTrigger(ezString sName)
{
  if (sName == m_sStopTrigger)
    return;

  m_sStopTrigger = std::move(sName);
}

const ezString& ezAudioTriggerComponent::GetStopTrigger() const
{
  return m_sStopTrigger;
}

void ezAudioTriggerComponent::Play(bool bSync)
{
  if (m_sPlayTrigger.IsEmpty())
    return;

  if (IsPlaying() || IsStarting())
    return;

  if (!m_bPlayTriggerLoaded)
    LoadPlayTrigger(true); // Need to be sync if data was not loaded before

  m_eState = ezAudioSystemTriggerState::Starting;

  ezAudioSystemRequestActivateTrigger request;

  request.m_uiEntityId = GetOwner()->GetHandle().GetInternalID().m_Data;
  request.m_uiObjectId = ezAudioSystem::GetSingleton()->GetTriggerId(m_sPlayTrigger);
  request.m_uiEventId = m_uiPlayEventId;

  request.m_Callback = [this](const ezAudioSystemRequestActivateTrigger& e)
  {
    if (e.m_eStatus.Succeeded())
      m_eState = ezAudioSystemTriggerState::Playing;
    else
      m_eState = ezAudioSystemTriggerState::Invalid;
  };

  ezVariant v(request);

  if (bSync)
  {
    ezAudioSystem::GetSingleton()->SendRequestSync(std::move(v));
  }
  else
  {
    ezAudioSystem::GetSingleton()->SendRequest(std::move(v));
  }
}

void ezAudioTriggerComponent::Stop(bool bSync)
{
  m_eState = ezAudioSystemTriggerState::Stopping;

  if (m_sStopTrigger.IsEmpty())
  {
    ezAudioSystemRequestStopEvent request;

    request.m_uiEntityId = GetOwner()->GetHandle().GetInternalID().m_Data;
    request.m_uiTriggerId = ezAudioSystem::GetSingleton()->GetTriggerId(m_sPlayTrigger);
    request.m_uiObjectId = m_uiPlayEventId;

    request.m_Callback = [this](const ezAudioSystemRequestStopEvent& e)
    {
      if (e.m_eStatus.Succeeded())
        m_eState = ezAudioSystemTriggerState::Stopped;
      else
        m_eState = ezAudioSystemTriggerState::Invalid;
    };

    ezVariant v(request);


    if (bSync)
    {
      ezAudioSystem::GetSingleton()->SendRequestSync(std::move(v));
    }
    else
    {
      ezAudioSystem::GetSingleton()->SendRequest(std::move(v));
    }
  }
  else
  {
    if (!m_bStopTriggerLoaded)
      LoadStopTrigger(true); // Need to be sync if data was not loaded before

    ezAudioSystemRequestActivateTrigger request;

    request.m_uiEntityId = GetOwner()->GetHandle().GetInternalID().m_Data;
    request.m_uiObjectId = ezAudioSystem::GetSingleton()->GetTriggerId(m_sStopTrigger);
    request.m_uiEventId = m_uiStopEventId;

    request.m_Callback = [this](const ezAudioSystemRequestActivateTrigger& e)
    {
      if (e.m_eStatus.Succeeded())
        m_eState = ezAudioSystemTriggerState::Stopped;
      else
        m_eState = ezAudioSystemTriggerState::Invalid;
    };

    ezVariant v(request);

    ezAudioSystem::GetSingleton()->SendRequest(std::move(v));
  }
}

const ezEnum<ezAudioSystemTriggerState>& ezAudioTriggerComponent::GetState() const
{
  return m_eState;
}

bool ezAudioTriggerComponent::IsLoading() const
{
  return m_eState == ezAudioSystemTriggerState::Loading;
}

bool ezAudioTriggerComponent::IsReady() const
{
  return m_eState == ezAudioSystemTriggerState::Ready;
}

bool ezAudioTriggerComponent::IsStarting() const
{
  return m_eState == ezAudioSystemTriggerState::Starting;
}

bool ezAudioTriggerComponent::IsPlaying() const
{
  return m_eState == ezAudioSystemTriggerState::Playing;
}

bool ezAudioTriggerComponent::IsStopping() const
{
  return m_eState == ezAudioSystemTriggerState::Stopping;
}

bool ezAudioTriggerComponent::IsStopped() const
{
  return m_eState == ezAudioSystemTriggerState::Stopped;
}

bool ezAudioTriggerComponent::IsUnloading() const
{
  return m_eState == ezAudioSystemTriggerState::Unloading;
}

void ezAudioTriggerComponent::Initialize()
{
  SUPER::Initialize();

  if (m_bLoadOnInit)
  {
    LoadPlayTrigger(false);
    LoadStopTrigger(false);
  }
}

void ezAudioTriggerComponent::OnActivated()
{
  SUPER::OnActivated();

  if (m_bPlayOnActivate && !m_bHasPlayedOnActivate)
  {
    Play();
    m_bHasPlayedOnActivate = true;
  }
}

void ezAudioTriggerComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();
}

void ezAudioTriggerComponent::OnDeactivated()
{
  if (IsPlaying())
    Stop();

  m_bHasPlayedOnActivate = false;
  SUPER::OnDeactivated();
}

void ezAudioTriggerComponent::Deinitialize()
{
  if (m_bStopTriggerLoaded)
    UnloadStopTrigger();

  if (m_bPlayTriggerLoaded)
    UnloadPlayTrigger();

  SUPER::Deinitialize();
}

void ezAudioTriggerComponent::Update()
{
  // noop
}

void ezAudioTriggerComponent::LoadPlayTrigger(bool bSync)
{
  if (m_sPlayTrigger.IsEmpty())
    return;

  if (m_bPlayTriggerLoaded)
  {
    m_eState = ezAudioSystemTriggerState::Ready;
    return;
  }

  m_eState = ezAudioSystemTriggerState::Loading;

  ezAudioSystemRequestLoadTrigger request;

  request.m_uiEntityId = GetOwner()->GetHandle().GetInternalID().m_Data;
  request.m_uiObjectId = ezAudioSystem::GetSingleton()->GetTriggerId(m_sPlayTrigger);
  request.m_uiEventId = m_uiPlayEventId;

  request.m_Callback = [this](const ezAudioSystemRequestLoadTrigger& m)
  {
    if (m.m_eStatus.Failed())
    {
      m_eState = ezAudioSystemTriggerState::Invalid;
      return;
    }

    m_bPlayTriggerLoaded = true;
    m_eState = ezAudioSystemTriggerState::Ready;
    ezLog::Info("[AudioSystem] Loaded Play Trigger '{0}'.", m_sPlayTrigger);
  };

  ezVariant v(request);

  if (bSync)
  {
    ezAudioSystem::GetSingleton()->SendRequestSync(std::move(v));
  }
  else
  {
    ezAudioSystem::GetSingleton()->SendRequest(std::move(v));
  }
}

void ezAudioTriggerComponent::LoadStopTrigger(bool bSync)
{
  if (m_sStopTrigger.IsEmpty())
    return;

  if (m_bStopTriggerLoaded)
    return;

  ezAudioSystemRequestLoadTrigger request;

  request.m_uiEntityId = GetOwner()->GetHandle().GetInternalID().m_Data;
  request.m_uiObjectId = ezAudioSystem::GetSingleton()->GetTriggerId(m_sStopTrigger);
  request.m_uiEventId = m_uiStopEventId;

  request.m_Callback = [this](const ezAudioSystemRequestLoadTrigger& m)
  {
    if (m.m_eStatus.Failed())
      return;

    m_bStopTriggerLoaded = true;
    ezLog::Info("[AudioSystem] Loaded Stop Trigger '{0}'.", m_sStopTrigger);
  };

  ezVariant v(request);

  if (bSync)
  {
    ezAudioSystem::GetSingleton()->SendRequestSync(std::move(v));
  }
  else
  {
    ezAudioSystem::GetSingleton()->SendRequest(std::move(v));
  }
}

void ezAudioTriggerComponent::UnloadPlayTrigger()
{
  if (!m_bPlayTriggerLoaded)
    return;

  m_eState = ezAudioSystemTriggerState::Unloading;

  ezAudioSystemRequestUnloadTrigger request;

  request.m_uiEntityId = GetOwner()->GetHandle().GetInternalID().m_Data;
  request.m_uiObjectId = ezAudioSystem::GetSingleton()->GetTriggerId(m_sPlayTrigger);

  request.m_Callback = [this](const ezAudioSystemRequestUnloadTrigger& m)
  {
    if (m.m_eStatus.Failed())
    {
      m_eState = ezAudioSystemTriggerState::Invalid;
      return;
    }

    m_bPlayTriggerLoaded = false;
    m_eState = ezAudioSystemTriggerState::Invalid;
    ezLog::Info("[AudioSystem] Unloaded Play Trigger '{0}'.", m_sPlayTrigger);
  };

  ezVariant v(request);

  ezAudioSystem::GetSingleton()->SendRequest(std::move(v));
}

void ezAudioTriggerComponent::UnloadStopTrigger()
{
  if (!m_bStopTriggerLoaded)
    return;

  ezAudioSystemRequestUnloadTrigger request;

  request.m_uiEntityId = GetOwner()->GetHandle().GetInternalID().m_Data;
  request.m_uiObjectId = ezAudioSystem::GetSingleton()->GetTriggerId(m_sStopTrigger);

  request.m_Callback = [this](const ezAudioSystemRequestUnloadTrigger& m)
  {
    if (m.m_eStatus.Failed())
      return;

    m_bStopTriggerLoaded = false;
    ezLog::Info("[AudioSystem] Unloaded Stop Trigger '{0}'.", m_sStopTrigger);
  };

  ezVariant v(request);

  ezAudioSystem::GetSingleton()->SendRequest(std::move(v));
}

void ezAudioTriggerComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s.WriteVersion(kVersion_AudioTriggerComponent);

  s << m_sPlayTrigger;
  s << m_sStopTrigger;
  s << m_eObstructionType;
  s << m_bLoadOnInit;
  s << m_bPlayOnActivate;
}

void ezAudioTriggerComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);

  auto& s = stream.GetStream();

  s.ReadVersion(kVersion_AudioTriggerComponent);

  s >> m_sPlayTrigger;
  s >> m_sStopTrigger;
  s >> m_eObstructionType;
  s >> m_bLoadOnInit;
  s >> m_bPlayOnActivate;
}

EZ_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_Components_AudioTriggerComponent);
