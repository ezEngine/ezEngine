#include <PCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_Age.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleFinalizerFactory_Age, 1, ezRTTIDefaultAllocator<ezParticleFinalizerFactory_Age>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleFinalizer_Age, 1, ezRTTIDefaultAllocator<ezParticleFinalizer_Age>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleFinalizerFactory_Age::ezParticleFinalizerFactory_Age() {}

const ezRTTI* ezParticleFinalizerFactory_Age::GetFinalizerType() const
{
  return ezGetStaticRTTI<ezParticleFinalizer_Age>();
}

void ezParticleFinalizerFactory_Age::CopyFinalizerProperties(ezParticleFinalizer* pObject) const
{
  ezParticleFinalizer_Age* pFinalizer = static_cast<ezParticleFinalizer_Age*>(pObject);

  pFinalizer->m_LifeTime = m_LifeTime;
  pFinalizer->m_sOnDeathEvent = ezTempHashedString(m_sOnDeathEvent.GetData());
  pFinalizer->m_sLifeScaleParameter = ezTempHashedString(m_sLifeScaleParameter.GetData());
}

ezParticleFinalizer_Age::ezParticleFinalizer_Age() = default;

ezParticleFinalizer_Age::~ezParticleFinalizer_Age()
{
  if (m_bHasOnDeathEventHandler)
  {
    GetOwnerSystem()->RemoveParticleDeathEventHandler(ezMakeDelegate(&ezParticleFinalizer_Age::OnParticleDeath, this));
  }
}

void ezParticleFinalizer_Age::CreateRequiredStreams()
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


void ezParticleFinalizer_Age::AfterPropertiesConfigured(bool bFirstTime)
{
  if (m_bHasOnDeathEventHandler)
  {
    m_bHasOnDeathEventHandler = false;
    GetOwnerSystem()->RemoveParticleDeathEventHandler(ezMakeDelegate(&ezParticleFinalizer_Age::OnParticleDeath, this));
  }

  if (m_sOnDeathEvent.GetHash() != 0)
  {
    m_bHasOnDeathEventHandler = true;
    GetOwnerSystem()->AddParticleDeathEventHandler(ezMakeDelegate(&ezParticleFinalizer_Age::OnParticleDeath, this));
  }
}

void ezParticleFinalizer_Age::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  EZ_PROFILE("PFX: Age Init");

  ezFloat16Vec2* pLifeTime = m_pStreamLifeTime->GetWritableData<ezFloat16Vec2>();
  const float fLifeScale = ezMath::Max(GetOwnerEffect()->GetFloatParameter(m_sLifeScaleParameter, 1.0f), 0.0f);

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

void ezParticleFinalizer_Age::Process(ezUInt64 uiNumElements)
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

void ezParticleFinalizer_Age::OnParticleDeath(const ezStreamGroupElementRemovedEvent& e)
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



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Finalizer_ParticleFinalizer_Age);
