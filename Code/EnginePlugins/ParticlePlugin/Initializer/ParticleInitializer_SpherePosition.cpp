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
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializer_SpherePosition, 2, ezRTTIDefaultAllocator<ezParticleInitializer_SpherePosition>)
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
// clang-format on

ezParticleInitializer_SpherePosition::ezParticleInitializer_SpherePosition()
{
  m_fRadius = 0.25f;
  m_vPositionOffset.SetZero();
  m_bSpawnOnSurface = false;
  m_bSetVelocity = false;
}

float ezParticleInitializer_SpherePosition::GetSpawnCountMultiplier(const ezParticleEffectInstance* pEffect) const
{
  const float fScale = pEffect->GetFloatParameter(m_sScaleRadiusParameter, 1.0f);

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

void ezParticleInitializer_SpherePosition::Save(ezStreamWriter& inout_stream) const
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

void ezParticleInitializer_SpherePosition::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
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

void ezParticleInitializer_SpherePosition::QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const
{
  if (m_bSetVelocity)
  {
    inout_finalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizer_ApplyVelocity>());
  }
}

//////////////////////////////////////////////////////////////////////////

bool ezParticleInitializer_SpherePosition::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void ezParticleInitializer_SpherePosition::CreateRequiredStreams(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  ctx.CreateStream(ezParticleStreamNames::s_sPosition, ezProcessingStream::DataType::Float4, &data.m_pStreamPosition, true);

  data.m_pStreamVelocity = nullptr;

  if (m_bSetVelocity)
  {
    ctx.CreateStream(ezParticleStreamNames::s_sVelocity, ezProcessingStream::DataType::Half4, &data.m_pStreamVelocity, true);
  }
}

void ezParticleInitializer_SpherePosition::OnFinalize(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  const float fScale = ctx.GetEffect()->GetFloatParameter(m_sScaleRadiusParameter, 1.0f);

  data.m_fRadius = ezMath::Max(m_fRadius * fScale, 0.01f); // prevent 0 radius
  data.m_fRadiusScale = fScale;
  data.m_Area.Reset();
}

void ezParticleInitializer_SpherePosition::UpdateFromWorld(const ezParticleModuleContext& ctx, const ezWorld& world, ezGameObjectHandle hOwnerObject) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  const ezParticleSphereAreaComponent* pArea = data.m_Area.Update<ezParticleSphereAreaComponent>(world, *ctx.GetEffect());

  // if the area is gone, the last values stay in use, the spawn counts were scaled for them
  if (pArea == nullptr)
    return;

  const float fAreaRadius = ezMath::Max(pArea->m_fRadius * data.m_fRadiusScale, 0.01f); // prevent 0 radius

  if (!data.m_Area.IsAreaUsed())
  {
    // the scale of the area relative to the effect, the scale of the effect itself applies to both shapes
    // with non-uniform scale, the surface area is approximated
    const ezVec3 vAreaScale = data.m_Area.GetTransform().m_vScale.Abs();
    const float fVolumeScale = vAreaScale.x * vAreaScale.y * vAreaScale.z;

    // data.m_fRadius is still the radius of the initializer here
    const float fRadiusRatio = fAreaRadius / data.m_fRadius;

    if (m_bSpawnOnSurface)
    {
      data.m_Area.UseArea(pArea->m_fSpawnScale * fRadiusRatio * fRadiusRatio * ezMath::Pow(fVolumeScale, 2.0f / 3.0f));
    }
    else
    {
      data.m_Area.UseArea(pArea->m_fSpawnScale * fRadiusRatio * fRadiusRatio * fRadiusRatio * fVolumeScale);
    }
  }

  data.m_fRadius = fAreaRadius;
}

bool ezParticleInitializer_SpherePosition::PrepareToSpawn(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  return !m_bUseSpawnArea || data.m_Area.IsReady(*ctx.GetEffect());
}

float ezParticleInitializer_SpherePosition::GetInstanceSpawnCountMultiplier(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  return data.m_Area.GetSpawnCountMultiplier();
}

void ezParticleInitializer_SpherePosition::InitializeElements(const ezParticleModuleContext& ctx, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  EZ_PROFILE_SCOPE("PFX: Sphere Position");

  const ezVec3 startVel = ctx.m_pSystem->GetParticleStartVelocity();
  const float fStartSpeed = startVel.GetLength();
  const ezVec3 startDir = fStartSpeed > 0.0f ? startVel / fStartSpeed : ezVec3(0, 0, 1);

  ezVec4* pPosition = data.m_pStreamPosition->GetWritableData<ezVec4>();
  ezFloat16Vec4* pVelocity = m_bSetVelocity ? data.m_pStreamVelocity->GetWritableData<ezFloat16Vec4>() : nullptr;

  ezRandom& rng = ctx.GetRNG();

  // the position offset is already part of the area transform
  const ezTransform trans = data.m_Area.IsAreaUsed() ? ctx.m_pSystem->GetTransform() * data.m_Area.GetTransformWithOffset(m_vPositionOffset) : ctx.m_pSystem->GetTransform();
  const ezVec3 vOffset = data.m_Area.IsAreaUsed() ? ezVec3::MakeZero() : m_vPositionOffset;

  for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    ezVec3 pos = ezVec3::MakeRandomPointInSphere(rng) * data.m_fRadius;
    ezVec3 normalPos = pos;

    if (m_bSpawnOnSurface || m_bSetVelocity)
    {
      normalPos.Normalize();
    }

    if (m_bSpawnOnSurface)
      pos = normalPos * data.m_fRadius;

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

class ezParticleInitializer_SpherePosition_1_2 : public ezGraphPatch
{
public:
  ezParticleInitializer_SpherePosition_1_2()
    : ezGraphPatch("ezParticleInitializerFactory_SpherePosition", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->InlineProperty("Speed").IgnoreResult();
  }
};

ezParticleInitializer_SpherePosition_1_2 g_ezParticleInitializer_SpherePosition_1_2;

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_SpherePosition);
