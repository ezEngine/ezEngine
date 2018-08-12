#include <PCH.h>

#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_PullAlong.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_PullAlong, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_PullAlong>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Strength", m_fStrength)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_PullAlong, 1, ezRTTIDefaultAllocator<ezParticleBehavior_PullAlong>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleBehaviorFactory_PullAlong::ezParticleBehaviorFactory_PullAlong() {}

const ezRTTI* ezParticleBehaviorFactory_PullAlong::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_PullAlong>();
}

void ezParticleBehaviorFactory_PullAlong::CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const
{
  ezParticleBehavior_PullAlong* pBehavior = static_cast<ezParticleBehavior_PullAlong*>(pObject);

  pBehavior->m_fStrength = ezMath::Clamp(m_fStrength, 0.0f, 1.0f);
}

enum class BehaviorPullAlongVersion
{
  Version_0 = 0,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleBehaviorFactory_PullAlong::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)BehaviorPullAlongVersion::Version_Current;
  stream << uiVersion;

  stream << m_fStrength;
}

void ezParticleBehaviorFactory_PullAlong::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)BehaviorPullAlongVersion::Version_Current, "Invalid version {0}", uiVersion);

  stream >> m_fStrength;
}

void ezParticleBehavior_PullAlong::CreateRequiredStreams()
{
  m_bFirstTime = true;

  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);
}

void ezParticleBehavior_PullAlong::Process(ezUInt64 uiNumElements)
{
  EZ_PROFILE("PFX: PullAlong");

  if (m_vApplyPull.IsZero())
    return;

  ezProcessingStreamIterator<ezSimdVec4f> itPosition(m_pStreamPosition, uiNumElements - m_uiIgnoreNewParticles, 0);
  ezSimdVec4f pull;
  pull.Load<3>(&m_vApplyPull.x);

  while (!itPosition.HasReachedEnd())
  {
    itPosition.Current() += pull;

    itPosition.Advance();
  }
}

void ezParticleBehavior_PullAlong::StepParticleSystem(const ezTime& tDiff, ezUInt32 uiNumNewParticles)
{
  m_uiIgnoreNewParticles = uiNumNewParticles;
  const ezVec3 vPos = GetOwnerSystem()->GetTransform().m_vPosition;

  if (!m_bFirstTime)
  {
    m_vApplyPull = (vPos - m_vLastEmitterPosition) * m_fStrength;
  }
  else
  {
    m_bFirstTime = false;
    m_vApplyPull.SetZero();
  }

  m_vLastEmitterPosition = vPos;
}
