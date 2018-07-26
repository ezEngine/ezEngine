#include <PCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_CylinderPosition.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializerFactory_CylinderPosition, 1, ezRTTIDefaultAllocator<ezParticleInitializerFactory_CylinderPosition>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("PositionOffset", m_vPositionOffset),
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(0.25f), new ezClampValueAttribute(0.01f, 100.0f)),
    EZ_MEMBER_PROPERTY("Height", m_fHeight)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 100.0f)),
    EZ_MEMBER_PROPERTY("OnSurface", m_bSpawnOnSurface),
    EZ_MEMBER_PROPERTY("SetVelocity", m_bSetVelocity),
    EZ_MEMBER_PROPERTY("Speed", m_Speed),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCylinderVisualizerAttribute(ezBasisAxis::PositiveZ, "Height", "Radius", nullptr, ezColor::MediumVioletRed, "PositionOffset")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializer_CylinderPosition, 1, ezRTTIDefaultAllocator<ezParticleInitializer_CylinderPosition>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleInitializerFactory_CylinderPosition::ezParticleInitializerFactory_CylinderPosition()
{
  m_vPositionOffset.SetZero();
  m_fRadius = 0.25f;
  m_fHeight = 1.0f;
  m_bSpawnOnSurface = false;
  m_bSetVelocity = false;
}

const ezRTTI* ezParticleInitializerFactory_CylinderPosition::GetInitializerType() const
{
  return ezGetStaticRTTI<ezParticleInitializer_CylinderPosition>();
}

void ezParticleInitializerFactory_CylinderPosition::CopyInitializerProperties(ezParticleInitializer* pInitializer0, bool bFirstTime) const
{
  ezParticleInitializer_CylinderPosition* pInitializer = static_cast<ezParticleInitializer_CylinderPosition*>(pInitializer0);

  pInitializer->m_vPositionOffset = m_vPositionOffset;
  pInitializer->m_fRadius = ezMath::Max(m_fRadius, 0.01f); // prevent 0 radius
  pInitializer->m_fHeight = ezMath::Max(m_fHeight, 0.0f);
  pInitializer->m_bSpawnOnSurface = m_bSpawnOnSurface;
  pInitializer->m_bSetVelocity = m_bSetVelocity;
  pInitializer->m_Speed = m_Speed;
}

void ezParticleInitializerFactory_CylinderPosition::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 2;
  stream << uiVersion;

  stream << m_fRadius;
  stream << m_fHeight;
  stream << m_bSpawnOnSurface;
  stream << m_bSetVelocity;
  stream << m_Speed.m_Value;
  stream << m_Speed.m_fVariance;

  // version 2
  stream << m_vPositionOffset;
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

  if (uiVersion >= 2)
  {
    stream >> m_vPositionOffset;
  }
}

void ezParticleInitializerFactory_CylinderPosition::QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_FinalizerDeps) const
{
  if (m_bSetVelocity)
  {
    inout_FinalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizerFactory_ApplyVelocity>());
  }
}

//////////////////////////////////////////////////////////////////////////

void ezParticleInitializer_CylinderPosition::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, true);

  m_pStreamVelocity = nullptr;

  if (m_bSetVelocity)
  {
    CreateStream("Velocity", ezProcessingStream::DataType::Float3, &m_pStreamVelocity, true);
  }
}

void ezParticleInitializer_CylinderPosition::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  EZ_PROFILE("PFX: Cylinder Position");

  const ezVec3 startVel = GetOwnerSystem()->GetParticleStartVelocity();

  ezVec4* pPosition = m_pStreamPosition->GetWritableData<ezVec4>();
  ezVec3* pVelocity = m_bSetVelocity ? m_pStreamVelocity->GetWritableData<ezVec3>() : nullptr;

  ezRandom& rng = GetRNG();

  const float fRadiusSqr = m_fRadius * m_fRadius;
  const float fHalfHeight = m_fHeight * 0.5f;

  const ezTransform trans = GetOwnerSystem()->GetTransform();

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
    } while (len > fRadiusSqr ||
             len <= 0.000001f); // prevent spawning at the exact center (note: this has to be smaller than the minimum allowed radius sqr)

    ezVec3 normalPos = pos;

    if (m_bSpawnOnSurface || m_bSetVelocity)
    {
      normalPos.Normalize();
    }

    if (m_bSpawnOnSurface)
      pos = normalPos * m_fRadius;

    if (m_fHeight > 0)
    {
      pos.z = (float)rng.DoubleMinMax(-fHalfHeight, fHalfHeight);
    }

    pos += m_vPositionOffset;

    if (m_bSetVelocity)
    {
      const float fSpeed = (float)rng.DoubleVariance(m_Speed.m_Value, m_Speed.m_fVariance);

      pVelocity[i] = startVel + trans.m_qRotation * normalPos * fSpeed;
    }

    pPosition[i] = (trans * pos).GetAsVec4(0);
  }
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_CylinderPosition);
