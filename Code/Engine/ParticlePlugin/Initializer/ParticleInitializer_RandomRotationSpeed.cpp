#include <PCH.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_RandomRotationSpeed.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <GameEngine/Curves/Curve1DResource.h>
#include <Core/WorldSerializer/ResourceHandleReader.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializerFactory_RandomRotationSpeed, 1, ezRTTIDefaultAllocator<ezParticleInitializerFactory_RandomRotationSpeed>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("DegreesPerSecond", m_RotationSpeed)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(90)), new ezClampValueAttribute(ezAngle::Degree(0), ezVariant())),
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

  pInitializer->m_RotationSpeed = m_RotationSpeed;
}

void ezParticleInitializerFactory_RandomRotationSpeed::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 1;
  stream << uiVersion;

  stream << m_RotationSpeed.m_Value;
  stream << m_RotationSpeed.m_fVariance;
}

void ezParticleInitializerFactory_RandomRotationSpeed::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_RotationSpeed.m_Value;
  stream >> m_RotationSpeed.m_fVariance;
}


void ezParticleInitializer_RandomRotationSpeed::CreateRequiredStreams()
{
  CreateStream("RotationSpeed", ezProcessingStream::DataType::Float, &m_pStreamRotationSpeed, true);
}

void ezParticleInitializer_RandomRotationSpeed::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  EZ_PROFILE("PFX: Random Rotation");

  float* pSpeed = m_pStreamRotationSpeed->GetWritableData<float>();

  ezRandom& rng = GetRNG();

  for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    pSpeed[i] = (float)rng.DoubleVariance(m_RotationSpeed.m_Value.GetRadian(), m_RotationSpeed.m_fVariance);

    // this can certainly be done more elegantly
    if (ezMath::IsEven(rng.UInt()))
      pSpeed[i] = -pSpeed[i];
  }
}




EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_RandomRotationSpeed);

