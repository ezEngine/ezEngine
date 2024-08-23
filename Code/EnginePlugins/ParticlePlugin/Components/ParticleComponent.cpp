#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/DeleteObjectMessage.h>
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

ezParticleComponentManager::ezParticleComponentManager(ezWorld* pWorld)
  : SUPER(pWorld)
{
}

void ezParticleComponentManager::Initialize()
{
  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezParticleComponentManager::Update, this);
    desc.m_bOnlyUpdateWhenSimulating = true;
    RegisterUpdateFunction(desc);
  }
}

void ezParticleComponentManager::Update(const ezWorldModule::UpdateContext& context)
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

void ezParticleComponentManager::UpdatePfxTransformsAndBounds()
{
  for (auto it = this->m_ComponentStorage.GetIterator(); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->UpdatePfxTransformAndBounds();

      // This function is called in the post-transform phase so the global bounds and transform have already been calculated at this point.
      // Therefore we need to manually update the global bounds again to ensure correct bounds for culling and rendering.
      pComponent->GetOwner()->UpdateLocalBounds();
      pComponent->GetOwner()->UpdateGlobalBounds();
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezParticleComponent, 5, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Effect", GetParticleEffectFile, SetParticleEffectFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Particle_Effect", ezDependencyFlags::Package)),
    EZ_MEMBER_PROPERTY("SpawnAtStart", m_bSpawnAtStart)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ENUM_MEMBER_PROPERTY("OnFinishedAction", ezOnComponentFinishedAction2, m_OnFinishedAction),
    EZ_MEMBER_PROPERTY("MinRestartDelay", m_MinRestartDelay),
    EZ_MEMBER_PROPERTY("RestartDelayRange", m_RestartDelayRange),
    EZ_MEMBER_PROPERTY("RandomSeed", m_uiRandomSeed),
    EZ_ENUM_MEMBER_PROPERTY("SpawnDirection", ezBasisAxis, m_SpawnDirection)->AddAttributes(new ezDefaultValueAttribute((ezInt32)ezBasisAxis::PositiveZ)),
    EZ_MEMBER_PROPERTY("IgnoreOwnerRotation", m_bIgnoreOwnerRotation),
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
    EZ_MESSAGE_HANDLER(ezMsgSetPlaying, OnMsgSetPlaying),
    EZ_MESSAGE_HANDLER(ezMsgInterruptPlaying, OnMsgInterruptPlaying),
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
    EZ_MESSAGE_HANDLER(ezMsgDeleteGameObject, OnMsgDeleteGameObject),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(StartEffect),
    EZ_SCRIPT_FUNCTION_PROPERTY(StopEffect),
    EZ_SCRIPT_FUNCTION_PROPERTY(InterruptEffect),
    EZ_SCRIPT_FUNCTION_PROPERTY(IsEffectActive),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezParticleComponent::ezParticleComponent() = default;
ezParticleComponent::~ezParticleComponent() = default;

void ezParticleComponent::OnDeactivated()
{
  m_EffectController.Invalidate();

  ezRenderComponent::OnDeactivated();
}

void ezParticleComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  auto& s = inout_stream.GetStream();

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

  // version 4
  s << m_bIgnoreOwnerRotation;

  // version 5
  s << m_SpawnDirection;

  /// \todo store effect state
}

void ezParticleComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  auto& s = inout_stream.GetStream();
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

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

  if (uiVersion >= 4)
  {
    s >> m_bIgnoreOwnerRotation;
  }

  if (uiVersion >= 5)
  {
    s >> m_SpawnDirection;
  }
}

bool ezParticleComponent::StartEffect()
{
  // stop any previous effect
  m_EffectController.Invalidate();

  if (m_hEffectResource.IsValid())
  {
    ezParticleWorldModule* pModule = GetWorld()->GetOrCreateModule<ezParticleWorldModule>();

    m_EffectController.Create(m_hEffectResource, pModule, m_uiRandomSeed, m_sSharedInstanceName, this, m_FloatParams, m_ColorParams);

    UpdatePfxTransformAndBounds();

    m_bFloatParamsChanged = false;
    m_bColorParamsChanged = false;

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
}

bool ezParticleComponent::IsEffectActive() const
{
  return m_EffectController.IsAlive();
}

void ezParticleComponent::OnMsgSetPlaying(ezMsgSetPlaying& ref_msg)
{
  if (ref_msg.m_bPlay)
  {
    StartEffect();
  }
  else
  {
    StopEffect();
  }
}

void ezParticleComponent::OnMsgInterruptPlaying(ezMsgInterruptPlaying& ref_msg)
{
  InterruptEffect();
}

void ezParticleComponent::SetParticleEffect(const ezParticleEffectResourceHandle& hEffect)
{
  m_EffectController.Invalidate();

  m_hEffectResource = hEffect;

  TriggerLocalBoundsUpdate();
}


void ezParticleComponent::SetParticleEffectFile(ezStringView sFile)
{
  ezParticleEffectResourceHandle hEffect;

  if (!sFile.IsEmpty())
  {
    hEffect = ezResourceManager::LoadResource<ezParticleEffectResource>(sFile);
  }

  SetParticleEffect(hEffect);
}


ezStringView ezParticleComponent::GetParticleEffectFile() const
{
  if (!m_hEffectResource.IsValid())
    return "";

  return m_hEffectResource.GetResourceID();
}


ezResult ezParticleComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  if (m_EffectController.IsAlive())
  {
    ezBoundingBoxSphere volume = ezBoundingBoxSphere::MakeInvalid();

    m_EffectController.GetBoundingVolume(volume);

    if (volume.IsValid())
    {
      if (m_SpawnDirection != ezBasisAxis::PositiveZ)
      {
        const ezQuat qRot = ezBasisAxis::GetBasisRotation(ezBasisAxis::PositiveZ, m_SpawnDirection);
        volume.Transform(qRot.GetAsMat4());
      }

      if (m_bIgnoreOwnerRotation)
      {
        volume.Transform((GetOwner()->GetGlobalRotation().GetInverse()).GetAsMat4());
      }

      ref_bounds = volume;
      return EZ_SUCCESS;
    }
  }

  return EZ_FAILURE;
}


void ezParticleComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  // do not extract particles during shadow map rendering
  if (msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Shadow)
    return;

  m_EffectController.ExtractRenderData(msg, GetPfxTransform());
}

void ezParticleComponent::OnMsgDeleteGameObject(ezMsgDeleteGameObject& msg)
{
  ezOnComponentFinishedAction2::HandleDeleteObjectMsg(msg, m_OnFinishedAction);
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
      const ezTime tDiff = ezTime::MakeFromSeconds(GetWorld()->GetRandomNumberGenerator().DoubleInRange(m_MinRestartDelay.GetSeconds(), m_RestartDelayRange.GetSeconds()));

      m_RestartTime = tNow + tDiff;
    }
    else if (m_RestartTime <= tNow)
    {
      m_RestartTime = ezTime::MakeZero();
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

    m_EffectController.UpdateWindSamples(GetWorld()->GetClock().GetTimeDiff());
  }
  else
  {
    ezOnComponentFinishedAction2::HandleFinishedAction(this, m_OnFinishedAction);
  }
}

const ezRangeView<const char*, ezUInt32> ezParticleComponent::GetParameters() const
{
  return ezRangeView<const char*, ezUInt32>([this]() -> ezUInt32
    { return 0; },
    [this]() -> ezUInt32
    { return m_FloatParams.GetCount() + m_ColorParams.GetCount(); },
    [this](ezUInt32& ref_uiIt)
    { ++ref_uiIt; },
    [this](const ezUInt32& uiIt) -> const char*
    {
      if (uiIt < m_FloatParams.GetCount())
        return m_FloatParams[uiIt].m_sName.GetData();
      else
        return m_ColorParams[uiIt - m_FloatParams.GetCount()].m_sName.GetData();
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
      m_FloatParams.RemoveAtAndSwap(i);
      return;
    }
  }

  for (ezUInt32 i = 0; i < m_ColorParams.GetCount(); ++i)
  {
    if (m_ColorParams[i].m_sName == th)
    {
      m_ColorParams.RemoveAtAndSwap(i);
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

ezTransform ezParticleComponent::GetPfxTransform() const
{
  ezTransform transform = GetOwner()->GetGlobalTransform();

  const ezQuat qRot = ezBasisAxis::GetBasisRotation(ezBasisAxis::PositiveZ, m_SpawnDirection);

  if (m_bIgnoreOwnerRotation)
  {
    transform.m_qRotation = qRot;
  }
  else
  {
    transform.m_qRotation = transform.m_qRotation * qRot;
  }

  return transform;
}

void ezParticleComponent::UpdatePfxTransformAndBounds()
{
  m_EffectController.SetTransform(GetPfxTransform(), GetOwner()->GetLinearVelocity());
  m_EffectController.CombineSystemBoundingVolumes();
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Components_ParticleComponent);
