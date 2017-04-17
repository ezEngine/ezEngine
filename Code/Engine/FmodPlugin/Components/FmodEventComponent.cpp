#include <PCH.h>
#include <FmodPlugin/Components/FmodEventComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <FmodPlugin/FmodSingleton.h>
#include <FmodPlugin/Resources/FmodSoundEventResource.h>

EZ_BEGIN_COMPONENT_TYPE(ezFmodEventComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Paused", GetPaused, SetPaused),
    EZ_ACCESSOR_PROPERTY("Volume", GetVolume, SetVolume)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_ACCESSOR_PROPERTY("Pitch", GetPitch, SetPitch)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.01f, 100.0f)),
    EZ_ACCESSOR_PROPERTY("SoundEvent", GetSoundEventFile, SetSoundEventFile)->AddAttributes(new ezAssetBrowserAttribute("Sound Event")),
    EZ_ENUM_MEMBER_PROPERTY("OnFinishedAction", ezOnComponentFinishedAction, m_OnFinishedAction),
  }
  EZ_END_PROPERTIES
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

  /// \todo store and restore current playback position
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
    EZ_FMOD_ASSERT(m_pEventInstance->setPitch(m_fPitch * (float) GetWorld()->GetClock().GetSpeed()));
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
  m_hSoundEvent = hSoundEvent;

  if (m_pEventInstance)
  {
    StopSound();

    EZ_FMOD_ASSERT(m_pEventInstance->release());
    m_pEventInstance = nullptr;
  }
}

void ezFmodEventComponent::OnDeactivated()
{
  if (m_pEventInstance != nullptr)
  {
    EZ_FMOD_ASSERT(m_pEventInstance->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT));
    EZ_FMOD_ASSERT(m_pEventInstance->release());
    m_pEventInstance = nullptr;
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
  if (!m_hSoundEvent.IsValid())
    return;

  if (m_pEventInstance == nullptr)
  {
    ezResourceLock<ezFmodSoundEventResource> pEvent(m_hSoundEvent, ezResourceAcquireMode::NoFallback); // allow fallback ??

    if (pEvent->IsMissingResource())
      return;

    m_pEventInstance = pEvent->CreateInstance();
    EZ_ASSERT_DEV(m_pEventInstance != nullptr, "Sound Event Instance pointer should be valid");

    EZ_FMOD_ASSERT(m_pEventInstance->setUserData(this));
  }

  EZ_FMOD_ASSERT(m_pEventInstance->setPaused(m_bPaused));
  EZ_FMOD_ASSERT(m_pEventInstance->setPitch(m_fPitch * (float)GetWorld()->GetClock().GetSpeed()));
  EZ_FMOD_ASSERT(m_pEventInstance->setVolume(m_fVolume));

  EZ_FMOD_ASSERT(m_pEventInstance->start());
}

void ezFmodEventComponent::StartOneShot()
{
  /// \todo Move the one shot stuff into the event resource

  if (!m_hSoundEvent.IsValid())
    return;

  ezResourceLock<ezFmodSoundEventResource> pEvent(m_hSoundEvent, ezResourceAcquireMode::NoFallback);

  bool bIsOneShot = false;
  pEvent->GetDescriptor()->isOneshot(&bIsOneShot);

  // do not start sounds that will not terminate
  if (!bIsOneShot)
    return;

  FMOD::Studio::EventInstance* pEventInstance = pEvent->CreateInstance();

  SetParameters3d(pEventInstance);

  EZ_FMOD_ASSERT(pEventInstance->setVolume(m_fVolume));

  EZ_FMOD_ASSERT(pEventInstance->start());
  EZ_FMOD_ASSERT(pEventInstance->release());
}

void ezFmodEventComponent::StopSound()
{
  if (m_pEventInstance != nullptr)
  {
    EZ_FMOD_ASSERT(m_pEventInstance->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT));
  }
}


void ezFmodEventComponent::StopSoundImmediate()
{
  if (m_pEventInstance != nullptr)
  {
    EZ_FMOD_ASSERT(m_pEventInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE));
  }
}

void ezFmodEventComponent::Update()
{
  if (m_pEventInstance)
  {
    SetParameters3d(m_pEventInstance);

    FMOD_STUDIO_PLAYBACK_STATE state;
    EZ_FMOD_ASSERT(m_pEventInstance->getPlaybackState(&state));

    if (state == FMOD_STUDIO_PLAYBACK_STOPPED)
    {
      if (m_OnFinishedAction == ezOnComponentFinishedAction::DeleteEntity)
      {
        GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
      }
      else if (m_OnFinishedAction == ezOnComponentFinishedAction::DeleteComponent)
      {
        GetManager()->DeleteComponent(GetHandle());
      }
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

  EZ_FMOD_ASSERT(pEventInstance->setPitch(m_fPitch * (float)GetWorld()->GetClock().GetSpeed()));
  EZ_FMOD_ASSERT(pEventInstance->set3DAttributes(&attr));
}




EZ_STATICLINK_FILE(FmodPlugin, FmodPlugin_Components_FmodEventComponent);

