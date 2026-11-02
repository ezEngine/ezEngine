#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_SpherePosition.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializerFactory_SpherePosition, 2, ezRTTIDefaultAllocator<ezParticleInitializerFactory_SpherePosition>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("PositionOffset", m_vPositionOffset),
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(0.25f), new ezClampValueAttribute(0.01f, 100.0f)),
    EZ_MEMBER_PROPERTY("OnSurface", m_bSpawnOnSurface),
    EZ_MEMBER_PROPERTY("SetVelocity", m_bSetVelocity),
    EZ_MEMBER_PROPERTY("Speed", m_Speed),
    EZ_MEMBER_PROPERTY("ScaleRadiusParam", m_sScaleRadiusParameter),
    EZ_MEMBER_PROPERTY("UseSpawnArea", m_bUseSpawnArea),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezSphereVisualizerAttribute("Radius", ezColor::MediumVioletRed, nullptr, ezVisualizerAnchor::Center, ezVec3(1.0f), "PositionOffset"),
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

void ezParticleInitializerFactory_SpherePosition::CopyInitializerProperties(ezParticleInitializer* pInitializer0, bool bFirstTime) const
{
  ezParticleInitializer_SpherePosition* pInitializer = static_cast<ezParticleInitializer_SpherePosition*>(pInitializer0);

  const float fScale = pInitializer->GetOwnerEffect()->GetFloatParameter(ezTempHashedString(m_sScaleRadiusParameter.GetData()), 1.0f);

  pInitializer->m_fRadius = ezMath::Max(m_fRadius * fScale, 0.01f); // prevent 0 radius
  pInitializer->m_bSpawnOnSurface = m_bSpawnOnSurface;
  pInitializer->m_bSetVelocity = m_bSetVelocity;
  pInitializer->m_Speed = m_Speed;
  pInitializer->m_vPositionOffset = m_vPositionOffset;
  pInitializer->m_bUseSpawnArea = m_bUseSpawnArea;
  pInitializer->m_fRadiusScale = fScale;
}

float ezParticleInitializerFactory_SpherePosition::GetSpawnCountMultiplier(const ezParticleEffectInstance* pEffect) const
{
  const float fScale = pEffect->GetFloatParameter(ezTempHashedString(m_sScaleRadiusParameter.GetData()), 1.0f);

  if (m_fRadius != 0.0f && fScale != 1.0f)
  {
    if (m_bSpawnOnSurface)
    {
      // original surface area
      const float s0 = 1.0f; /*4.0f * ezMath::Pi<float>() * m_fRadius * m_fRadius; */
      // new surface area
      const float s1 = 1.0f /*4.0f * ezMath::Pi<float>() * m_fRadius * m_fRadius */ * fScale * fScale;

      return s1 / s0;
    }
    else
    {
      // original volume
      const float v0 = 1.0f;
      /* 4.0f / 3.0f * ezMath::Pi<float>() * m_fRadius* m_fRadius* m_fRadius; */
      // new volume
      const float v1 = 1.0f /* 4.0f / 3.0f * ezMath::Pi<float>() * m_fRadius * m_fRadius * m_fRadius*/ * fScale * fScale * fScale;

      return v1 / v0;
    }
  }

  return 1.0f;
}

void ezParticleInitializerFactory_SpherePosition::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 4;
  inout_stream << uiVersion;

  inout_stream << m_fRadius;
  inout_stream << m_bSpawnOnSurface;
  inout_stream << m_bSetVelocity;
  inout_stream << m_Speed.m_Value;
  inout_stream << m_Speed.m_fVariance;

  // version 2
  inout_stream << m_vPositionOffset;

  // version 3
  inout_stream << m_sScaleRadiusParameter;

  // version 4
  inout_stream << m_bUseSpawnArea;
}

void ezParticleInitializerFactory_SpherePosition::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_fRadius;
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
  }

  if (uiVersion >= 4)
  {
    inout_stream >> m_bUseSpawnArea;
  }
}

void ezParticleInitializerFactory_SpherePosition::QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const
{
  if (m_bSetVelocity)
  {
    inout_finalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizerFactory_ApplyVelocity>());
  }
}

//////////////////////////////////////////////////////////////////////////

void ezParticleInitializer_SpherePosition::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, true);

  m_pStreamVelocity = nullptr;

  if (m_bSetVelocity)
  {
    CreateStream("Velocity", ezProcessingStream::DataType::Half4, &m_pStreamVelocity, true);
  }
}

void ezParticleInitializer_SpherePosition::OnFinalize()
{
  if (m_bUseSpawnArea)
  {
    RequestUpdateFromWorld();
  }
}

void ezParticleInitializer_SpherePosition::OnReset()
{
  m_Area.Reset();
}

void ezParticleInitializer_SpherePosition::UpdateFromWorld(const ezWorld& world, ezGameObjectHandle hOwnerObject)
{
  const ezParticleSphereAreaComponent* pArea = m_Area.Update<ezParticleSphereAreaComponent>(world, *GetOwnerEffect());

  // if the area is gone, the last values stay in use, the spawn counts were scaled for them
  if (pArea == nullptr)
    return;

  const float fAreaRadius = ezMath::Max(pArea->m_fRadius * m_fRadiusScale, 0.01f); // prevent 0 radius

  if (!m_Area.IsAreaUsed())
  {
    // the scale of the area relative to the effect, the scale of the effect itself applies to both shapes
    // with non-uniform scale, the surface area is approximated
    const ezVec3 vAreaScale = m_Area.GetTransform().m_vScale.Abs();
    const float fVolumeScale = vAreaScale.x * vAreaScale.y * vAreaScale.z;

    // m_fRadius is still the radius of the initializer here
    const float fRadiusRatio = fAreaRadius / m_fRadius;

    if (m_bSpawnOnSurface)
    {
      m_Area.UseArea(pArea->m_fSpawnScale * fRadiusRatio * fRadiusRatio * ezMath::Pow(fVolumeScale, 2.0f / 3.0f));
    }
    else
    {
      m_Area.UseArea(pArea->m_fSpawnScale * fRadiusRatio * fRadiusRatio * fRadiusRatio * fVolumeScale);
    }
  }

  m_fRadius = fAreaRadius;
}

bool ezParticleInitializer_SpherePosition::IsReadyToSpawn() const
{
  return !m_bUseSpawnArea || m_Area.IsReady(*GetOwnerEffect());
}

float ezParticleInitializer_SpherePosition::GetSpawnCountMultiplier() const
{
  return m_Area.GetSpawnCountMultiplier();
}

void ezParticleInitializer_SpherePosition::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  EZ_PROFILE_SCOPE("PFX: Sphere Position");

  const ezVec3 startVel = GetOwnerSystem()->GetParticleStartVelocity();
  const float fStartSpeed = startVel.GetLength();
  const ezVec3 startDir = fStartSpeed > 0.0f ? startVel / fStartSpeed : ezVec3(0, 0, 1);

  ezVec4* pPosition = m_pStreamPosition->GetWritableData<ezVec4>();
  ezFloat16Vec4* pVelocity = m_bSetVelocity ? m_pStreamVelocity->GetWritableData<ezFloat16Vec4>() : nullptr;

  ezRandom& rng = GetRNG();

  // the position offset is already part of the area transform
  const ezTransform trans = m_Area.IsAreaUsed() ? GetOwnerSystem()->GetTransform() * m_Area.GetTransformWithOffset(m_vPositionOffset) : GetOwnerSystem()->GetTransform();
  const ezVec3 vOffset = m_Area.IsAreaUsed() ? ezVec3::MakeZero() : m_vPositionOffset;

  for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    ezVec3 pos = ezVec3::MakeRandomPointInSphere(rng) * m_fRadius;
    ezVec3 normalPos = pos;

    if (m_bSpawnOnSurface || m_bSetVelocity)
    {
      normalPos.Normalize();
    }

    if (m_bSpawnOnSurface)
      pos = normalPos * m_fRadius;

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

class ezParticleInitializerFactory_SpherePosition_1_2 : public ezGraphPatch
{
public:
  ezParticleInitializerFactory_SpherePosition_1_2()
    : ezGraphPatch("ezParticleInitializerFactory_SpherePosition", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->InlineProperty("Speed").IgnoreResult();
  }
};

ezParticleInitializerFactory_SpherePosition_1_2 g_ezParticleInitializerFactory_SpherePosition_1_2;

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_SpherePosition);
