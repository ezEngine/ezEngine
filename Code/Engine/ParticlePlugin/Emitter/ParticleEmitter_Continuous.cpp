#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_Continuous.h>
#include <CoreUtils/DataProcessing/Stream/StreamGroup.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <Foundation/Time/Clock.h>
#include <Core/World/World.h>
#include <GameUtils/Curves/Curve1DResource.h>
#include <Core/WorldSerializer/ResourceHandleReader.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEmitterFactory_Continuous, 1, ezRTTIDefaultAllocator<ezParticleEmitterFactory_Continuous>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Duration", m_Duration),

    EZ_MEMBER_PROPERTY("Min Spawn Count", m_uiSpawnCountMin)->AddAttributes(new ezDefaultValueAttribute(1)),
    EZ_MEMBER_PROPERTY("Spawn Count Range", m_uiSpawnCountRange),

    EZ_MEMBER_PROPERTY("Min Interval", m_SpawnIntervalMin)->AddAttributes(new ezDefaultValueAttribute(ezTime::Seconds(0.1))),
    EZ_MEMBER_PROPERTY("Interval Range", m_SpawnIntervalRange),

    EZ_ACCESSOR_PROPERTY("Count Curve", GetCountCurveFile, SetCountCurveFile)->AddAttributes(new ezAssetBrowserAttribute("Curve1D")),
    EZ_MEMBER_PROPERTY("Curve Duration", m_CurveDuration)->AddAttributes(new ezDefaultValueAttribute(ezTime::Seconds(10.0))),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEmitter_Continuous, 1, ezRTTIDefaultAllocator<ezParticleEmitter_Continuous>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleEmitterFactory_Continuous::ezParticleEmitterFactory_Continuous()
{
  m_uiSpawnCountMin = 1;
  m_uiSpawnCountRange = 0;

  m_SpawnIntervalMin = ezTime::Seconds(0.1);
  m_SpawnIntervalRange = ezTime();

  m_CurveDuration = ezTime::Seconds(10.0);
}


const ezRTTI* ezParticleEmitterFactory_Continuous::GetEmitterType() const
{
  return ezGetStaticRTTI<ezParticleEmitter_Continuous>();
}

void ezParticleEmitterFactory_Continuous::CopyEmitterProperties(ezParticleEmitter* pEmitter0) const
{
  ezParticleEmitter_Continuous* pEmitter = static_cast<ezParticleEmitter_Continuous*>(pEmitter0);

  pEmitter->m_Duration = m_Duration;

  pEmitter->m_uiSpawnCountMin = m_uiSpawnCountMin;
  pEmitter->m_uiSpawnCountRange = m_uiSpawnCountRange;

  pEmitter->m_SpawnIntervalMin = m_SpawnIntervalMin;
  pEmitter->m_SpawnIntervalRange = m_SpawnIntervalRange;

  pEmitter->m_hCountCurve = m_hCountCurve;
  pEmitter->m_CurveDuration = ezMath::Max(m_CurveDuration, ezTime::Seconds(1.0));
}

void ezParticleEmitterFactory_Continuous::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 3;
  stream << uiVersion;

  // Version 3
  stream << m_Duration;

  // Version 1
  stream << m_uiSpawnCountMin;
  stream << m_uiSpawnCountRange;
  stream << m_SpawnIntervalMin;
  stream << m_SpawnIntervalRange;

  // Version 2
  stream << m_hCountCurve;
  stream << m_CurveDuration;
}

void ezParticleEmitterFactory_Continuous::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= 3, "Invalid version %u", uiVersion);

  if (uiVersion >= 3)
  {
    stream >> m_Duration;
  }

  stream >> m_uiSpawnCountMin;
  stream >> m_uiSpawnCountRange;
  stream >> m_SpawnIntervalMin;
  stream >> m_SpawnIntervalRange;

  if (uiVersion >= 2)
  {
    stream >> m_hCountCurve;
    stream >> m_CurveDuration;
  }
}


void ezParticleEmitterFactory_Continuous::SetCountCurveFile(const char* szFile)
{
  ezCurve1DResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezCurve1DResource>(szFile);
  }

  m_hCountCurve = hResource;
}

const char* ezParticleEmitterFactory_Continuous::GetCountCurveFile() const
{
  if (!m_hCountCurve.IsValid())
    return "";

  return m_hCountCurve.GetResourceID();
}


void ezParticleEmitter_Continuous::CreateRequiredStreams()
{
  //CreateStream("Position", ezStream::DataType::Float3, &m_pStreamPosition);
  //CreateStream("Velocity", ezStream::DataType::Float3, &m_pStreamVelocity);
  //CreateStream("Color", ezStream::DataType::Float4, &m_pStreamColor);
}

void ezParticleEmitter_Continuous::SpawnElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  //ezVec3* pPosition = m_pStreamPosition->GetWritableData<ezVec3>();
  //ezVec3* pVelocity = m_pStreamVelocity->GetWritableData<ezVec3>();
  //ezColor* pColor = m_pStreamColor->GetWritableData<ezColor>();

  //ezRandom& rng = GetRNG();
  //const ezTransform transform = GetOwnerSystem()->GetTransform();

  //for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  //{
  //  pPosition[i] = transform.m_vPosition;
  //  pVelocity[i].SetZero();// = transform.m_Rotation * ezVec3(1, 3, 10);
  //  pColor[i] = ezColor::White;
  //}
}

ezUInt32 ezParticleEmitter_Continuous::ComputeSpawnCount(const ezTime& tDiff)
{
  if (m_Duration > ezTime::Seconds(0))
  {
    if (tDiff >= m_Duration)
      m_Duration = ezTime::Seconds(-1);
    else
      m_Duration -= tDiff;
  }

  m_NextSpawn -= tDiff;
  m_CountCurveTime += tDiff;

  if (m_NextSpawn > ezTime::Seconds(0))
    return 0;

  float fSpawnFactor = 1.0f;

  if (m_hCountCurve.IsValid())
  {
    ezResourceLock<ezCurve1DResource> pCurve(m_hCountCurve, ezResourceAcquireMode::NoFallback);

    if (!pCurve->GetDescriptor().m_Curves.IsEmpty())
    {
      while (m_CountCurveTime > m_CurveDuration)
        m_CountCurveTime -= m_CurveDuration;

      const auto& curve = pCurve->GetDescriptor().m_Curves[0];

      const float normPos = (float)(m_CountCurveTime.GetSeconds() / m_CurveDuration.GetSeconds());
      const float evalPos = curve.ConvertNormalizedPos(normPos);

      fSpawnFactor = ezMath::Max(0.0f, curve.Evaluate(evalPos));
    }
  }

  ezRandom& rng = GetRNG();

  const ezUInt32 uiSpawn = (ezUInt32)(rng.IntInRange(m_uiSpawnCountMin, 1 + m_uiSpawnCountRange) * fSpawnFactor);

  const ezTime interval = ezTime::Seconds(rng.DoubleInRange(m_SpawnIntervalMin.GetSeconds(), m_SpawnIntervalRange.GetSeconds()));

  // we ignore the fact that with lower update frequencies (bad framerate), the actual interval will become larger and the effect
  // might visibly change
  m_NextSpawn = interval;

  return uiSpawn;
}

