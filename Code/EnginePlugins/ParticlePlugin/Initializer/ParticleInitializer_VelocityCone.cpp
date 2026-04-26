#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_VelocityCone.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializerFactory_VelocityCone, 2, ezRTTIDefaultAllocator<ezParticleInitializerFactory_VelocityCone>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Angle", m_Angle)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(30)), new ezClampValueAttribute(ezAngle::MakeFromDegree(1), ezAngle::MakeFromDegree(89))),
    EZ_MEMBER_PROPERTY("Speed", m_Speed),
    EZ_MEMBER_PROPERTY("SpeedScaleParam", m_sSpeedScaleParameter)
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezConeVisualizerAttribute(ezBasisAxis::PositiveZ, "Angle", 1.0f, nullptr, ezColor::CornflowerBlue)
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializer_VelocityCone, 1, ezRTTIDefaultAllocator<ezParticleInitializer_VelocityCone>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleInitializerFactory_VelocityCone::ezParticleInitializerFactory_VelocityCone()
{
  m_Angle = ezAngle::MakeFromDegree(45);
}

const ezRTTI* ezParticleInitializerFactory_VelocityCone::GetInitializerType() const
{
  return ezGetStaticRTTI<ezParticleInitializer_VelocityCone>();
}

void ezParticleInitializerFactory_VelocityCone::CopyInitializerProperties(ezParticleInitializer* pInitializer0, bool bFirstTime) const
{
  ezParticleInitializer_VelocityCone* pInitializer = static_cast<ezParticleInitializer_VelocityCone*>(pInitializer0);

  pInitializer->m_Angle = ezMath::Clamp(m_Angle, ezAngle::MakeFromDegree(1), ezAngle::MakeFromDegree(89));
  pInitializer->m_Speed = m_Speed;
  pInitializer->m_sSpeedScaleParameter = ezTempHashedString(m_sSpeedScaleParameter.GetData());
}

void ezParticleInitializerFactory_VelocityCone::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 2;
  inout_stream << uiVersion;

  // Version 1
  inout_stream << m_Angle;
  inout_stream << m_Speed.m_Value;
  inout_stream << m_Speed.m_fVariance;

  // Version 2
  inout_stream << m_sSpeedScaleParameter;
}

void ezParticleInitializerFactory_VelocityCone::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_Angle;
  inout_stream >> m_Speed.m_Value;
  inout_stream >> m_Speed.m_fVariance;

  if (uiVersion >= 2)
  {
    inout_stream >> m_sSpeedScaleParameter;
  }
}

void ezParticleInitializerFactory_VelocityCone::QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const
{
  inout_finalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizerFactory_ApplyVelocity>());
}

//////////////////////////////////////////////////////////////////////////

void ezParticleInitializer_VelocityCone::CreateRequiredStreams()
{
  CreateStream("Velocity", ezProcessingStream::DataType::Half4, &m_pStreamVelocity, true);
}

void ezParticleInitializer_VelocityCone::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  EZ_PROFILE_SCOPE("PFX: Velocity Cone");

  const ezVec3 startVel = GetOwnerSystem()->GetParticleStartVelocity();

  ezFloat16Vec4* pVelocity = m_pStreamVelocity->GetWritableData<ezFloat16Vec4>();

  ezRandom& rng = GetRNG();

  const float fSpeedScale = ezMath::Max(GetOwnerEffect()->GetFloatParameter(m_sSpeedScaleParameter, 1.0f), 0.0f);

  // const float dist = 1.0f / ezMath::Tan(m_Angle);

  for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    const ezVec3 dir = ezVec3::MakeRandomDeviationZ(rng, m_Angle);
    // dir.z = 0;
    // float len = 0.0f;

    // do
    //{
    //  // random point in a rectangle
    //  dir.x = (float)rng.DoubleMinMax(-1.0, 1.0);
    //  dir.y = (float)rng.DoubleMinMax(-1.0, 1.0);

    //  // discard points outside the circle
    //  len = dir.GetLengthSquared();
    //} while (len > 1.0f);

    // dir.z = dist;
    // dir.Normalize();

    const float fSpeed = (float)rng.DoubleVariance(m_Speed.m_Value, m_Speed.m_fVariance) * fSpeedScale;

    const ezVec3 vel = startVel + GetOwnerSystem()->GetTransform().m_qRotation * dir * fSpeed;
    const float fVelLength = vel.GetLength();
    const ezVec3 velDir = fVelLength > 0.0f ? vel / fVelLength : ezVec3(0, 0, 1);

    pVelocity[i] = ezVec4(velDir.x, velDir.y, velDir.z, fVelLength);
  }
}

//////////////////////////////////////////////////////////////////////////

class ezParticleInitializerFactory_VelocityCone_1_2 : public ezGraphPatch
{
public:
  ezParticleInitializerFactory_VelocityCone_1_2()
    : ezGraphPatch("ezParticleInitializerFactory_VelocityCone", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->InlineProperty("Speed").IgnoreResult();
  }
};

ezParticleInitializerFactory_VelocityCone_1_2 g_ezParticleInitializerFactory_VelocityCone_1_2;

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_VelocityCone);
