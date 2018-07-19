#include <PCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Age.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_Age, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_Age>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezHiddenAttribute()
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Age, 1, ezRTTIDefaultAllocator<ezParticleBehavior_Age>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleBehaviorFactory_Age::ezParticleBehaviorFactory_Age() {}

const ezRTTI* ezParticleBehaviorFactory_Age::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_Age>();
}

void ezParticleBehaviorFactory_Age::CopyBehaviorProperties(ezParticleBehavior* pObject) const
{
  ezParticleBehavior_Age* pBehavior = static_cast<ezParticleBehavior_Age*>(pObject);

  pBehavior->m_LifeTime = m_LifeTime;
  pBehavior->m_sOnDeathEvent = ezTempHashedString(m_sOnDeathEvent.GetData());
  pBehavior->m_sLifeScaleParameter = ezTempHashedString(m_sLifeScaleParameter.GetData());
}


enum class BehaviorAgeVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,
  Version_3,
  Version_4,
  Version_5, // added life scale param

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleBehaviorFactory_Age::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)BehaviorAgeVersion::Version_Current;
  stream << uiVersion;

  stream << m_sOnDeathEvent;

  stream << m_LifeTime.m_Value;
  stream << m_LifeTime.m_fVariance;

  // Version 5
  stream << m_sLifeScaleParameter;
}

void ezParticleBehaviorFactory_Age::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)BehaviorAgeVersion::Version_Current, "Invalid version {0}", uiVersion);

  if (uiVersion >= 2)
  {
    stream >> m_sOnDeathEvent;
  }

  if (uiVersion >= 3)
  {
    stream >> m_LifeTime.m_Value;
    stream >> m_LifeTime.m_fVariance;
  }

  if (uiVersion >= 5)
  {
    stream >> m_sLifeScaleParameter;
  }
}


ezParticleBehavior_Age::ezParticleBehavior_Age()
{
  m_bHasOnDeathEventHandler = false;
}

ezParticleBehavior_Age::~ezParticleBehavior_Age()
{
  if (m_bHasOnDeathEventHandler)
  {
    GetOwnerSystem()->RemoveParticleDeathEventHandler(ezMakeDelegate(&ezParticleBehavior_Age::OnParticleDeath, this));
  }
}

void ezParticleBehavior_Age::CreateRequiredStreams()
{
  CreateStream("LifeTime", ezProcessingStream::DataType::Half2, &m_pStreamLifeTime, true);

  m_pStreamPosition = nullptr;
  m_pStreamVelocity = nullptr;

  if (m_sOnDeathEvent.GetHash() != 0)
  {
    CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);
    CreateStream("Velocity", ezProcessingStream::DataType::Float3, &m_pStreamVelocity, false);
  }
}


void ezParticleBehavior_Age::AfterPropertiesConfigured(bool bFirstTime)
{
  if (m_bHasOnDeathEventHandler)
  {
    m_bHasOnDeathEventHandler = false;
    GetOwnerSystem()->RemoveParticleDeathEventHandler(ezMakeDelegate(&ezParticleBehavior_Age::OnParticleDeath, this));
  }

  if (m_sOnDeathEvent.GetHash() != 0)
  {
    m_bHasOnDeathEventHandler = true;
    GetOwnerSystem()->AddParticleDeathEventHandler(ezMakeDelegate(&ezParticleBehavior_Age::OnParticleDeath, this));
  }
}

void ezParticleBehavior_Age::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  EZ_PROFILE("PFX: Age Init");

  ezFloat16Vec2* pLifeTime = m_pStreamLifeTime->GetWritableData<ezFloat16Vec2>();
  const float fLifeScale = ezMath::Clamp(GetOwnerEffect()->GetFloatParameter(m_sLifeScaleParameter, 1.0f), 0.0f, 10.0f);

  if (m_LifeTime.m_fVariance == 0)
  {
    const float tLifeTime = (fLifeScale * (float)m_LifeTime.m_Value.GetSeconds()) + 0.01f; // make sure it's not zero
    const float tInvLifeTime = 1.0f / tLifeTime;

    for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pLifeTime[i].x = tLifeTime;
      pLifeTime[i].y = tInvLifeTime;
    }
  }
  else // random range
  {
    ezRandom& rng = GetRNG();

    for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      const float tLifeTime = (fLifeScale * (float)rng.DoubleVariance(m_LifeTime.m_Value.GetSeconds(), m_LifeTime.m_fVariance)) +
                              0.01f; // make sure it's not zero
      const float tInvLifeTime = 1.0f / tLifeTime;

      pLifeTime[i].x = tLifeTime;
      pLifeTime[i].y = tInvLifeTime;
    }
  }
}

void ezParticleBehavior_Age::Process(ezUInt64 uiNumElements)
{
  EZ_PROFILE("PFX: Age");

  ezFloat16Vec2* pLifeTime = m_pStreamLifeTime->GetWritableData<ezFloat16Vec2>();

  const float tDiff = (float)m_TimeDiff.GetSeconds();

  for (ezUInt32 i = 0; i < uiNumElements; ++i)
  {
    pLifeTime[i].x = pLifeTime[i].x - tDiff;

    if (pLifeTime[i].x <= 0)
    {
      pLifeTime[i].x = 0;

      /// \todo Get current element index from iterator ?
      m_pStreamGroup->RemoveElement(i);
    }
  }
}

void ezParticleBehavior_Age::OnParticleDeath(const ezStreamGroupElementRemovedEvent& e)
{
  const ezVec4* pPosition = m_pStreamPosition->GetData<ezVec4>();
  const ezVec3* pVelocity = m_pStreamVelocity->GetData<ezVec3>();

  ezParticleEvent pe;
  pe.m_EventType = m_sOnDeathEvent;
  pe.m_vPosition = pPosition[e.m_uiElementIndex].GetAsVec3();
  pe.m_vDirection = pVelocity[e.m_uiElementIndex];
  pe.m_vNormal.SetZero();

  GetOwnerEffect()->AddParticleEvent(pe);
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Age);
