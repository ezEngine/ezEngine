#include <PCH.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_CylinderPosition.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <Foundation/Profiling/Profiling.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializerFactory_CylinderPosition, 1, ezRTTIDefaultAllocator<ezParticleInitializerFactory_CylinderPosition>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(0.25f)),
    EZ_MEMBER_PROPERTY("Height", m_fHeight)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("OnSurface", m_bSpawnOnSurface),
    EZ_MEMBER_PROPERTY("SetVelocity", m_bSetVelocity),
    EZ_MEMBER_PROPERTY("Speed", m_Speed),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializer_CylinderPosition, 1, ezRTTIDefaultAllocator<ezParticleInitializer_CylinderPosition>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleInitializerFactory_CylinderPosition::ezParticleInitializerFactory_CylinderPosition()
{
  m_fRadius = 0.25f;
  m_fHeight = 1.0f;
  m_bSpawnOnSurface = false;
  m_bSetVelocity = false;
}

const ezRTTI* ezParticleInitializerFactory_CylinderPosition::GetInitializerType() const
{
  return ezGetStaticRTTI<ezParticleInitializer_CylinderPosition>();
}

void ezParticleInitializerFactory_CylinderPosition::CopyInitializerProperties(ezParticleInitializer* pInitializer0) const
{
  ezParticleInitializer_CylinderPosition* pInitializer = static_cast<ezParticleInitializer_CylinderPosition*>(pInitializer0);

  pInitializer->m_fRadius = ezMath::Max(m_fRadius, 0.02f); // prevent 0 radius
  pInitializer->m_fHeight = ezMath::Max(m_fHeight, 0.0f);
  pInitializer->m_bSpawnOnSurface = m_bSpawnOnSurface;
  pInitializer->m_bSetVelocity = m_bSetVelocity;
  pInitializer->m_Speed = m_Speed;
}

void ezParticleInitializerFactory_CylinderPosition::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 1;
  stream << uiVersion;

  stream << m_fRadius;
  stream << m_fHeight;
  stream << m_bSpawnOnSurface;
  stream << m_bSetVelocity;
  stream << m_Speed.m_Value;
  stream << m_Speed.m_fVariance;
}

void ezParticleInitializerFactory_CylinderPosition::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_fRadius;
  stream >> m_fHeight;
  stream >> m_bSpawnOnSurface;
  stream >> m_bSetVelocity;
  stream >> m_Speed.m_Value;
  stream >> m_Speed.m_fVariance;
}

void ezParticleInitializer_CylinderPosition::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, true);

  if (m_bSetVelocity)
  {
    CreateStream("Velocity", ezProcessingStream::DataType::Float3, &m_pStreamVelocity, true);
  }
}

void ezParticleInitializer_CylinderPosition::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  EZ_PROFILE("PFX: Cylinder Position");

  ezVec4* pPosition = m_pStreamPosition->GetWritableData<ezVec4>();
  ezVec3* pVelocity = m_bSetVelocity ? m_pStreamVelocity->GetWritableData<ezVec3>() : nullptr;

  ezRandom& rng = GetRNG();

  const float fRadiusSqr = m_fRadius * m_fRadius;

  for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    ezVec3 pos;
    float len = 0.0f;
    pos.z = 0.0f;

    do
    {
      pos.x = (float)rng.DoubleMinMax(-m_fRadius, m_fRadius);
      pos.y = (float)rng.DoubleMinMax(-m_fRadius, m_fRadius);

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

    if (m_fHeight > 0)
    {
      pos.z = (float)rng.DoubleMinMax(0, m_fHeight);
    }

    if (m_bSetVelocity)
    {
      const float fSpeed = (float)rng.DoubleVariance(m_Speed.m_Value, m_Speed.m_fVariance);

      /// \todo Ignore scale ?
      pVelocity[i] = GetOwnerSystem()->GetTransform().m_qRotation * normalPos * fSpeed;
    }

    pPosition[i] = (GetOwnerSystem()->GetTransform() * pos).GetAsVec4(0);
  }
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_CylinderPosition);

