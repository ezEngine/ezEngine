#include <PCH.h>
#include <FmodPlugin/Components/FmodEventComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <FmodPlugin/FmodSingleton.h>
#include <FmodPlugin/Resources/FmodSoundEventResource.h>
#include <Core/ResourceManager/ResourceBase.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <FmodPlugin/FmodIncludes.h>

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezFmodEventComponent_RestartSoundMsg);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFmodEventComponent_RestartSoundMsg, 1, ezRTTIDefaultAllocator<ezFmodEventComponent_RestartSoundMsg>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("OneShot", m_bOneShotInstance)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezFmodEventComponent_StopSoundMsg);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFmodEventComponent_StopSoundMsg, 1, ezRTTIDefaultAllocator<ezFmodEventComponent_StopSoundMsg>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Immediate", m_bImmediate),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezFmodSoundFinishedEventMessage);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFmodSoundFinishedEventMessage, 1, ezRTTIDefaultAllocator<ezFmodSoundFinishedEventMessage>)
EZ_END_DYNAMIC_REFLECTED_TYPE

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezFmodEventComponent_SoundCueMsg);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFmodEventComponent_SoundCueMsg, 1, ezRTTIDefaultAllocator<ezFmodEventComponent_SoundCueMsg>)
EZ_END_DYNAMIC_REFLECTED_TYPE

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_COMPONENT_TYPE(ezFmodEventComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Paused", GetPaused, SetPaused),
    EZ_ACCESSOR_PROPERTY("Volume", GetVolume, SetVolume)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_ACCESSOR_PROPERTY("Pitch", GetPitch, SetPitch)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.01f, 100.0f)),
    EZ_ACCESSOR_PROPERTY("SoundEvent", GetSoundEventFile, SetSoundEventFile)->AddAttributes(new ezAssetBrowserAttribute("Sound Event")),
    EZ_ENUM_MEMBER_PROPERTY("OnFinishedAction", ezOnComponentFinishedAction, m_OnFinishedAction),
    //EZ_FUNCTION_PROPERTY("Preview", StartOneShot), // This doesn't seem to be working anymore, and I cannot find code for exposing it in the UI either
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezFmodEventComponent_RestartSoundMsg, RestartSound),
    EZ_MESSAGE_HANDLER(ezFmodEventComponent_StopSoundMsg, StopSound),
    EZ_MESSAGE_HANDLER(ezFmodEventComponent_SoundCueMsg, SoundCue),
  }
  EZ_END_MESSAGEHANDLERS
    EZ_BEGIN_MESSAGESENDERS
  {
    EZ_MESSAGE_SENDER(m_SoundFinishedEventSender),
  }
  EZ_END_MESSAGESENDERS
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezFmodEventComponent::ezFmodEventComponent()
{
  m_pEventDesc = nullptr;
  m_pEventInstance = nullptr;
  m_bPaused = false;
  m_fPitch = 1.0f;
  m_fVolume = 1.0f;
}

ezFmodEventComponent::~ezFmodEventComponent()
{

}

void ezFmodEventComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_bPaused;
  s << m_fPitch;
  s << m_fVolume;

  s << m_hSoundEvent;

  ezOnComponentFinishedAction::StorageType type = m_OnFinishedAction;
  s << type;

  ezInt32 iTimelinePosition = -1;

  if (m_pEventInstance)
  {
    FMOD_STUDIO_PLAYBACK_STATE state;
    m_pEventInstance->getPlaybackState(&state);

    if (state == FMOD_STUDIO_PLAYBACK_STARTING ||
        state == FMOD_STUDIO_PLAYBACK_PLAYING ||
        state == FMOD_STUDIO_PLAYBACK_SUSTAINING)
    {
      m_pEventInstance->getTimelinePosition(&iTimelinePosition);
    }
  }

  s << iTimelinePosition;
}

void ezFmodEventComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_bPaused;
  s >> m_fPitch;
  s >> m_fVolume;
  s >> m_hSoundEvent;

  ezOnComponentFinishedAction::StorageType type;
  s >> type;
  m_OnFinishedAction = (ezOnComponentFinishedAction::Enum) type;

  s >> m_iTimelinePosition;
}

void ezFmodEventComponent::SetPaused(bool b)
{
  if (b == m_bPaused)
    return;

  m_bPaused = b;

  if (m_pEventInstance != nullptr)
  {
    EZ_FMOD_ASSERT(m_pEventInstance->setPaused(m_bPaused));
  }
  else if (!m_bPaused)
  {
    Restart();
  }
}

void ezFmodEventComponent::SetPitch(float f)
{
  if (f == m_fPitch)
    return;

  m_fPitch = f;

  if (m_pEventInstance != nullptr)
  {
    /// \todo Global pitch might better be a bus setting
    EZ_FMOD_ASSERT(m_pEventInstance->setPitch(m_fPitch * (float)GetWorld()->GetClock().GetSpeed()));
  }
}

void ezFmodEventComponent::SetVolume(float f)
{
  if (f == m_fVolume)
    return;

  m_fVolume = f;

  if (m_pEventInstance != nullptr)
  {
    EZ_FMOD_ASSERT(m_pEventInstance->setVolume(m_fVolume));
  }
}

void ezFmodEventComponent::SetSoundEventFile(const char* szFile)
{
  ezFmodSoundEventResourceHandle hRes;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hRes = ezResourceManager::LoadResource<ezFmodSoundEventResource>(szFile);
  }

  SetSoundEvent(hRes);
}

const char* ezFmodEventComponent::GetSoundEventFile() const
{
  if (!m_hSoundEvent.IsValid())
    return "";

  return m_hSoundEvent.GetResourceID();
}

void ezFmodEventComponent::SetSoundEvent(const ezFmodSoundEventResourceHandle& hSoundEvent)
{
  if (m_pEventInstance)
  {
    ezFmodEventComponent_StopSoundMsg msg;
    msg.m_bImmediate = false;
    StopSound(msg);

    EZ_FMOD_ASSERT(m_pEventInstance->release());
    m_pEventInstance = nullptr;
    m_iTimelinePosition = -1;
  }

  m_hSoundEvent = hSoundEvent;
}

void ezFmodEventComponent::OnDeactivated()
{
  if (m_pEventInstance != nullptr)
  {
    // we could expose this decision as a property 'AlwaysFinish' or so
    bool bLetFinish = true;

    FMOD::Studio::EventDescription* pDesc = nullptr;
    m_pEventInstance->getDescription(&pDesc);
    pDesc->isOneshot(&bLetFinish);

    if (!bLetFinish)
    {
      EZ_FMOD_ASSERT(m_pEventInstance->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT));
    }

    EZ_FMOD_ASSERT(m_pEventInstance->release());
    m_pEventInstance = nullptr;
    m_iTimelinePosition = -1;
  }
}

void ezFmodEventComponent::OnSimulationStarted()
{
  if (!m_bPaused)
  {
    Restart();
  }
}

void ezFmodEventComponent::Restart()
{
  // reset this, using the value is handled outside this function
  m_iTimelinePosition = -1;

  if (!m_hSoundEvent.IsValid() || !IsActiveAndSimulating())
    return;

  if (m_pEventInstance == nullptr)
  {
    ezResourceLock<ezFmodSoundEventResource> pEvent(m_hSoundEvent, ezResourceAcquireMode::NoFallback);

    if (pEvent->IsMissingResource())
      return;

    m_pEventInstance = pEvent->CreateInstance();
    if (m_pEventInstance == nullptr)
    {
      ezLog::Debug("Cannot start sound event, instance could not be created.");
      return;
    }
  }

  m_bPaused = false;

  SetParameters3d(m_pEventInstance);

  EZ_FMOD_ASSERT(m_pEventInstance->setPaused(false));
  EZ_FMOD_ASSERT(m_pEventInstance->setVolume(m_fVolume));

  EZ_FMOD_ASSERT(m_pEventInstance->start());
}

void ezFmodEventComponent::StartOneShot()
{
  if (!m_hSoundEvent.IsValid())
    return;

  ezResourceLock<ezFmodSoundEventResource> pEvent(m_hSoundEvent, ezResourceAcquireMode::NoFallback);

  if (pEvent->IsMissingResource())
  {
    ezLog::Debug("Cannot start one-shot sound event, resource is missing.");
    return;
  }

  if (pEvent->GetDescriptor() == nullptr)
  {
    ezLog::Debug("Cannot start one-shot sound event, descriptor is null.");
    return;
  }

  bool bIsOneShot = false;
  pEvent->GetDescriptor()->isOneshot(&bIsOneShot);

  // do not start sounds that will not terminate
  if (!bIsOneShot)
  {
    ezLog::Warning("ezFmodEventComponent::StartOneShot: Request ignored, because sound event '{0}' ('{0}') is not a one-shot event.", pEvent->GetResourceID(), pEvent->GetResourceDescription());
    return;
  }

  FMOD::Studio::EventInstance* pEventInstance = pEvent->CreateInstance();

  if (pEventInstance == nullptr)
  {
    ezLog::Debug("Cannot start one-shot sound event, instance could not be created.");
    return;
  }

  SetParameters3d(pEventInstance);

  EZ_FMOD_ASSERT(pEventInstance->setVolume(m_fVolume));

  EZ_FMOD_ASSERT(pEventInstance->start());
  EZ_FMOD_ASSERT(pEventInstance->release());
}

void ezFmodEventComponent::RestartSound(ezFmodEventComponent_RestartSoundMsg& msg)
{
  if (msg.m_bOneShotInstance)
    StartOneShot();
  else
    Restart();
}
void ezFmodEventComponent::StopSound(ezFmodEventComponent_StopSoundMsg& msg)
{
  if (m_pEventInstance != nullptr)
  {
    m_iTimelinePosition = -1;
    EZ_FMOD_ASSERT(m_pEventInstance->stop(msg.m_bImmediate ? FMOD_STUDIO_STOP_IMMEDIATE : FMOD_STUDIO_STOP_ALLOWFADEOUT));
  }
}

void ezFmodEventComponent::SoundCue(ezFmodEventComponent_SoundCueMsg& msg)
{
  if (m_pEventInstance != nullptr && m_pEventInstance->isValid())
  {
    EZ_FMOD_ASSERT(m_pEventInstance->triggerCue());
  }
}

ezInt32 ezFmodEventComponent::FindParameter(const char* szName) const
{
  if (!m_hSoundEvent.IsValid())
    return -1;

  ezResourceLock<ezFmodSoundEventResource> pEvent(m_hSoundEvent, ezResourceAcquireMode::NoFallback);

  FMOD::Studio::EventDescription* pEventDesc = pEvent->GetDescriptor();
  if (pEventDesc == nullptr || !pEventDesc->isValid())
    return -1;

  FMOD_STUDIO_PARAMETER_DESCRIPTION paramDesc;
  if (pEventDesc->getParameter(szName, &paramDesc) != FMOD_OK)
    return -1;

  return paramDesc.index;
}

void ezFmodEventComponent::SetParameter(ezInt32 iParamIndex, float fValue)
{
  if (m_pEventInstance == nullptr || iParamIndex < 0 || !m_pEventInstance->isValid())
    return;

  m_pEventInstance->setParameterValueByIndex(iParamIndex, fValue);
}

float ezFmodEventComponent::GetParameter(ezInt32 iParamIndex) const
{
  if (m_pEventInstance == nullptr || iParamIndex < 0 || !m_pEventInstance->isValid())
    return 0.0f;

  float value = 0;
  m_pEventInstance->getParameterValueByIndex(iParamIndex, &value, nullptr);
  return value;
}

void ezFmodEventComponent::Update()
{
  if (m_pEventInstance)
  {
    if (!m_pEventInstance->isValid())
    {
      InvalidateResource(false);
      return;
    }

    SetParameters3d(m_pEventInstance);

    FMOD_STUDIO_PLAYBACK_STATE state;
    EZ_FMOD_ASSERT(m_pEventInstance->getPlaybackState(&state));

    if (state == FMOD_STUDIO_PLAYBACK_STOPPED)
    {
      m_iTimelinePosition = -1;

      ezFmodSoundFinishedEventMessage msg;
      m_SoundFinishedEventSender.SendMessage(msg, this, GetOwner());

      if (m_OnFinishedAction == ezOnComponentFinishedAction::DeleteGameObject)
      {
        GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
      }
      else if (m_OnFinishedAction == ezOnComponentFinishedAction::DeleteComponent)
      {
        GetOwningManager()->DeleteComponent(GetHandle());
      }
    }
  }
  else if (m_iTimelinePosition >= 0 && !m_bPaused)
  {
    // Restore the event to the last playback position
    const ezInt32 iTimelinePos = m_iTimelinePosition;

    Restart(); // will reset m_iTimelinePosition, that's why it is copied first

    if (m_pEventInstance)
    {
      // we need to store this value again, because it is very likely during a resource reload that we
      // run into the InvalidateResource(false) case at the top of this function, right after this
      // so we can restore the timeline position again
      m_iTimelinePosition = iTimelinePos;
      m_pEventInstance->setTimelinePosition(iTimelinePos);
    }
  }
}

void ezFmodEventComponent::SetParameters3d(FMOD::Studio::EventInstance* pEventInstance)
{
  const auto pos = GetOwner()->GetGlobalPosition();
  const auto vel = GetOwner()->GetVelocity();
  const auto fwd = (GetOwner()->GetGlobalRotation() * ezVec3(1, 0, 0)).GetNormalized();
  const auto up = (GetOwner()->GetGlobalRotation() * ezVec3(0, 0, 1)).GetNormalized();

  FMOD_3D_ATTRIBUTES attr;
  attr.position.x = pos.x;
  attr.position.y = pos.y;
  attr.position.z = pos.z;
  attr.forward.x = fwd.x;
  attr.forward.y = fwd.y;
  attr.forward.z = fwd.z;
  attr.up.x = up.x;
  attr.up.y = up.y;
  attr.up.z = up.z;
  attr.velocity.x = vel.x;
  attr.velocity.y = vel.y;
  attr.velocity.z = vel.z;

  // have to update pitch every time, in case the clock speed changes
  EZ_FMOD_ASSERT(pEventInstance->setPitch(m_fPitch * (float)GetWorld()->GetClock().GetSpeed()));
  EZ_FMOD_ASSERT(pEventInstance->set3DAttributes(&attr));
}

void ezFmodEventComponent::InvalidateResource(bool bTryToRestore)
{
  if (m_pEventInstance)
  {
    if (bTryToRestore)
    {
      FMOD_STUDIO_PLAYBACK_STATE state;
      m_pEventInstance->getPlaybackState(&state);

      if (state == FMOD_STUDIO_PLAYBACK_STARTING ||
          state == FMOD_STUDIO_PLAYBACK_PLAYING ||
          state == FMOD_STUDIO_PLAYBACK_SUSTAINING)
      {
        m_pEventInstance->getTimelinePosition(&m_iTimelinePosition);
      }
      else
      {
        // only reset when bTryToRestore is true, otherwise keep the old value
        m_iTimelinePosition = -1;
      }
    }

    // pointer is no longer valid!
    m_pEventInstance = nullptr;

    //ezLog::Debug("Fmod instance pointer has been invalidated.");
  }
}

//////////////////////////////////////////////////////////////////////////

ezFmodEventComponentManager::ezFmodEventComponentManager(ezWorld* pWorld)
  : ezComponentManagerSimple(pWorld)
{

}

void ezFmodEventComponentManager::Initialize()
{
  ezComponentManagerSimple<class ezFmodEventComponent, ezComponentUpdateType::WhenSimulating>::Initialize();

  ezResourceManager::s_ResourceEvents.AddEventHandler(ezMakeDelegate(&ezFmodEventComponentManager::ResourceEventHandler, this));
}

void ezFmodEventComponentManager::Deinitialize()
{
  ezResourceManager::s_ResourceEvents.RemoveEventHandler(ezMakeDelegate(&ezFmodEventComponentManager::ResourceEventHandler, this));

  ezComponentManagerSimple<class ezFmodEventComponent, ezComponentUpdateType::WhenSimulating>::Deinitialize();
}

void ezFmodEventComponentManager::ResourceEventHandler(const ezResourceEvent& e)
{
  if (e.m_EventType == ezResourceEventType::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<ezFmodSoundEventResource>())
  {
    ezFmodSoundEventResourceHandle hResource((ezFmodSoundEventResource*)(e.m_pResource));

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hSoundEvent == hResource)
      {
        it->InvalidateResource(true);
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_SetFmodEventParameter, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_SetFmodEventParameter>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Sound"),
    new ezTitleAttribute("Fmod Param '{Parameter}' = {Value}"),
  }
    EZ_END_ATTRIBUTES
    EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Parameter", GetParameterName, SetParameterName),
    // Execution Pins (Input)
    EZ_INPUT_EXECUTION_PIN("run", 0),
    // Execution Pins (Output)
    EZ_OUTPUT_EXECUTION_PIN("then", 0),
    // Data Pins (Input)
    EZ_INPUT_DATA_PIN("Component", 0, ezVisualScriptDataPinType::ComponentHandle),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("Value", 1, ezVisualScriptDataPinType::Number, m_fValue),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_SetFmodEventParameter::ezVisualScriptNode_SetFmodEventParameter() { }

void ezVisualScriptNode_SetFmodEventParameter::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_bInputValuesChanged && m_iParameterIndex != -2)
  {
    ezFmodEventComponent* pEvent = nullptr;
    if (!pInstance->GetWorld()->TryGetComponent(m_hComponent, pEvent))
      goto failure;

    // index not yet initialized
    if (m_iParameterIndex < 0)
    {
      m_iParameterIndex = pEvent->FindParameter(m_sParameterName.GetData());

      // parameter not found
      if (m_iParameterIndex < 0)
        goto failure;
    }

    pEvent->SetParameter(m_iParameterIndex, (float)m_fValue);
  }

  pInstance->ExecuteConnectedNodes(this, 0);
  return;

failure:
  ezLog::Warning("Script: Fmod Event Parameter '{0}' could not be found. Note that event parameters are not available for one-shot events.", m_sParameterName.GetString());

  m_iParameterIndex = -2; // make sure we don't try this again
  pInstance->ExecuteConnectedNodes(this, 0);
}

void* ezVisualScriptNode_SetFmodEventParameter::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
  case 0:
    return &m_hComponent;

  case 1:
    return &m_fValue;
  }

  return nullptr;
}




EZ_STATICLINK_FILE(FmodPlugin, FmodPlugin_Components_FmodEventComponent);

