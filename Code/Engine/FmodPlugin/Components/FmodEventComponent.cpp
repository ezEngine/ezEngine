#include <FmodPlugin/PCH.h>
#include <FmodPlugin/Components/FmodEventComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <FmodPlugin/FmodSingleton.h>

EZ_BEGIN_COMPONENT_TYPE(ezFmodEventComponent, 1);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Event", m_sEvent),
    EZ_ACCESSOR_PROPERTY("Paused", GetPaused, SetPaused),
    EZ_ACCESSOR_PROPERTY("Volume", GetVolume, SetVolume)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_ACCESSOR_PROPERTY("Pitch", GetPitch, SetPitch)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

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

  /// \todo store and restore current playback position
}


void ezFmodEventComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);

  auto& s = stream.GetStream();

  s >> m_bPaused;
  s >> m_fPitch;
  s >> m_fVolume;
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

ezComponent::Initialization ezFmodEventComponent::Initialize()
{
  if (m_sEvent.IsEmpty())
    m_sEvent = "event:/UI/Cancel";

  EZ_FMOD_ASSERT(ezFmod::GetSingleton()->GetSystem()->getEvent(m_sEvent.GetData(), &m_pEventDesc));

  EZ_FMOD_ASSERT(m_pEventDesc->createInstance(&m_pEventInstance));

  EZ_FMOD_ASSERT(m_pEventInstance->setPaused(m_bPaused));
  EZ_FMOD_ASSERT(m_pEventInstance->setPitch(m_fPitch * (float)GetWorld()->GetClock().GetSpeed()));
  EZ_FMOD_ASSERT(m_pEventInstance->setVolume(m_fVolume));

  m_pEventInstance->setUserData(this);
  m_pEventInstance->start();

  /// \todo register callbacks, pass through 'stopped' etc. events

  return ezComponent::Initialization::Done;
}


void ezFmodEventComponent::Deinitialize()
{
  if (m_pEventInstance != nullptr)
  {
    EZ_FMOD_ASSERT(m_pEventInstance->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT));
    m_pEventInstance->release();
    m_pEventInstance = nullptr;
  }
}


void ezFmodEventComponent::StartOneShot()
{
  FMOD::Studio::EventInstance* pEventInstance;

  EZ_FMOD_ASSERT(ezFmod::GetSingleton()->GetSystem()->getEvent(m_sEvent.GetData(), &m_pEventDesc));
  EZ_FMOD_ASSERT(m_pEventDesc->createInstance(&pEventInstance));

  SetParameters3d(pEventInstance);

  EZ_FMOD_ASSERT(pEventInstance->setVolume(m_fVolume));

  pEventInstance->start();
  pEventInstance->release();
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


