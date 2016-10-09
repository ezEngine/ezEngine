#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_RandomRotationSpeed.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <GameUtils/Curves/Curve1DResource.h>
#include <Core/WorldSerializer/ResourceHandleReader.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializerFactory_RandomRotationSpeed, 1, ezRTTIDefaultAllocator<ezParticleInitializerFactory_RandomRotationSpeed>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Rotation Speed", m_RotationPerSecond)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(90)), new ezClampValueAttribute(ezAngle::Degree(0), ezVariant())),
    EZ_MEMBER_PROPERTY("Rotation Variance", m_fRotationVariance)->AddAttributes(new ezDefaultValueAttribute(0.25f), new ezClampValueAttribute(0.0f, 1.0f)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializer_RandomRotationSpeed, 1, ezRTTIDefaultAllocator<ezParticleInitializer_RandomRotationSpeed>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleInitializerFactory_RandomRotationSpeed::GetInitializerType() const
{
  return ezGetStaticRTTI<ezParticleInitializer_RandomRotationSpeed>();
}

void ezParticleInitializerFactory_RandomRotationSpeed::CopyInitializerProperties(ezParticleInitializer* pInitializer0) const
{
  ezParticleInitializer_RandomRotationSpeed* pInitializer = static_cast<ezParticleInitializer_RandomRotationSpeed*>(pInitializer0);

  pInitializer->m_RotationPerSecond = m_RotationPerSecond;
  pInitializer->m_fRotationVariance = m_fRotationVariance;
}

void ezParticleInitializerFactory_RandomRotationSpeed::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 1;
  stream << uiVersion;

  stream << m_RotationPerSecond;
  stream << m_fRotationVariance;
}

void ezParticleInitializerFactory_RandomRotationSpeed::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_RotationPerSecond;
  stream >> m_fRotationVariance;
}


void ezParticleInitializer_RandomRotationSpeed::CreateRequiredStreams()
{
  CreateStream("RotationSpeed", ezProcessingStream::DataType::Float, &m_pStreamRotationSpeed);
}

void ezParticleInitializer_RandomRotationSpeed::SpawnElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  float* pSpeed = m_pStreamRotationSpeed->GetWritableData<float>();

  ezRandom& rng = GetRNG();

  for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    const float fAvg = m_RotationPerSecond.GetRadian();
    const float fVar = fAvg * m_fRotationVariance;
    const float fMin = fAvg - fVar;
    const float fMax = fAvg + fVar;

    pSpeed[i] = (float)rng.DoubleInRange(fMin, fMax);

    // this can certainly be done more elegantly
    if ((rng.UInt() & 0x1) != 0)
      pSpeed[i] = -pSpeed[i];
  }
}


