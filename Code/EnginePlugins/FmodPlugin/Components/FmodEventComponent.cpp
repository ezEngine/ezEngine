#include <FmodPlugin/FmodPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <FmodPlugin/Components/FmodEventComponent.h>
#include <FmodPlugin/FmodIncludes.h>
#include <FmodPlugin/FmodSingleton.h>
#include <FmodPlugin/Resources/FmodSoundEventResource.h>
#include <Foundation/Configuration/CVar.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

static_assert(sizeof(ezFmodParameterId) == sizeof(FMOD_STUDIO_PARAMETER_ID));

EZ_ALWAYS_INLINE FMOD_STUDIO_PARAMETER_ID ConvertEzToFmodId(ezFmodParameterId paramId)
{
  return *reinterpret_cast<FMOD_STUDIO_PARAMETER_ID*>(&paramId);
}

EZ_ALWAYS_INLINE ezFmodParameterId ConvertFmodToEzId(FMOD_STUDIO_PARAMETER_ID paramId)
{
  return *reinterpret_cast<ezFmodParameterId*>(&paramId);
}


//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgFmodSoundFinished);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgFmodSoundFinished, 1, ezRTTIDefaultAllocator<ezMsgFmodSoundFinished>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

ezCVarInt cvar_FmodOcclusionNumRays("FMOD.Occlusion.NumRays", 2, ezCVarFlags::Default, "Number of occlusion rays per component per frame");

static ezVec3 s_InSpherePositions[32];
static bool s_bInSpherePositionsInitialized = false;

ezFmodEventComponentManager::ezFmodEventComponentManager(ezWorld* pWorld)
  : ezComponentManager(pWorld)
{
  if (!s_bInSpherePositionsInitialized)
  {
    s_bInSpherePositionsInitialized = true;

    ezRandom rng;
    rng.Initialize(3);
    ezRandomGauss rngGauss;
    rngGauss.Initialize(27, 0xFFFF);

    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(s_InSpherePositions); ++i)
    {
      ezVec3& pos = s_InSpherePositions[i];
      pos.x = (float)rngGauss.SignedValue();
      pos.y = (float)rngGauss.SignedValue();
      pos.z = (float)rngGauss.SignedValue();

      float fRadius = ezMath::Pow((float)rng.DoubleZeroToOneExclusive(), 1.0f / 3.0f);
      fRadius = fRadius * 0.5f + 0.5f;
      pos.SetLength(fRadius).IgnoreResult();
    }
  }
}

void ezFmodEventComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezFmodEventComponentManager::UpdateOcclusion, this);
    desc.m_Phase = ezWorldUpdatePhase::Async;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezFmodEventComponentManager::UpdateEvents, this);
    desc.m_Phase = ezWorldUpdatePhase::PostTransform;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }

  ezResourceManager::GetResourceEvents().AddEventHandler(ezMakeDelegate(&ezFmodEventComponentManager::ResourceEventHandler, this));
}

void ezFmodEventComponentManager::Deinitialize()
{
  ezResourceManager::GetResourceEvents().RemoveEventHandler(ezMakeDelegate(&ezFmodEventComponentManager::ResourceEventHandler, this));

  SUPER::Deinitialize();
}

ezUInt32 ezFmodEventComponentManager::AddOcclusionState(ezFmodEventComponent* pComponent, ezFmodParameterId occlusionParamId, float fRadius)
{
  auto& occlusionState = m_OcclusionStates.ExpandAndGetRef();
  occlusionState.m_pComponent = pComponent;
  occlusionState.m_OcclusionParamId = occlusionParamId;
  occlusionState.m_fRadius = fRadius;

  if (const auto pPhysicsWorldModule = GetWorld()->GetModule<ezPhysicsWorldModuleInterface>())
  {
    ezVec3 listenerPos = ezFmod::GetSingleton()->GetListenerPosition();
    ShootOcclusionRays(occlusionState, listenerPos, 8, pPhysicsWorldModule, ezTime::MakeFromSeconds(1000.0));
  }

  return m_OcclusionStates.GetCount() - 1;
}

void ezFmodEventComponentManager::RemoveOcclusionState(ezUInt32 uiIndex)
{
  if (uiIndex >= m_OcclusionStates.GetCount())
    return;

  m_OcclusionStates.RemoveAtAndSwap(uiIndex);

  if (uiIndex != m_OcclusionStates.GetCount())
  {
    m_OcclusionStates[uiIndex].m_pComponent->m_uiOcclusionStateIndex = uiIndex;
  }
}

void ezFmodEventComponentManager::ShootOcclusionRays(OcclusionState& state, ezVec3 listenerPos, ezUInt32 uiNumRays, const ezPhysicsWorldModuleInterface* pPhysicsWorldModule, ezTime deltaTime)
{
  uiNumRays = ezMath::Min(uiNumRays, 32u);

  ezVec3 centerPos = state.m_pComponent->GetOwner()->GetGlobalPosition();
  ezUInt8 uiCollisionLayer = state.m_pComponent->m_uiOcclusionCollisionLayer;
  ezPhysicsCastResult hitResult;

  for (ezUInt32 i = 0; i < uiNumRays; ++i)
  {
    ezUInt32 uiRayIndex = state.m_uiNextRayIndex;
    ezVec3 targetPos = centerPos + s_InSpherePositions[uiRayIndex] * state.m_fRadius;
    ezVec3 dir = targetPos - listenerPos;
    float fDistance = dir.GetLengthAndNormalize();

    ezPhysicsQueryParameters query(uiCollisionLayer);
    query.m_bIgnoreInitialOverlap = true;
    query.m_ShapeTypes = ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic;

    bool bHit = pPhysicsWorldModule->Raycast(hitResult, listenerPos, dir, fDistance, query);
    if (bHit)
    {
      state.m_uiRaycastHits |= (1 << uiRayIndex);
    }
    else
    {
      state.m_uiRaycastHits &= ~(1 << uiRayIndex);
    }

    state.m_uiNextRayIndex = (state.m_uiNextRayIndex + 1) % 32;
    state.m_uiNumUsedRays = ezMath::Min<ezUInt8>(state.m_uiNumUsedRays + 1, 32);
  }

  float fNewOcclusionValue = (float)ezMath::CountBits(state.m_uiRaycastHits) / state.m_uiNumUsedRays;
  float fNormalizedDistance = ezMath::Min((centerPos - listenerPos).GetLength() / state.m_fRadius, 1.0f);
  fNewOcclusionValue = ezMath::Max(fNewOcclusionValue - 1.0f + fNormalizedDistance, 0.0f);
  state.m_fLastOcclusionValue = ezMath::Lerp(state.m_fLastOcclusionValue, fNewOcclusionValue, ezMath::Min(deltaTime.GetSeconds() * 8.0, 1.0));
}

void ezFmodEventComponentManager::UpdateOcclusion(const ezWorldModule::UpdateContext& context)
{
  if (const auto pPhysicsWorldModule = GetWorld()->GetModuleReadOnly<ezPhysicsWorldModuleInterface>())
  {
    ezVec3 listenerPos = ezFmod::GetSingleton()->GetListenerPosition();
    ezTime deltaTime = GetWorld()->GetClock().GetTimeDiff();

    ezUInt32 uiNumRays = ezMath::Clamp<int>(cvar_FmodOcclusionNumRays, 1, 32);

    for (auto& occlusionState : m_OcclusionStates)
    {
      ShootOcclusionRays(occlusionState, listenerPos, uiNumRays, pPhysicsWorldModule, deltaTime);
    }
  }
}

void ezFmodEventComponentManager::UpdateEvents(const ezWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->Update();
    }
  }
}

void ezFmodEventComponentManager::ResourceEventHandler(const ezResourceEvent& e)
{
  if (e.m_Type == ezResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<ezFmodSoundEventResource>())
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

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezFmodEventComponent, 4, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Paused", GetPaused, SetPaused),
    EZ_ACCESSOR_PROPERTY("Volume", GetVolume, SetVolume)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_ACCESSOR_PROPERTY("Pitch", GetPitch, SetPitch)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.1f, 10.0f)),
    EZ_ACCESSOR_PROPERTY("NoGlobalPitch", GetNoGlobalPitch, SetNoGlobalPitch),
    EZ_RESOURCE_ACCESSOR_PROPERTY("SoundEvent", GetSoundEvent, SetSoundEvent)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Fmod_Event", ezDependencyFlags::Package)),
    EZ_ACCESSOR_PROPERTY("UseOcclusion", GetUseOcclusion, SetUseOcclusion),
    EZ_ACCESSOR_PROPERTY("OcclusionThreshold", GetOcclusionThreshold, SetOcclusionThreshold)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_ACCESSOR_PROPERTY("OcclusionCollisionLayer", GetOcclusionCollisionLayer, SetOcclusionCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_ENUM_MEMBER_PROPERTY("OnFinishedAction", ezOnComponentFinishedAction, m_OnFinishedAction),
    EZ_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    //EZ_FUNCTION_PROPERTY("Preview", StartOneShot), // This doesn't seem to be working anymore, and I cannot find code for exposing it in the UI either
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgDeleteGameObject, OnMsgDeleteGameObject),
    EZ_MESSAGE_HANDLER(ezMsgSetFloatParameter, OnMsgSetFloatParameter),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_MESSAGESENDERS
  {
    EZ_MESSAGE_SENDER(m_SoundFinishedEventSender),
  }
  EZ_END_MESSAGESENDERS;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(Play),
    EZ_SCRIPT_FUNCTION_PROPERTY(Pause),
    EZ_SCRIPT_FUNCTION_PROPERTY(Stop),
    EZ_SCRIPT_FUNCTION_PROPERTY(FadeOut),
    EZ_SCRIPT_FUNCTION_PROPERTY(StartOneShot),
    EZ_SCRIPT_FUNCTION_PROPERTY(SoundCue),
    EZ_SCRIPT_FUNCTION_PROPERTY(SetEventParameter, In, "ParamName", In, "Value"),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

enum
{
  ShowDebugInfoFlag = 0,
  NoGlobalPitch = 1,
};

ezFmodEventComponent::ezFmodEventComponent()
{
  m_pEventDesc = nullptr;
  m_pEventInstance = nullptr;
  m_bPaused = false;
  m_bUseOcclusion = false;
  m_uiOcclusionThreshold = 128;
  m_uiOcclusionCollisionLayer = 0;
  m_fPitch = 1.0f;
  m_fVolume = 1.0f;
}

ezFmodEventComponent::~ezFmodEventComponent() = default;

void ezFmodEventComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_bPaused;
  s << m_bUseOcclusion;
  s << m_uiOcclusionThreshold;
  s << m_uiOcclusionCollisionLayer;
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

    if (state == FMOD_STUDIO_PLAYBACK_STARTING || state == FMOD_STUDIO_PLAYBACK_PLAYING || state == FMOD_STUDIO_PLAYBACK_SUSTAINING)
    {
      m_pEventInstance->getTimelinePosition(&iTimelinePosition);
    }
  }

  s << iTimelinePosition;
}

void ezFmodEventComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_bPaused;

  if (uiVersion >= 3)
  {
    s >> m_bUseOcclusion;
  }

  if (uiVersion >= 4)
  {
    s >> m_uiOcclusionThreshold;
    s >> m_uiOcclusionCollisionLayer;
  }

  s >> m_fPitch;
  s >> m_fVolume;
  s >> m_hSoundEvent;

  ezOnComponentFinishedAction::StorageType type;
  s >> type;
  m_OnFinishedAction = (ezOnComponentFinishedAction::Enum)type;

  s >> m_iTimelinePosition;
}

void ezFmodEventComponent::SetPaused(bool b)
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

void ezFmodEventComponent::SetUseOcclusion(bool b)
{
  if (b == GetUseOcclusion())
    return;

  m_bUseOcclusion = b;

  if (!b)
  {
    static_cast<ezFmodEventComponentManager*>(GetOwningManager())->RemoveOcclusionState(m_uiOcclusionStateIndex);
    m_uiOcclusionStateIndex = ezInvalidIndex;
  }
}

void ezFmodEventComponent::SetOcclusionCollisionLayer(ezUInt8 uiCollisionLayer)
{
  m_uiOcclusionCollisionLayer = uiCollisionLayer;
}

void ezFmodEventComponent::SetOcclusionThreshold(float fThreshold)
{
  m_uiOcclusionThreshold = ezMath::ColorFloatToByte(fThreshold);
}

float ezFmodEventComponent::GetOcclusionThreshold() const
{
  return ezMath::ColorByteToFloat(m_uiOcclusionThreshold);
}

void ezFmodEventComponent::SetPitch(float f)
{
  if (f == m_fPitch)
    return;

  m_fPitch = f;

  if (m_pEventInstance != nullptr)
  {
    if (GetNoGlobalPitch())
    {
      EZ_FMOD_ASSERT(m_pEventInstance->setPitch(m_fPitch));
    }
    else
    {
      EZ_FMOD_ASSERT(m_pEventInstance->setPitch(m_fPitch * (float)GetWorld()->GetClock().GetSpeed()));
    }
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

void ezFmodEventComponent::SetSoundEvent(const ezFmodSoundEventResourceHandle& hSoundEvent)
{
  if (m_pEventInstance)
  {
    Stop();

    EZ_FMOD_ASSERT(m_pEventInstance->release());
    m_pEventInstance = nullptr;
    m_iTimelinePosition = -1;
  }

  m_hSoundEvent = hSoundEvent;
}

void ezFmodEventComponent::SetShowDebugInfo(bool bShow)
{
  SetUserFlag(ShowDebugInfoFlag, bShow);
}

bool ezFmodEventComponent::GetShowDebugInfo() const
{
  return GetUserFlag(ShowDebugInfoFlag);
}

void ezFmodEventComponent::SetNoGlobalPitch(bool bEnable)
{
  SetUserFlag(NoGlobalPitch, bEnable);
}

bool ezFmodEventComponent::GetNoGlobalPitch() const
{
  return GetUserFlag(NoGlobalPitch);
}

void ezFmodEventComponent::OnSimulationStarted()
{
  if (!m_bPaused)
  {
    Play();
  }
}

void ezFmodEventComponent::OnDeactivated()
{
  if (GetUseOcclusion())
  {
    static_cast<ezFmodEventComponentManager*>(GetOwningManager())->RemoveOcclusionState(m_uiOcclusionStateIndex);
    m_uiOcclusionStateIndex = ezInvalidIndex;
  }

  FadeOut();
}

void ezFmodEventComponent::Play()
{
  if (!m_hSoundEvent.IsValid() || !IsActiveAndSimulating())
    return;

  if (m_pEventInstance == nullptr)
  {
    ezResourceLock<ezFmodSoundEventResource> pEvent(m_hSoundEvent, ezResourceAcquireMode::BlockTillLoaded);

    if (pEvent.GetAcquireResult() == ezResourceAcquireResult::MissingFallback)
      return;

    m_pEventInstance = pEvent->CreateInstance();
    if (m_pEventInstance == nullptr)
    {
      ezLog::Error("Failed to start sound event '{}'.", m_hSoundEvent.GetResourceIdOrDescription());
      return;
    }
  }

  if (m_bPaused)
  {
    m_bPaused = false;
    EZ_FMOD_ASSERT(m_pEventInstance->setPaused(false));
  }

  UpdateParameters(m_pEventInstance);

  FMOD_STUDIO_PLAYBACK_STATE state;
  EZ_FMOD_ASSERT(m_pEventInstance->getPlaybackState(&state));

  if (state != FMOD_STUDIO_PLAYBACK_PLAYING && state != FMOD_STUDIO_PLAYBACK_SUSTAINING)
  {
    // reset this, using the value is handled outside this function
    m_iTimelinePosition = -1;

    EZ_FMOD_ASSERT(m_pEventInstance->start());
  }
}

void ezFmodEventComponent::Pause()
{
  if (m_bPaused)
    return;

  m_bPaused = true;

  if (m_pEventInstance == nullptr)
    return;

  EZ_FMOD_ASSERT(m_pEventInstance->setPaused(m_bPaused));
}

void ezFmodEventComponent::FadeOut()
{
  if (m_pEventInstance == nullptr)
    return;

  EZ_FMOD_ASSERT(m_pEventInstance->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT));

  EZ_FMOD_ASSERT(m_pEventInstance->release());
  m_pEventInstance = nullptr;
  m_iTimelinePosition = -1;
}

void ezFmodEventComponent::StartOneShot()
{
  if (!m_hSoundEvent.IsValid())
    return;

  ezResourceLock<ezFmodSoundEventResource> pEvent(m_hSoundEvent, ezResourceAcquireMode::BlockTillLoaded);

  if (pEvent.GetAcquireResult() == ezResourceAcquireResult::MissingFallback)
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
    ezLog::Warning("ezFmodEventComponent::StartOneShot: Request ignored, because sound event '{}' is not a one-shot event.", pEvent->GetResourceIdOrDescription());
    return;
  }

  FMOD::Studio::EventInstance* pEventInstance = pEvent->CreateInstance();

  if (pEventInstance == nullptr)
  {
    ezLog::Debug("Cannot start one-shot sound event, instance could not be created.");
    return;
  }

  UpdateParameters(pEventInstance);

  EZ_FMOD_ASSERT(pEventInstance->start());
  EZ_FMOD_ASSERT(pEventInstance->release());
}

void ezFmodEventComponent::Stop()
{
  if (m_pEventInstance == nullptr)
    return;

  m_iTimelinePosition = -1;
  EZ_FMOD_ASSERT(m_pEventInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE));
}

void ezFmodEventComponent::SoundCue()
{
  if (m_pEventInstance != nullptr && m_pEventInstance->isValid())
  {
#if ((FMOD_VERSION & 0x0000FF00) >> 8 >= 2)
    EZ_FMOD_ASSERT(m_pEventInstance->keyOff());
#else
    EZ_FMOD_ASSERT(m_pEventInstance->triggerCue());
#endif
  }
}

void ezFmodEventComponent::OnMsgDeleteGameObject(ezMsgDeleteGameObject& msg)
{
  ezOnComponentFinishedAction::HandleDeleteObjectMsg(msg, m_OnFinishedAction);
}

ezFmodParameterId ezFmodEventComponent::FindParameter(const char* szName) const
{
  if (!m_hSoundEvent.IsValid())
    return ezFmodParameterId();

  ezResourceLock<ezFmodSoundEventResource> pEvent(m_hSoundEvent, ezResourceAcquireMode::BlockTillLoaded);

  FMOD::Studio::EventDescription* pEventDesc = pEvent->GetDescriptor();
  if (pEventDesc == nullptr || !pEventDesc->isValid())
    return ezFmodParameterId();

  FMOD_STUDIO_PARAMETER_DESCRIPTION paramDesc;
  if (pEventDesc->getParameterDescriptionByName(szName, &paramDesc) != FMOD_OK)
    return ezFmodParameterId();

  return ConvertFmodToEzId(paramDesc.id);
}

void ezFmodEventComponent::SetParameter(ezFmodParameterId paramId, float fValue)
{
  if (m_pEventInstance == nullptr || !m_pEventInstance->isValid() || paramId.IsInvalidated())
    return;

  m_pEventInstance->setParameterByID(ConvertEzToFmodId(paramId), fValue);
}

float ezFmodEventComponent::GetParameter(ezFmodParameterId paramId) const
{
  if (m_pEventInstance == nullptr || !m_pEventInstance->isValid() || paramId.IsInvalidated())
    return 0.0f;

  float value = 0;
  float fFinalValue = 0.0f;
  m_pEventInstance->getParameterByID(ConvertEzToFmodId(paramId), &value, &fFinalValue);
  return fFinalValue;
}

void ezFmodEventComponent::SetEventParameter(const char* szParamName, float fValue)
{
  ezFmodParameterId paramId = FindParameter(szParamName);
  if (paramId.IsInvalidated())
    return;

  SetParameter(paramId, fValue);
}

void ezFmodEventComponent::OnMsgSetFloatParameter(ezMsgSetFloatParameter& ref_msg)
{
  SetEventParameter(ref_msg.m_sParameterName, ref_msg.m_fValue);
}

void ezFmodEventComponent::Update()
{
  FMOD_STUDIO_PLAYBACK_STATE state = FMOD_STUDIO_PLAYBACK_FORCEINT;

  if (m_pEventInstance)
  {
    if (!m_pEventInstance->isValid())
    {
      InvalidateResource(false);
      return;
    }

    UpdateParameters(m_pEventInstance);
    if (GetUseOcclusion())
    {
      UpdateOcclusion();
    }

    EZ_FMOD_ASSERT(m_pEventInstance->getPlaybackState(&state));

    if (state == FMOD_STUDIO_PLAYBACK_STOPPED)
    {
      m_iTimelinePosition = -1;

      ezMsgFmodSoundFinished msg;
      m_SoundFinishedEventSender.SendEventMessage(msg, this, GetOwner());

      ezOnComponentFinishedAction::HandleFinishedAction(this, m_OnFinishedAction);
    }
  }
  else if (m_iTimelinePosition >= 0 && !m_bPaused)
  {
    // Restore the event to the last playback position
    const ezInt32 iTimelinePos = m_iTimelinePosition;

    Play(); // will reset m_iTimelinePosition, that's why it is copied first

    if (m_pEventInstance)
    {
      // we need to store this value again, because it is very likely during a resource reload that we
      // run into the InvalidateResource(false) case at the top of this function, right after this
      // so we can restore the timeline position again
      m_iTimelinePosition = iTimelinePos;
      m_pEventInstance->setTimelinePosition(iTimelinePos);
    }
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  if (GetShowDebugInfo())
  {
    if (m_pEventInstance)
    {
      FMOD::Studio::EventDescription* pDesc = nullptr;
      m_pEventInstance->getDescription(&pDesc);

      bool is3D = false;
      pDesc->is3D(&is3D);
      if (is3D)
      {
        float minDistance = 0.0f;
        float maxDistance = 0.0f;
#  if ((FMOD_VERSION & 0x0000FF00) >> 8 >= 2)
        pDesc->getMinMaxDistance(&minDistance, &maxDistance);
#  else
        pDesc->getMinimumDistance(&minDistance);
        pDesc->getMaximumDistance(&maxDistance);
#  endif

        ezDebugRenderer::DrawLineSphere(GetWorld(), ezBoundingSphere::MakeFromCenterAndRadius(GetOwner()->GetGlobalPosition(), minDistance), ezColor::Blue);
        ezDebugRenderer::DrawLineSphere(GetWorld(), ezBoundingSphere::MakeFromCenterAndRadius(GetOwner()->GetGlobalPosition(), maxDistance), ezColor::Cyan);
      }

      char path[128];
      pDesc->getPath(path, EZ_ARRAY_SIZE(path), nullptr);

      const char* szStates[] = {"PLAYING", "SUSTAINING", "STOPPED", "STARTING", "STOPPING"};

      const char* szCurrentState = "Invalid";
      if (state != FMOD_STUDIO_PLAYBACK_FORCEINT)
      {
        szCurrentState = szStates[state];
      }

      ezStringBuilder sb;
      sb.SetFormat("{}\n{}", path, szCurrentState);

      if (GetUseOcclusion())
      {
        auto& occlusionState = static_cast<ezFmodEventComponentManager*>(GetOwningManager())->GetOcclusionState(m_uiOcclusionStateIndex);
        sb.AppendFormat("\nOcclusion: {}", occlusionState.GetOcclusionValue(GetOcclusionThreshold()));

        ezVec3 centerPos = GetOwner()->GetGlobalPosition();
        for (ezUInt32 uiRayIndex = 0; uiRayIndex < EZ_ARRAY_SIZE(s_InSpherePositions); ++uiRayIndex)
        {
          ezVec3 targetPos = centerPos + s_InSpherePositions[uiRayIndex] * occlusionState.m_fRadius;
          ezColor color = (occlusionState.m_uiRaycastHits & (1 << uiRayIndex)) ? ezColor::Red : ezColor::Green;
          ezDebugRenderer::DrawCross(GetWorld(), targetPos, 0.1f, color);
        }
      }

      ezDebugRenderer::Draw3DText(GetWorld(), sb, GetOwner()->GetGlobalPosition(), ezColor::Cyan);
    }
  }
#endif
}

void ezFmodEventComponent::UpdateParameters(FMOD::Studio::EventInstance* pInstance)
{
  const auto pos = GetOwner()->GetGlobalPosition();
  const auto vel = GetOwner()->GetLinearVelocity();
  const auto fwd = GetOwner()->GetGlobalRotation() * ezVec3::MakeAxisX();
  const auto up = GetOwner()->GetGlobalRotation() * ezVec3::MakeAxisZ();

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

  if (GetNoGlobalPitch())
  {
    EZ_FMOD_ASSERT(pInstance->setPitch(m_fPitch));
  }
  else
  {
    // have to update pitch every time, in case the clock speed changes
    EZ_FMOD_ASSERT(pInstance->setPitch(m_fPitch * (float)GetWorld()->GetClock().GetSpeed()));
  }

  EZ_FMOD_ASSERT(pInstance->setVolume(m_fVolume));
  EZ_FMOD_ASSERT(pInstance->set3DAttributes(&attr));
}

void ezFmodEventComponent::UpdateOcclusion()
{
  if (m_uiOcclusionStateIndex == ezInvalidIndex)
  {
    ezFmodParameterId occlusionParamId = FindParameter("Occlusion");
    if (occlusionParamId.IsInvalidated())
    {
      ezLog::Warning("'Occlusion' FMOD Event Parameter could not be found.");
      m_bUseOcclusion = false;
      return;
    }

    float fRadius = 1.0f;
    float fMaxDist = 0;
    {
      FMOD::Studio::EventDescription* pDesc = nullptr;
      m_pEventInstance->getDescription(&pDesc);

      bool is3D = false;
      pDesc->is3D(&is3D);
      if (is3D)
      {
#if ((FMOD_VERSION & 0x0000FF00) >> 8 >= 2)
        pDesc->getMinMaxDistance(&fRadius, &fMaxDist);
#else
        pDesc->getMinimumDistance(&fRadius);
        pDesc->getMaximumDistance(&fMaxDist);
#endif
      }
    }

    m_uiOcclusionStateIndex = static_cast<ezFmodEventComponentManager*>(GetOwningManager())->AddOcclusionState(this, occlusionParamId, fRadius);
  }

  auto& occlusionState = static_cast<ezFmodEventComponentManager*>(GetOwningManager())->GetOcclusionState(m_uiOcclusionStateIndex);
  SetParameter(occlusionState.m_OcclusionParamId, occlusionState.GetOcclusionValue(GetOcclusionThreshold()));
}

void ezFmodEventComponent::InvalidateResource(bool bTryToRestore)
{
  if (m_pEventInstance)
  {
    if (bTryToRestore)
    {
      FMOD_STUDIO_PLAYBACK_STATE state;
      m_pEventInstance->getPlaybackState(&state);

      if (state == FMOD_STUDIO_PLAYBACK_STARTING || state == FMOD_STUDIO_PLAYBACK_PLAYING || state == FMOD_STUDIO_PLAYBACK_SUSTAINING)
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
  }
}

EZ_STATICLINK_FILE(FmodPlugin, FmodPlugin_Components_FmodEventComponent);
