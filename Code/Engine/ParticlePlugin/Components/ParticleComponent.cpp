#include <PCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/WorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <ParticlePlugin/Components/ParticleFinisherComponent.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezParticleComponent, 3, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Effect", GetParticleEffectFile, SetParticleEffectFile)->AddAttributes(new ezAssetBrowserAttribute("Particle Effect")),
    EZ_MEMBER_PROPERTY("SpawnAtStart", m_bSpawnAtStart)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ENUM_MEMBER_PROPERTY("OnFinishedAction", ezOnComponentFinishedAction2, m_OnFinishedAction),
    EZ_MEMBER_PROPERTY("MinRestartDelay", m_MinRestartDelay),
    EZ_MEMBER_PROPERTY("RestartDelayRange", m_RestartDelayRange),
    EZ_MEMBER_PROPERTY("RandomSeed", m_uiRandomSeed),
    EZ_MEMBER_PROPERTY("SharedInstanceName", m_sSharedInstanceName),
    EZ_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new ezExposedParametersAttribute("Effect"), new ezExposeColorAlphaAttribute),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Effects"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgSetPlaying, OnSetPlaying),
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;

}
EZ_END_COMPONENT_TYPE
// clang-format on

ezParticleComponent::ezParticleComponent()
{
  m_uiRandomSeed = 0;
  m_bSpawnAtStart = true;
}

ezParticleComponent::~ezParticleComponent() = default;

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

  // Version 1
  {
    bool bAutoRestart = false;
    s << bAutoRestart;
  }

  s << m_MinRestartDelay;
  s << m_RestartDelayRange;
  s << m_RestartTime;
  s << m_uiRandomSeed;
  s << m_sSharedInstanceName;

  // Version 2
  s << m_FloatParams.GetCount();
  for (ezUInt32 i = 0; i < m_FloatParams.GetCount(); ++i)
  {
    s << m_FloatParams[i].m_sName;
    s << m_FloatParams[i].m_Value;
  }
  s << m_ColorParams.GetCount();
  for (ezUInt32 i = 0; i < m_ColorParams.GetCount(); ++i)
  {
    s << m_ColorParams[i].m_sName;
    s << m_ColorParams[i].m_Value;
  }

  // Version 3
  s << m_OnFinishedAction;

  /// \todo store effect state
}

void ezParticleComponent::DeserializeComponent(ezWorldReader& stream)
{
  auto& s = stream.GetStream();
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  s >> m_hEffectResource;
  s >> m_bSpawnAtStart;

  // Version 1
  {
    bool bAutoRestart = false;
    s >> bAutoRestart;
  }

  s >> m_MinRestartDelay;
  s >> m_RestartDelayRange;
  s >> m_RestartTime;
  s >> m_uiRandomSeed;
  s >> m_sSharedInstanceName;

  if (uiVersion >= 2)
  {
    ezUInt32 numFloats, numColors;

    s >> numFloats;
    m_FloatParams.SetCountUninitialized(numFloats);

    for (ezUInt32 i = 0; i < m_FloatParams.GetCount(); ++i)
    {
      s >> m_FloatParams[i].m_sName;
      s >> m_FloatParams[i].m_Value;
    }

    m_bFloatParamsChanged = numFloats > 0;

    s >> numColors;
    m_ColorParams.SetCountUninitialized(numColors);

    for (ezUInt32 i = 0; i < m_ColorParams.GetCount(); ++i)
    {
      s >> m_ColorParams[i].m_sName;
      s >> m_ColorParams[i].m_Value;
    }

    m_bColorParamsChanged = numColors > 0;
  }

  if (uiVersion >= 3)
  {
    s >> m_OnFinishedAction;
  }
}

bool ezParticleComponent::StartEffect()
{
  // stop any previous effect
  HandOffToFinisher();

  if (m_hEffectResource.IsValid())
  {
    ezParticleWorldModule* pModule = GetWorld()->GetOrCreateModule<ezParticleWorldModule>();

    m_EffectController.Create(m_hEffectResource, pModule, m_uiRandomSeed, m_sSharedInstanceName, this, m_FloatParams, m_ColorParams);

    m_EffectController.SetTransform(GetOwner()->GetGlobalTransform(), GetOwner()->GetVelocity());

    m_bFloatParamsChanged = false;
    m_bColorParamsChanged = false;

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


void ezParticleComponent::OnSetPlaying(ezMsgSetPlaying& msg)
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
    m_LastBVolumeUpdate = m_EffectController.GetBoundingVolume(volume);

    if (!m_LastBVolumeUpdate.IsZero())
    {
      bounds.ExpandToInclude(volume);
      return EZ_SUCCESS;
    }
  }

  return EZ_FAILURE;
}


void ezParticleComponent::OnExtractRenderData(ezMsgExtractRenderData& msg) const
{
  // do not extract particles during shadow map rendering
  if (msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Shadow)
    return;

  m_EffectController.SetIsInView();
}

void ezParticleComponent::Update()
{
  if (!m_EffectController.IsAlive() && m_bSpawnAtStart)
  {
    if (StartEffect())
    {
      m_bSpawnAtStart = false;

      if (m_EffectController.IsContinuousEffect())
      {
        if (m_bIfContinuousStopRightAway)
        {
          StopEffect();
        }
        else
        {
          m_bSpawnAtStart = true;
        }
      }
    }
  }

  if (!m_EffectController.IsAlive() && (m_OnFinishedAction == ezOnComponentFinishedAction2::Restart))
  {
    const ezTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

    if (m_RestartTime == ezTime())
    {
      const ezTime tDiff = ezTime::Seconds(
          GetWorld()->GetRandomNumberGenerator().DoubleInRange(m_MinRestartDelay.GetSeconds(), m_RestartDelayRange.GetSeconds()));

      m_RestartTime = tNow + tDiff;
    }
    else if (m_RestartTime <= tNow)
    {
      m_RestartTime.SetZero();
      StartEffect();
    }
  }

  if (m_EffectController.IsAlive())
  {
    if (m_bFloatParamsChanged)
    {
      m_bFloatParamsChanged = false;

      for (ezUInt32 i = 0; i < m_FloatParams.GetCount(); ++i)
      {
        const auto& e = m_FloatParams[i];
        m_EffectController.SetParameter(e.m_sName, e.m_Value);
      }
    }

    if (m_bColorParamsChanged)
    {
      m_bColorParamsChanged = false;

      for (ezUInt32 i = 0; i < m_ColorParams.GetCount(); ++i)
      {
        const auto& e = m_ColorParams[i];
        m_EffectController.SetParameter(e.m_sName, e.m_Value);
      }
    }

    m_EffectController.SetTransform(GetOwner()->GetGlobalTransform(), GetOwner()->GetVelocity());

    CheckBVolumeUpdate();
  }
  else
  {
    if (m_OnFinishedAction == ezOnComponentFinishedAction2::DeleteGameObject)
    {
      GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
    }
    else if (m_OnFinishedAction == ezOnComponentFinishedAction2::DeleteComponent)
    {
      GetOwningManager()->DeleteComponent(GetHandle());
    }
  }
}

void ezParticleComponent::HandOffToFinisher()
{
  if (m_EffectController.IsAlive() && !m_EffectController.IsSharedInstance())
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
    ezParticleFinisherComponent::CreateComponent(pFinisher, pFinisherComp);

    pFinisherComp->m_EffectController = m_EffectController;
    pFinisherComp->m_EffectController.SetTransform(transform, ezVec3::ZeroVector()); // clear the velocity
  }

  // this will instruct the effect instance to cancel any further emission, thus continuous effects will stop in finite time
  m_EffectController.Invalidate();
}

void ezParticleComponent::CheckBVolumeUpdate()
{
  ezBoundingBoxSphere bvol;
  if (m_LastBVolumeUpdate < m_EffectController.GetBoundingVolume(bvol))
  {
    TriggerLocalBoundsUpdate();
  }
}

const ezRangeView<const char*, ezUInt32> ezParticleComponent::GetParameters() const
{
  return ezRangeView<const char*, ezUInt32>([this]() -> ezUInt32 { return 0; },
                                            [this]() -> ezUInt32 { return m_FloatParams.GetCount() + m_ColorParams.GetCount(); },
                                            [this](ezUInt32& it) { ++it; },
                                            [this](const ezUInt32& it) -> const char* {
                                              if (it < m_FloatParams.GetCount())
                                                return m_FloatParams[it].m_sName.GetData();
                                              else
                                                return m_ColorParams[it - m_FloatParams.GetCount()].m_sName.GetData();
                                            });
}

void ezParticleComponent::SetParameter(const char* szKey, const ezVariant& var)
{
  const ezTempHashedString th(szKey);
  if (var.CanConvertTo<float>())
  {
    float value = var.ConvertTo<float>();

    for (ezUInt32 i = 0; i < m_FloatParams.GetCount(); ++i)
    {
      if (m_FloatParams[i].m_sName == th)
      {
        if (m_FloatParams[i].m_Value != value)
        {
          m_bFloatParamsChanged = true;
          m_FloatParams[i].m_Value = value;
        }
        return;
      }
    }

    m_bFloatParamsChanged = true;
    auto& e = m_FloatParams.ExpandAndGetRef();
    e.m_sName.Assign(szKey);
    e.m_Value = value;

    return;
  }

  if (var.CanConvertTo<ezColor>())
  {
    ezColor value = var.ConvertTo<ezColor>();

    for (ezUInt32 i = 0; i < m_ColorParams.GetCount(); ++i)
    {
      if (m_ColorParams[i].m_sName == th)
      {
        if (m_ColorParams[i].m_Value != value)
        {
          m_bColorParamsChanged = true;
          m_ColorParams[i].m_Value = value;
        }
        return;
      }
    }

    m_bColorParamsChanged = true;
    auto& e = m_ColorParams.ExpandAndGetRef();
    e.m_sName.Assign(szKey);
    e.m_Value = value;

    return;
  }
}

void ezParticleComponent::RemoveParameter(const char* szKey)
{
  const ezTempHashedString th(szKey);

  for (ezUInt32 i = 0; i < m_FloatParams.GetCount(); ++i)
  {
    if (m_FloatParams[i].m_sName == th)
    {
      m_FloatParams.RemoveAtSwap(i);
      return;
    }
  }

  for (ezUInt32 i = 0; i < m_ColorParams.GetCount(); ++i)
  {
    if (m_ColorParams[i].m_sName == th)
    {
      m_ColorParams.RemoveAtSwap(i);
      return;
    }
  }
}

bool ezParticleComponent::GetParameter(const char* szKey, ezVariant& out_value) const
{
  const ezTempHashedString th(szKey);

  for (const auto& e : m_FloatParams)
  {
    if (e.m_sName == th)
    {
      out_value = e.m_Value;
      return true;
    }
  }
  for (const auto& e : m_ColorParams)
  {
    if (e.m_sName == th)
    {
      out_value = e.m_Value;
      return true;
    }
  }
  return false;
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Components_ParticleComponent);
