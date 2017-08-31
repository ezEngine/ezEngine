#include <PCH.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <Core/World/WorldModule.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <ParticlePlugin/Components/ParticleFinisherComponent.h>

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezParticleComponent_PlayEffectMsg);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleComponent_PlayEffectMsg, 1, ezRTTIDefaultAllocator<ezParticleComponent_PlayEffectMsg>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Play", m_bPlay)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_COMPONENT_TYPE(ezParticleComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Effect", GetParticleEffectFile, SetParticleEffectFile)->AddAttributes(new ezAssetBrowserAttribute("Particle Effect")),
    EZ_MEMBER_PROPERTY("SpawnAtStart", m_bSpawnAtStart)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("AutoRestart", m_bAutoRestart),
    EZ_MEMBER_PROPERTY("MinRestartDelay", m_MinRestartDelay),
    EZ_MEMBER_PROPERTY("RestartDelayRange", m_RestartDelayRange),
    EZ_MEMBER_PROPERTY("RandomSeed", m_uiRandomSeed),
    EZ_MEMBER_PROPERTY("SharedInstanceName", m_sSharedInstanceName),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Effects"),
  }
  EZ_END_ATTRIBUTES
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezParticleComponent_PlayEffectMsg, Play),
    EZ_MESSAGE_HANDLER(ezExtractRenderDataMessage, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS

}
EZ_END_COMPONENT_TYPE


ezParticleComponent::ezParticleComponent()
{
  m_uiRandomSeed = 0;
  m_bSpawnAtStart = true;
  m_bAutoRestart = false;
}

ezParticleComponent::~ezParticleComponent()
{
}

void ezParticleComponent::OnDeactivated()
{
  HandOffToFinisher();

  ezRenderComponent::OnDeactivated();
}

void ezParticleComponent::SerializeComponent(ezWorldWriter& stream) const
{
  auto& s = stream.GetStream();

  s << m_hEffectResource;
  s << m_bSpawnAtStart;
  s << m_bAutoRestart;
  s << m_MinRestartDelay;
  s << m_RestartDelayRange;
  s << m_RestartTime;
  s << m_uiRandomSeed;
  s << m_sSharedInstanceName;

  /// \todo store effect state
}

void ezParticleComponent::DeserializeComponent(ezWorldReader& stream)
{
  auto& s = stream.GetStream();

  s >> m_hEffectResource;
  s >> m_bSpawnAtStart;
  s >> m_bAutoRestart;
  s >> m_MinRestartDelay;
  s >> m_RestartDelayRange;
  s >> m_RestartTime;
  s >> m_uiRandomSeed;
  s >> m_sSharedInstanceName;
}

bool ezParticleComponent::StartEffect()
{
  // stop any previous effect
  HandOffToFinisher();

  if (m_hEffectResource.IsValid())
  {
    ezParticleWorldModule* pModule = GetWorld()->GetOrCreateModule<ezParticleWorldModule>();
    m_EffectController.Create(m_hEffectResource, pModule, m_uiRandomSeed, m_sSharedInstanceName, this);

    m_EffectController.SetTransform(GetOwner()->GetGlobalTransform());

    return true;
  }

  return false;
}

void ezParticleComponent::StopEffect()
{
  HandOffToFinisher();
}

void ezParticleComponent::InterruptEffect()
{
  m_EffectController.StopImmediate();
  m_EffectController.Invalidate();
}

bool ezParticleComponent::IsEffectActive() const
{
  return m_EffectController.IsAlive();
}


void ezParticleComponent::Play(ezParticleComponent_PlayEffectMsg& msg)
{
  if (msg.m_bPlay)
  {
    StartEffect();
  }
  else
  {
    StopEffect();
  }
}

void ezParticleComponent::SetParticleEffect(const ezParticleEffectResourceHandle& hEffect)
{
  m_hEffectResource = hEffect;
  HandOffToFinisher();

  TriggerLocalBoundsUpdate();
}


void ezParticleComponent::SetParticleEffectFile(const char* szFile)
{
  ezParticleEffectResourceHandle hEffect;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hEffect = ezResourceManager::LoadResource<ezParticleEffectResource>(szFile);
  }

  SetParticleEffect(hEffect);
}


const char* ezParticleComponent::GetParticleEffectFile() const
{
  if (!m_hEffectResource.IsValid())
    return "";

  return m_hEffectResource.GetResourceID();
}


ezResult ezParticleComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  if (m_EffectController.IsAlive())
  {
    ezBoundingBoxSphere volume;
    const ezUInt64 uiChangeCounter = m_EffectController.GetBoundingVolume(volume);

    if (uiChangeCounter > 0)
    {
      ezTransform inv = GetOwner()->GetGlobalTransform().GetInverse();
      volume.Transform(inv.GetAsMat4());

      bounds.ExpandToInclude(volume);
      return EZ_SUCCESS;
    }
  }

  bounds.ExpandToInclude(ezBoundingSphere(ezVec3::ZeroVector(), 0.25f));
  return EZ_SUCCESS;
}


void ezParticleComponent::OnExtractRenderData(ezExtractRenderDataMessage& msg) const
{
  m_EffectController.SetIsInView();
}

void ezParticleComponent::Update()
{
  if (!IsEffectActive() && m_bSpawnAtStart)
  {
    if (StartEffect())
    {
      m_bSpawnAtStart = false;
    }
  }

  if (!IsEffectActive() && m_bAutoRestart)
  {
    const ezTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

    if (m_RestartTime == ezTime())
    {
      const ezTime tDiff = ezTime::Seconds(GetWorld()->GetRandomNumberGenerator().DoubleInRange(m_MinRestartDelay.GetSeconds(), m_RestartDelayRange.GetSeconds()));

      m_RestartTime = tNow + tDiff;
    }
    else if (m_RestartTime <= tNow)
    {
      m_RestartTime.SetZero();
      StartEffect();
    }
  }

  m_EffectController.SetTransform(GetOwner()->GetGlobalTransform());

  TriggerLocalBoundsUpdate();
}

void ezParticleComponent::HandOffToFinisher()
{
  if (m_EffectController.IsAlive())
  {
    ezGameObject* pOwner = GetOwner();
    ezWorld* pWorld = pOwner->GetWorld();

    ezTransform transform = pOwner->GetGlobalTransform();

    ezGameObjectDesc go;
    go.m_LocalPosition = transform.m_vPosition;
    go.m_LocalRotation = transform.m_qRotation;
    go.m_LocalScaling = transform.m_vScale;
    go.m_Tags = GetOwner()->GetTags();

    ezGameObject* pFinisher;
    pWorld->CreateObject(go, pFinisher);

    ezParticleFinisherComponent* pFinisherComp;
    pWorld->GetOrCreateComponentManager<ezParticleFinisherComponentManager>()->CreateComponent(pFinisher, pFinisherComp);

    pFinisherComp->m_EffectController = m_EffectController;
  }

  m_EffectController.Invalidate();
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Components_ParticleComponent);

