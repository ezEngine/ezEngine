#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_VelocityCone.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializerFactory_VelocityCone, 1, ezRTTIDefaultAllocator<ezParticleInitializerFactory_VelocityCone>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Angle", m_Angle)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(45)), new ezClampValueAttribute(ezAngle::Degree(1), ezAngle::Degree(70))),
    EZ_MEMBER_PROPERTY("Min Speed", m_fMinSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Speed Range", m_fSpeedRange)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializer_VelocityCone, 1, ezRTTIDefaultAllocator<ezParticleInitializer_VelocityCone>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleInitializerFactory_VelocityCone::ezParticleInitializerFactory_VelocityCone()
{
  m_Angle = ezAngle::Degree(45);
  m_fMinSpeed = 1.0;
  m_fSpeedRange = 0;
}

const ezRTTI* ezParticleInitializerFactory_VelocityCone::GetInitializerType() const
{
  return ezGetStaticRTTI<ezParticleInitializer_VelocityCone>();
}

void ezParticleInitializerFactory_VelocityCone::CopyInitializerProperties(ezParticleInitializer* pInitializer0) const
{
  ezParticleInitializer_VelocityCone* pInitializer = static_cast<ezParticleInitializer_VelocityCone*>(pInitializer0);

  pInitializer->m_Angle = ezMath::Clamp(m_Angle, ezAngle::Degree(1), ezAngle::Degree(89));
  pInitializer->m_fMinSpeed = m_fMinSpeed;
  pInitializer->m_fSpeedRange = m_fSpeedRange;
}

void ezParticleInitializerFactory_VelocityCone::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 1;
  stream << uiVersion;

  stream << m_Angle;
  stream << m_fMinSpeed;
  stream << m_fSpeedRange;
}

void ezParticleInitializerFactory_VelocityCone::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_Angle;
  stream >> m_fMinSpeed;
  stream >> m_fSpeedRange;
}


void ezParticleInitializer_VelocityCone::CreateRequiredStreams()
{
  CreateStream("Velocity", ezProcessingStream::DataType::Float3, &m_pStreamVelocity);
}

void ezParticleInitializer_VelocityCone::SpawnElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  ezVec3* pVelocity = m_pStreamVelocity->GetWritableData<ezVec3>();

  ezRandom& rng = GetRNG();

  const float dist = 1.0f / ezMath::Tan(m_Angle);

  for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    ezVec3 dir;
    dir.z = 0;
    float len = 0.0f;

    do
    {
      dir.x = (float)rng.DoubleMinMax(-1.0, 1.0);
      dir.y = (float)rng.DoubleMinMax(-1.0, 1.0);

      len = dir.GetLengthSquared();
    }
    while (len > 1.0f);

    dir.z = dist;
    dir.Normalize();

    const float fSpeed = (float)rng.DoubleInRange(m_fMinSpeed, m_fSpeedRange);

    pVelocity[i] = GetOwnerSystem()->GetTransform().m_Rotation * dir * fSpeed;
  }
}
