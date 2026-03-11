#include <MiniAudioPlugin/MiniAudioPluginPCH.h>

#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/ResourceManager/Implementation/ResourceHandleReflection.h>
#include <Core/World/GameObject.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <MiniAudioPlugin/Components/MiniAudioSoundComponent.h>
#include <MiniAudioPlugin/MiniAudioSingleton.h>
#include <MiniAudioPlugin/Resources/MiniAudioSoundResource.h>

ezMiniAudioSoundComponentManager::ezMiniAudioSoundComponentManager(ezWorld* pWorld)
  : ezComponentManager(pWorld)
{
}

void ezMiniAudioSoundComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezMiniAudioSoundComponentManager::UpdateEvents, this);
    desc.m_Phase = ezWorldUpdatePhase::PostTransform;
    desc.m_bOnlyUpdateWhenSimulating = true;

    RegisterUpdateFunction(desc);
  }
}

void ezMiniAudioSoundComponentManager::Deinitialize()
{
  SUPER::Deinitialize();

  ezMiniAudioSingleton::GetSingleton()->StopWorldSounds(GetWorld());
}

void ezMiniAudioSoundComponentManager::UpdateEvents(const ezWorldModule::UpdateContext& context)
{
  constexpr ezUInt32 uiUpdatesPerSec = 20;
  constexpr ezTime tUpdateRate = ezTime::Milliseconds(1000 / uiUpdatesPerSec);

  const float fUpdateFraction = (GetWorld()->GetClock().GetTimeDiff() / tUpdateRate).AsFloatInSeconds();

  const ezUInt32 uiNumComps = m_ComponentStorage.GetCount();

  if (m_uiFirstComponentIndex >= uiNumComps)
  {
    m_uiFirstComponentIndex = 0;
  }

  const ezUInt32 uiNumUpdate = static_cast<ezUInt32>(uiNumComps * fUpdateFraction) + 1;
  const ezUInt32 uiLastCompP1 = ezMath::Min(m_uiFirstComponentIndex + uiNumUpdate, uiNumComps);

  for (auto it = m_ComponentStorage.GetIterator(m_uiFirstComponentIndex, uiNumComps); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;

    // a lot of components will actually be inactive (waiting to be reused)
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->Update();
    }
  }

  m_uiFirstComponentIndex = uiLastCompP1;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezMiniAudioSoundComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_MEMBER_PROPERTY("Sound", m_hSound)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_MiniAudio_Sound", ezDependencyFlags::Package)),
    EZ_ACCESSOR_PROPERTY("Paused", GetPaused, SetPaused),
    EZ_ACCESSOR_PROPERTY("Volume", GetVolume, SetVolume)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_ACCESSOR_PROPERTY("Pitch", GetPitch, SetPitch)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.1f, 10.0f)),
    EZ_ACCESSOR_PROPERTY("NoGlobalPitch", GetNoGlobalPitch, SetNoGlobalPitch),
    EZ_ENUM_MEMBER_PROPERTY("OnFinishedAction", ezOnComponentFinishedAction2, m_OnFinishedAction),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgDeleteGameObject, OnMsgDeleteGameObject),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(Play),
    EZ_SCRIPT_FUNCTION_PROPERTY(Pause),
    EZ_SCRIPT_FUNCTION_PROPERTY(Stop),
    EZ_SCRIPT_FUNCTION_PROPERTY(FadeOut, In, "Delay"),
    EZ_SCRIPT_FUNCTION_PROPERTY(StartOneShot),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Sound/MiniAudio"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

enum
{
  NoGlobalPitch = 0,
};

ezMiniAudioSoundComponent::ezMiniAudioSoundComponent() = default;
ezMiniAudioSoundComponent::~ezMiniAudioSoundComponent() = default;

void ezMiniAudioSoundComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_hSound;
  s << m_bPaused;
  s << m_fPitch;
  s << m_fComponentVolume;

  ezOnComponentFinishedAction2::StorageType type = m_OnFinishedAction;
  s << type;
}

void ezMiniAudioSoundComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_hSound;
  s >> m_bPaused;
  s >> m_fPitch;
  s >> m_fComponentVolume;

  ezOnComponentFinishedAction2::StorageType type;
  s >> type;
  m_OnFinishedAction = (ezOnComponentFinishedAction2::Enum)type;
}

void ezMiniAudioSoundComponent::SetPaused(bool b)
{
  if (b == m_bPaused)
    return;

  m_bPaused = b;

  if (m_bPaused)
  {
    Pause();
  }
  else
  {
    Play();
  }
}

void ezMiniAudioSoundComponent::SetPitch(float f)
{
  if (f == m_fPitch)
    return;

  m_fPitch = f;
}

void ezMiniAudioSoundComponent::SetVolume(float f)
{
  if (f == m_fComponentVolume)
    return;

  m_fComponentVolume = f;
}

void ezMiniAudioSoundComponent::SetNoGlobalPitch(bool bEnable)
{
  SetUserFlag(NoGlobalPitch, bEnable);
}

bool ezMiniAudioSoundComponent::GetNoGlobalPitch() const
{
  return GetUserFlag(NoGlobalPitch);
}

void ezMiniAudioSoundComponent::OnSimulationStarted()
{
  if (!m_bPaused)
  {
    Play();
  }
}

void ezMiniAudioSoundComponent::OnDeactivated()
{
  if (m_pInstance)
  {
    ezMiniAudioSingleton* pMA = ezMiniAudioSingleton::GetSingleton();

    // fade it out over a short period
    pMA->DetachAndFadeOutSoundInstance(m_pInstance, ezTime::Milliseconds(500));
  }
}

void ezMiniAudioSoundComponent::Play()
{
  if (!m_hSound.IsValid())
    return;

  if (m_pInstance == nullptr)
  {
    ezResourceLock<ezMiniAudioSoundResource> pResource(m_hSound, ezResourceAcquireMode::BlockTillLoaded_NeverFail);

    if (pResource.GetAcquireResult() != ezResourceAcquireResult::Final)
      return;

    ezRandom& rng = GetWorld()->GetRandomNumberGenerator();
    m_pInstance = pResource->InstantiateSound(&rng, GetWorld(), GetHandle());

    m_fResourceVolume = pResource->GetVolume(rng);
    m_fResourcePitch = pResource->GetPitch(rng);

    Update();
  }

  EZ_MA_CHECK(ma_sound_start(&m_pInstance->m_Sound));
  m_bPaused = false;
}

void ezMiniAudioSoundComponent::Pause()
{
  if (m_pInstance)
  {
    EZ_MA_CHECK(ma_sound_stop(&m_pInstance->m_Sound));
  }
}

void ezMiniAudioSoundComponent::Stop()
{
  if (m_pInstance == nullptr)
    return;

  // just free the sound, this will stop it right away
  ezMiniAudioSingleton* pMA = ezMiniAudioSingleton::GetSingleton();
  pMA->FreeSoundInstance(m_pInstance);
}

void ezMiniAudioSoundComponent::FadeOut(ezTime fadeDuration)
{
  if (m_pInstance == nullptr)
    return;

  ezMiniAudioSingleton* pMA = ezMiniAudioSingleton::GetSingleton();
  pMA->DetachAndFadeOutSoundInstance(m_pInstance, fadeDuration);
}

void ezMiniAudioSoundComponent::StartOneShot()
{
  if (!m_hSound.IsValid())
    return;

  ezResourceLock<ezMiniAudioSoundResource> pResource(m_hSound, ezResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pResource.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  ezRandom& rng = GetWorld()->GetRandomNumberGenerator();
  auto pInstance = pResource->InstantiateSound(&rng, GetWorld(), {});

  const float fResourceVolume = pResource->GetVolume(rng);
  const float fResourcePitch = pResource->GetPitch(rng);

  UpdateParameters(pInstance, m_fComponentVolume * fResourceVolume, m_fPitch * fResourcePitch);

  // the sound will play until it ends and then get cleaned up automatically
  EZ_MA_CHECK(ma_sound_start(&pInstance->m_Sound));
}

void ezMiniAudioSoundComponent::OnMsgDeleteGameObject(ezMsgDeleteGameObject& msg)
{
  ezOnComponentFinishedAction2::HandleDeleteObjectMsg(msg, m_OnFinishedAction);
}

void ezMiniAudioSoundComponent::Update()
{
  if (m_pInstance)
  {
    UpdateParameters(m_pInstance, m_fComponentVolume * m_fResourceVolume, m_fPitch * m_fResourcePitch);
  }
}

void ezMiniAudioSoundComponent::UpdateParameters(ezMiniAudioSoundInstance* pInstance, float fVolume, float fPitch) const
{
  const ezVec3 pos = GetOwner()->GetGlobalPosition();

  ma_sound_set_position(&pInstance->m_Sound, pos.x, pos.y, pos.z);

  if (GetNoGlobalPitch())
  {
    ma_sound_set_pitch(&pInstance->m_Sound, fPitch);
  }
  else
  {
    ma_sound_set_pitch(&pInstance->m_Sound, fPitch * (float)GetWorld()->GetClock().GetSpeed());
  }

  ma_sound_set_volume(&pInstance->m_Sound, fVolume);

  // no need to set the direction, we currently don't support directional sounds
  // const ezVec3 dir = GetOwner()->GetGlobalDirForwards();
  // ma_sound_set_direction(&m_pInstance->m_Sound, dir.x, dir.y, dir.z);
}

void ezMiniAudioSoundComponent::SoundFinished()
{
  // reset used sound
  m_pInstance = nullptr;

  // TODO MiniAudio: send event
  // ezMsgFmodSoundFinished msg;
  // m_SoundFinishedEventSender.SendEventMessage(msg, this, GetOwner());

  ezOnComponentFinishedAction2::HandleFinishedAction(this, m_OnFinishedAction);

  if (m_OnFinishedAction == ezOnComponentFinishedAction2::Restart)
  {
    Play();
  }
}
