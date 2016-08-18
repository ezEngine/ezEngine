#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_SpherePosition.h>
#include <CoreUtils/DataProcessing/Stream/StreamGroup.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializerFactory_SpherePosition, 1, ezRTTIDefaultAllocator<ezParticleInitializerFactory_SpherePosition>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(0.25f)),
    EZ_MEMBER_PROPERTY("On Surface", m_bSpawnOnSurface),
    EZ_MEMBER_PROPERTY("Set Velocity", m_bSetVelocity),
    EZ_MEMBER_PROPERTY("Min Speed", m_fMinSpeed),
    EZ_MEMBER_PROPERTY("Speed Range", m_fSpeedRange),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializer_SpherePosition, 1, ezRTTIDefaultAllocator<ezParticleInitializer_SpherePosition>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleInitializerFactory_SpherePosition::ezParticleInitializerFactory_SpherePosition()
{
  m_fRadius = 0.25f;
  m_bSpawnOnSurface = false;
  m_bSetVelocity = false;
  m_fMinSpeed = 0;
  m_fSpeedRange = 0;
}

const ezRTTI* ezParticleInitializerFactory_SpherePosition::GetInitializerType() const
{
  return ezGetStaticRTTI<ezParticleInitializer_SpherePosition>();
}

void ezParticleInitializerFactory_SpherePosition::CopyInitializerProperties(ezParticleInitializer* pInitializer0) const
{
  ezParticleInitializer_SpherePosition* pInitializer = static_cast<ezParticleInitializer_SpherePosition*>(pInitializer0);

  pInitializer->m_fRadius = ezMath::Max(m_fRadius, 0.02f); // prevent 0 radius
  pInitializer->m_bSpawnOnSurface = m_bSpawnOnSurface;
  pInitializer->m_bSetVelocity = m_bSetVelocity;
  pInitializer->m_fMinSpeed = m_fMinSpeed;
  pInitializer->m_fSpeedRange = m_fSpeedRange;
}

void ezParticleInitializerFactory_SpherePosition::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 1;
  stream << uiVersion;

  stream << m_fRadius;
  stream << m_bSpawnOnSurface;
  stream << m_bSetVelocity;
  stream << m_fMinSpeed;
  stream << m_fSpeedRange;
}

void ezParticleInitializerFactory_SpherePosition::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_fRadius;
  stream >> m_bSpawnOnSurface;
  stream >> m_bSetVelocity;
  stream >> m_fMinSpeed;
  stream >> m_fSpeedRange;
}

void ezParticleInitializer_SpherePosition::SpawnElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  const ezUInt64 uiElementSize = m_pStreamPosition->GetElementSize();

  ezVec3* pPosition = m_pStreamPosition->GetWritableData<ezVec3>();
  ezVec3* pVelocity = m_pStreamVelocity->GetWritableData<ezVec3>();

  ezRandom& rng = m_pOwnerSystem->GetRNG();

  const float fRadiusSqr = m_fRadius * m_fRadius;

  for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    ezVec3 pos;
    float len = 0.0f;

    do
    {
      pos.x = (float)rng.DoubleMinMax(-m_fRadius, m_fRadius);
      pos.y = (float)rng.DoubleMinMax(-m_fRadius, m_fRadius);
      pos.z = (float)rng.DoubleMinMax(-m_fRadius, m_fRadius);

      len = pos.GetLengthSquared();
    }
    while (len > fRadiusSqr || len <= 0.000001f); // prevent spawning at the exact center (note: this has to be smaller than the minimum allowed radius sqr)

    ezVec3 normalPos = pos;

    if (m_bSpawnOnSurface || m_bSetVelocity)
    {
      normalPos.Normalize();
    }

    if (m_bSpawnOnSurface)
      pos = normalPos * m_fRadius;

    if (m_bSetVelocity)
    {
      const float fSpeed = (float)rng.DoubleInRange(m_fMinSpeed, m_fSpeedRange);

      /// \todo Ignore scale ?
      pVelocity[i] = m_pOwnerSystem->GetTransform().m_Rotation * normalPos * fSpeed;
    }

    pPosition[i] = m_pOwnerSystem->GetTransform() * pos;
  }
}
