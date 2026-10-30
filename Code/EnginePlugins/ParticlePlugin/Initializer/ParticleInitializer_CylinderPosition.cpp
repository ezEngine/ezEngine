#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_CylinderPosition.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializerFactory_CylinderPosition, 2, ezRTTIDefaultAllocator<ezParticleInitializerFactory_CylinderPosition>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("PositionOffset", m_vPositionOffset),
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(0.25f), new ezClampValueAttribute(0.01f, 100.0f)),
    EZ_MEMBER_PROPERTY("Height", m_fHeight)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 100.0f)),
    EZ_MEMBER_PROPERTY("OnSurface", m_bSpawnOnSurface),
    EZ_MEMBER_PROPERTY("SetVelocity", m_bSetVelocity),
    EZ_MEMBER_PROPERTY("Speed", m_Speed),
    EZ_MEMBER_PROPERTY("ScaleRadiusParam", m_sScaleRadiusParameter),
    EZ_MEMBER_PROPERTY("ScaleHeightParam", m_sScaleHeightParameter),
    EZ_MEMBER_PROPERTY("UseSpawnArea", m_bUseSpawnArea),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCylinderVisualizerAttribute(ezBasisAxis::PositiveZ, "Height", "Radius", ezColor::MediumVioletRed, nullptr, ezVisualizerAnchor::Center, ezVec3(1.0f), "PositionOffset")
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

  const float fScaleRadius = pInitializer->GetOwnerEffect()->GetFloatParameter(ezTempHashedString(m_sScaleRadiusParameter.GetData()), 1.0f);
  const float fScaleHeight = pInitializer->GetOwnerEffect()->GetFloatParameter(ezTempHashedString(m_sScaleHeightParameter.GetData()), 1.0f);

  pInitializer->m_vPositionOffset = m_vPositionOffset;
  pInitializer->m_bUseSpawnArea = m_bUseSpawnArea;
  pInitializer->m_fRadius = ezMath::Max(m_fRadius * fScaleRadius, 0.01f); // prevent 0 radius
  pInitializer->m_fHeight = ezMath::Max(m_fHeight * fScaleHeight, 0.0f);
  pInitializer->m_bSpawnOnSurface = m_bSpawnOnSurface;
  pInitializer->m_bSetVelocity = m_bSetVelocity;
  pInitializer->m_Speed = m_Speed;
  pInitializer->m_fRadiusScale = fScaleRadius;
  pInitializer->m_fHeightScale = m_fHeight > 0.0f ? fScaleHeight : 0.0f;
}

float ezParticleInitializerFactory_CylinderPosition::GetSpawnCountMultiplier(const ezParticleEffectInstance* pEffect) const
{
  const float fScaleRadius = pEffect->GetFloatParameter(ezTempHashedString(m_sScaleRadiusParameter.GetData()), 1.0f);
  const float fScaleHeight = pEffect->GetFloatParameter(ezTempHashedString(m_sScaleHeightParameter.GetData()), 1.0f);

  if (m_bSpawnOnSurface)
  {
    const float s0 = /* 2.0f * ezMath::Pi<float>() * m_fRadius **/ m_fRadius + /* 2.0f * ezMath::Pi<float>() * m_fRadius **/ m_fHeight;
    const float s1 = /* 2.0f * ezMath::Pi<float>() * m_fRadius **/ m_fRadius * fScaleRadius * fScaleRadius +
                     /*2.0f * ezMath::Pi<float>() * m_fRadius **/ fScaleRadius * m_fHeight * fScaleHeight;

    return s1 / s0;
  }
  else
  {
    const float v0 = 1.0f /* ezMath::Pi<float>() * m_fRadius * m_fRadius*/;
    const float v1 = 1.0f /* ezMath::Pi<float>() * m_fRadius * m_fRadius*/ * fScaleRadius * fScaleRadius;

    return v1 / v0;
  }
}

void ezParticleInitializerFactory_CylinderPosition::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 4;
  inout_stream << uiVersion;

  inout_stream << m_fRadius;
  inout_stream << m_fHeight;
  inout_stream << m_bSpawnOnSurface;
  inout_stream << m_bSetVelocity;
  inout_stream << m_Speed.m_Value;
  inout_stream << m_Speed.m_fVariance;

  // version 2
  inout_stream << m_vPositionOffset;

  // version 3
  inout_stream << m_sScaleRadiusParameter;
  inout_stream << m_sScaleHeightParameter;

  // version 4
  inout_stream << m_bUseSpawnArea;
}

void ezParticleInitializerFactory_CylinderPosition::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_fRadius;
  inout_stream >> m_fHeight;
  inout_stream >> m_bSpawnOnSurface;
  inout_stream >> m_bSetVelocity;
  inout_stream >> m_Speed.m_Value;
  inout_stream >> m_Speed.m_fVariance;

  if (uiVersion >= 2)
  {
    inout_stream >> m_vPositionOffset;
  }

  if (uiVersion >= 3)
  {
    inout_stream >> m_sScaleRadiusParameter;
    inout_stream >> m_sScaleHeightParameter;
  }

  if (uiVersion >= 4)
  {
    inout_stream >> m_bUseSpawnArea;
  }
}

void ezParticleInitializerFactory_CylinderPosition::QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const
{
  if (m_bSetVelocity)
  {
    inout_finalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizerFactory_ApplyVelocity>());
  }
}

//////////////////////////////////////////////////////////////////////////

void ezParticleInitializer_CylinderPosition::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, true);

  m_pStreamVelocity = nullptr;

  if (m_bSetVelocity)
  {
    CreateStream("Velocity", ezProcessingStream::DataType::Half4, &m_pStreamVelocity, true);
  }
}

void ezParticleInitializer_CylinderPosition::OnFinalize()
{
  if (m_bUseSpawnArea)
  {
    RequestUpdateFromWorld();
  }
}

void ezParticleInitializer_CylinderPosition::OnReset()
{
  m_Area.Reset();
  m_bUseArea = false;
}

void ezParticleInitializer_CylinderPosition::UpdateFromWorld(const ezWorld& world, ezGameObjectHandle hOwnerObject)
{
  const ezParticleCylinderAreaComponent* pArea = m_Area.Update<ezParticleCylinderAreaComponent>(world, *GetOwnerEffect());

  m_bUseArea = pArea != nullptr;

  if (pArea)
  {
    m_fAreaRadius = ezMath::Max(pArea->m_fRadius * m_fRadiusScale, 0.01f); // prevent 0 radius
    m_fAreaHeight = ezMath::Max(pArea->m_fHeight * m_fHeightScale, 0.0f);
    m_fAreaSpawnScale = pArea->m_fSpawnScale;
  }
}

bool ezParticleInitializer_CylinderPosition::IsReadyToSpawn() const
{
  return !m_bUseSpawnArea || m_Area.IsReady(*GetOwnerEffect());
}

float ezParticleInitializer_CylinderPosition::GetSpawnCountMultiplier() const
{
  if (!m_bUseArea)
    return 1.0f;

  // the scale of the area relative to the effect, the scale of the effect itself applies to both shapes
  // with non-uniform scale, the circumference is approximated
  const ezVec3 vAreaScale = m_Area.GetTransform().m_vScale.Abs();
  const float fRadiusRatio = m_fAreaRadius / m_fRadius;

  float fMultiplier = m_fAreaSpawnScale;

  if (m_bSpawnOnSurface)
  {
    fMultiplier *= fRadiusRatio * ezMath::Sqrt(vAreaScale.x * vAreaScale.y);
  }
  else
  {
    fMultiplier *= fRadiusRatio * fRadiusRatio * vAreaScale.x * vAreaScale.y;
  }

  // with a height of zero, particles spawn in a disc or on a ring
  if (m_fHeight > 0.0f)
  {
    fMultiplier *= m_fAreaHeight * vAreaScale.z / m_fHeight;
  }

  return fMultiplier;
}

void ezParticleInitializer_CylinderPosition::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  EZ_PROFILE_SCOPE("PFX: Cylinder Position");

  const ezVec3 startVel = GetOwnerSystem()->GetParticleStartVelocity();
  const float fStartSpeed = startVel.GetLength();
  const ezVec3 startDir = fStartSpeed > 0.0f ? startVel / fStartSpeed : ezVec3(0, 0, 1);

  ezVec4* pPosition = m_pStreamPosition->GetWritableData<ezVec4>();
  ezFloat16Vec4* pVelocity = m_bSetVelocity ? m_pStreamVelocity->GetWritableData<ezFloat16Vec4>() : nullptr;

  ezRandom& rng = GetRNG();

  // the area replaces the radius and the height, the position offset is already part of the area transform
  const ezTransform trans = m_bUseArea ? GetOwnerSystem()->GetTransform() * m_Area.GetTransformWithOffset(m_vPositionOffset) : GetOwnerSystem()->GetTransform();
  const ezVec3 vOffset = m_bUseArea ? ezVec3::MakeZero() : m_vPositionOffset;
  const float fRadius = m_bUseArea ? m_fAreaRadius : m_fRadius;
  const float fHeight = m_bUseArea ? m_fAreaHeight : m_fHeight;

  const float fRadiusSqr = fRadius * fRadius;
  const float fHalfHeight = fHeight * 0.5f;

  for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    ezVec3 pos;
    float len = 0.0f;
    pos.z = 0.0f;

    do
    {
      pos.x = (float)rng.DoubleMinMax(-fRadius, fRadius);
      pos.y = (float)rng.DoubleMinMax(-fRadius, fRadius);

      len = pos.GetLengthSquared();
    } while (len > fRadiusSqr ||
             len <= 0.000001f); // prevent spawning at the exact center (note: this has to be smaller than the minimum allowed radius sqr)

    ezVec3 normalPos = pos;

    if (m_bSpawnOnSurface || m_bSetVelocity)
    {
      normalPos.Normalize();
    }

    if (m_bSpawnOnSurface)
      pos = normalPos * fRadius;

    if (fHeight > 0)
    {
      pos.z = (float)rng.DoubleMinMax(-fHalfHeight, fHalfHeight);
    }

    pos += vOffset;

    if (m_bSetVelocity)
    {
      const float fSpeed = (float)rng.DoubleVariance(m_Speed.m_Value, m_Speed.m_fVariance);

      const ezVec3 vel = startVel + trans.m_qRotation * normalPos * fSpeed;
      const float fVelLength = vel.GetLength();
      const ezVec3 velDir = fVelLength > 0.0f ? vel / fVelLength : ezVec3(0, 0, 1);

      pVelocity[i] = ezVec4(velDir.x, velDir.y, velDir.z, fVelLength);
    }

    pPosition[i] = (trans * pos).GetAsVec4(0);
  }
}

//////////////////////////////////////////////////////////////////////////

class ezParticleInitializerFactory_CylinderPosition_1_2 : public ezGraphPatch
{
public:
  ezParticleInitializerFactory_CylinderPosition_1_2()
    : ezGraphPatch("ezParticleInitializerFactory_CylinderPosition", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->InlineProperty("Speed").IgnoreResult();
  }
};

ezParticleInitializerFactory_CylinderPosition_1_2 g_ezParticleInitializerFactory_CylinderPosition_1_2;

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_CylinderPosition);
