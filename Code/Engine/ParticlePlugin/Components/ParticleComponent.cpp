#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <Core/World/WorldModule.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezParticleComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Effect", GetParticleEffectFile, SetParticleEffectFile)->AddAttributes(new ezAssetBrowserAttribute("Particle Effect")),
    EZ_MEMBER_PROPERTY("Spawn At Start", m_bSpawnAtStart)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Auto Restart", m_bAutoRestart),
    EZ_MEMBER_PROPERTY("Min Restart Delay", m_MinRestartDelay),
    EZ_MEMBER_PROPERTY("Restart Delay Range", m_RestartDelayRange),
    EZ_MEMBER_PROPERTY("Random Seed", m_uiRandomSeed),
    EZ_MEMBER_PROPERTY("Shared Instance Name", m_sSharedInstanceName),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("FX"),
  }
  EZ_END_ATTRIBUTES
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

void ezParticleComponent::OnBeforeDetachedFromObject()
{
  m_EffectController.Invalidate();

  ezRenderComponent::OnBeforeDetachedFromObject();
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

bool ezParticleComponent::SpawnEffect()
{
  // stop any previous effect
  m_EffectController.Invalidate();

  if (m_hEffectResource.IsValid())
  {
    ezParticleWorldModule* pModule = static_cast<ezParticleWorldModule*>(GetManager()->GetUserData());
    m_EffectController.Create(m_hEffectResource, pModule, m_uiRandomSeed, m_sSharedInstanceName, this);

    m_EffectController.SetTransform(GetOwner()->GetGlobalTransform());

    return true;
  }

  return false;
}

void ezParticleComponent::StopEffect()
{
  m_EffectController.Invalidate();
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

void ezParticleComponent::SetParticleEffect(const ezParticleEffectResourceHandle& hEffect)
{
  m_hEffectResource = hEffect;
  m_EffectController.Invalidate();

  TriggerLocalBoundsUpdate(true);
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


ezResult ezParticleComponent::GetLocalBounds(ezBoundingBoxSphere& bounds)
{
  /// \todo Get bbox from somewhere

  bounds.ExpandToInclude(ezBoundingSphere(ezVec3::ZeroVector(), 1.0f));
  return EZ_SUCCESS;
}

void ezParticleComponent::Update()
{
  if (!IsEffectActive() && m_bSpawnAtStart)
  {
    if (SpawnEffect())
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
      SpawnEffect();
    }
  }

  m_EffectController.SetTransform(GetOwner()->GetGlobalTransform());
}

//////////////////////////////////////////////////////////////////////////



ezParticleComponentManager::ezParticleComponentManager(ezWorld* pWorld)
  : ezComponentManagerSimple<ComponentType, true>(pWorld)
{
}

void ezParticleComponentManager::Initialize()
{
  ezComponentManagerSimple<ComponentType, true>::Initialize();

  ezParticleWorldModule* pModule = static_cast<ezParticleWorldModule*>(ezWorldModule::FindModule(GetWorld(), ezParticleWorldModule::GetStaticRTTI()));

  SetUserData(pModule);
}
