#include <PCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_SpherePosition.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializerFactory_SpherePosition, 1, ezRTTIDefaultAllocator<ezParticleInitializerFactory_SpherePosition>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("PositionOffset", m_vPositionOffset),
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(0.25f), new ezClampValueAttribute(0.01f, 100.0f)),
    EZ_MEMBER_PROPERTY("OnSurface", m_bSpawnOnSurface),
    EZ_MEMBER_PROPERTY("SetVelocity", m_bSetVelocity),
    EZ_MEMBER_PROPERTY("Speed", m_Speed),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezSphereVisualizerAttribute("Radius", nullptr, ezColor::MediumVioletRed, "PositionOffset"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializer_SpherePosition, 1, ezRTTIDefaultAllocator<ezParticleInitializer_SpherePosition>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleInitializerFactory_SpherePosition::ezParticleInitializerFactory_SpherePosition()
{
  m_fRadius = 0.25f;
  m_vPositionOffset.SetZero();
  m_bSpawnOnSurface = false;
  m_bSetVelocity = false;
}

const ezRTTI* ezParticleInitializerFactory_SpherePosition::GetInitializerType() const
{
  return ezGetStaticRTTI<ezParticleInitializer_SpherePosition>();
}

void ezParticleInitializerFactory_SpherePosition::CopyInitializerProperties(ezParticleInitializer* pInitializer0) const
{
  ezParticleInitializer_SpherePosition* pInitializer = static_cast<ezParticleInitializer_SpherePosition*>(pInitializer0);

  pInitializer->m_fRadius = ezMath::Max(m_fRadius, 0.01f); // prevent 0 radius
  pInitializer->m_bSpawnOnSurface = m_bSpawnOnSurface;
  pInitializer->m_bSetVelocity = m_bSetVelocity;
  pInitializer->m_Speed = m_Speed;
  pInitializer->m_vPositionOffset = m_vPositionOffset;
}

void ezParticleInitializerFactory_SpherePosition::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 2;
  stream << uiVersion;

  stream << m_fRadius;
  stream << m_bSpawnOnSurface;
  stream << m_bSetVelocity;
  stream << m_Speed.m_Value;
  stream << m_Speed.m_fVariance;

  // version 2
  stream << m_vPositionOffset;
}

void ezParticleInitializerFactory_SpherePosition::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_fRadius;
  stream >> m_bSpawnOnSurface;
  stream >> m_bSetVelocity;
  stream >> m_Speed.m_Value;
  stream >> m_Speed.m_fVariance;

  if (uiVersion >= 2)
  {
    stream >> m_vPositionOffset;
  }
}

void ezParticleInitializerFactory_SpherePosition::QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_FinalizerDeps) const
{
  if (m_bSetVelocity)
  {
    inout_FinalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizerFactory_ApplyVelocity>());
  }
}

//////////////////////////////////////////////////////////////////////////

void ezParticleInitializer_SpherePosition::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, true);

  m_pStreamVelocity = nullptr;

  if (m_bSetVelocity)
  {
    CreateStream("Velocity", ezProcessingStream::DataType::Float3, &m_pStreamVelocity, true);
  }
}

void ezParticleInitializer_SpherePosition::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  EZ_PROFILE("PFX: Sphere Position");

  const ezVec3 startVel = GetOwnerSystem()->GetParticleStartVelocity();

  ezVec4* pPosition = m_pStreamPosition->GetWritableData<ezVec4>();
  ezVec3* pVelocity = m_bSetVelocity ? m_pStreamVelocity->GetWritableData<ezVec3>() : nullptr;

  ezRandom& rng = GetRNG();

  const ezTransform trans = GetOwnerSystem()->GetTransform();

  for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    ezVec3 pos = ezVec3::CreateRandomPointInSphere(rng) * m_fRadius;
    ezVec3 normalPos = pos;

    if (m_bSpawnOnSurface || m_bSetVelocity)
    {
      normalPos.Normalize();
    }

    if (m_bSpawnOnSurface)
      pos = normalPos * m_fRadius;

    pos += m_vPositionOffset;

    if (m_bSetVelocity)
    {
      const float fSpeed = (float)rng.DoubleVariance(m_Speed.m_Value, m_Speed.m_fVariance);

      pVelocity[i] = startVel + trans.m_qRotation * normalPos * fSpeed;
    }

    pPosition[i] = (trans * pos).GetAsVec4(0);
  }
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_SpherePosition);
