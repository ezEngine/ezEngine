#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_BoxPosition.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializer_BoxPosition, 1, ezRTTIDefaultAllocator<ezParticleInitializer_BoxPosition>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("PositionOffset", m_vPositionOffset),
    EZ_MEMBER_PROPERTY("Size", m_vSize)->AddAttributes(new ezDefaultValueAttribute(ezVec3(0, 0, 0))),
    EZ_MEMBER_PROPERTY("ScaleXParam", m_sScaleXParameter),
    EZ_MEMBER_PROPERTY("ScaleYParam", m_sScaleYParameter),
    EZ_MEMBER_PROPERTY("ScaleZParam", m_sScaleZParameter),
    EZ_MEMBER_PROPERTY("UseSpawnArea", m_bUseSpawnArea),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezBoxVisualizerAttribute("Size", 1.0f, ezColor::MediumVioletRed, nullptr, ezVisualizerAnchor::Center, ezVec3(1.0f), "PositionOffset")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleInitializer_BoxPosition::ezParticleInitializer_BoxPosition()
{
  m_vPositionOffset.SetZero();
  m_vSize.Set(0, 0, 0);
}

float ezParticleInitializer_BoxPosition::GetSpawnCountMultiplier(const ezParticleEffectInstance* pEffect) const
{
  const float fScaleX = pEffect->GetFloatParameter(m_sScaleXParameter, 1.0f);
  const float fScaleY = pEffect->GetFloatParameter(m_sScaleYParameter, 1.0f);
  const float fScaleZ = pEffect->GetFloatParameter(m_sScaleZParameter, 1.0f);

  float fSpawnMultiplier = 1.0f;

  if (m_vSize.x != 0.0f)
    fSpawnMultiplier *= fScaleX;

  if (m_vSize.y != 0.0f)
    fSpawnMultiplier *= fScaleY;

  if (m_vSize.z != 0.0f)
    fSpawnMultiplier *= fScaleZ;

  return fSpawnMultiplier;
}

void ezParticleInitializer_BoxPosition::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 4;
  inout_stream << uiVersion;

  inout_stream << m_vSize;

  // version 2
  inout_stream << m_vPositionOffset;

  // version 3
  inout_stream << m_sScaleXParameter;
  inout_stream << m_sScaleYParameter;
  inout_stream << m_sScaleZParameter;

  // version 4
  inout_stream << m_bUseSpawnArea;
}

void ezParticleInitializer_BoxPosition::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_vSize;

  if (uiVersion >= 2)
  {
    inout_stream >> m_vPositionOffset;
  }

  if (uiVersion >= 3)
  {
    inout_stream >> m_sScaleXParameter;
    inout_stream >> m_sScaleYParameter;
    inout_stream >> m_sScaleZParameter;
  }

  if (uiVersion >= 4)
  {
    inout_stream >> m_bUseSpawnArea;
  }
}

bool ezParticleInitializer_BoxPosition::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void ezParticleInitializer_BoxPosition::CreateRequiredStreams(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  ctx.CreateStream(ezParticleStreamNames::s_sPosition, ezProcessingStream::DataType::Float4, &data.m_pStreamPosition, true);
}

void ezParticleInitializer_BoxPosition::OnFinalize(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  const float fScaleX = ctx.GetEffect()->GetFloatParameter(m_sScaleXParameter, 1.0f);
  const float fScaleY = ctx.GetEffect()->GetFloatParameter(m_sScaleYParameter, 1.0f);
  const float fScaleZ = ctx.GetEffect()->GetFloatParameter(m_sScaleZParameter, 1.0f);

  data.m_vSize.Set(m_vSize.x * fScaleX, m_vSize.y * fScaleY, m_vSize.z * fScaleZ);
  data.m_vSizeScale.Set(m_vSize.x != 0.0f ? fScaleX : 0.0f, m_vSize.y != 0.0f ? fScaleY : 0.0f, m_vSize.z != 0.0f ? fScaleZ : 0.0f);
  data.m_Area.Reset();
}

void ezParticleInitializer_BoxPosition::UpdateFromWorld(const ezParticleModuleContext& ctx, const ezWorld& world, ezGameObjectHandle hOwnerObject) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  const ezParticleBoxAreaComponent* pArea = data.m_Area.Update<ezParticleBoxAreaComponent>(world, *ctx.GetEffect());

  // if the area is gone, the last values stay in use, the spawn counts were scaled for them
  if (pArea == nullptr)
    return;

  const ezVec3 vAreaSize = pArea->m_vSize.CompMul(data.m_vSizeScale);

  if (!data.m_Area.IsAreaUsed())
  {
    // the scale of the area relative to the effect, the scale of the effect itself applies to both shapes
    const ezVec3 vAreaScale = data.m_Area.GetTransform().m_vScale.Abs();

    float fMultiplier = pArea->m_fSpawnScale;

    // data.m_vSize is still the size of the initializer here
    for (ezUInt32 i = 0; i < 3; ++i)
    {
      if (data.m_vSize.GetData()[i] != 0.0f)
      {
        fMultiplier *= vAreaSize.GetData()[i] * vAreaScale.GetData()[i] / ezMath::Abs(data.m_vSize.GetData()[i]);
      }
    }

    data.m_Area.UseArea(fMultiplier);
  }

  data.m_vSize = vAreaSize;
}

bool ezParticleInitializer_BoxPosition::PrepareToSpawn(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  return !m_bUseSpawnArea || data.m_Area.IsReady(*ctx.GetEffect());
}

float ezParticleInitializer_BoxPosition::GetInstanceSpawnCountMultiplier(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  return data.m_Area.GetSpawnCountMultiplier();
}

void ezParticleInitializer_BoxPosition::InitializeElements(const ezParticleModuleContext& ctx, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  EZ_PROFILE_SCOPE("PFX: Box Position");

  ezSimdVec4f* pPosition = data.m_pStreamPosition->GetWritableData<ezSimdVec4f>();

  ezRandom& rng = ctx.GetRNG();

  // the position offset is already part of the area transform
  const ezTransform trans = data.m_Area.IsAreaUsed() ? ctx.m_pSystem->GetTransform() * data.m_Area.GetTransformWithOffset(m_vPositionOffset) : ctx.m_pSystem->GetTransform();
  const ezVec3 vOffset = data.m_Area.IsAreaUsed() ? ezVec3::MakeZero() : m_vPositionOffset;

  if (data.m_vSize.IsZero())
  {
    ezSimdVec4f pos = ezSimdConversion::ToVec4((trans * vOffset).GetAsVec4(0));

    for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pPosition[i] = pos;
    }
  }
  else
  {
    ezSimdVec4f pos;
    ezSimdTransform transform = ezSimdConversion::ToTransform(trans);

    float p0[4];
    p0[3] = 0;

    for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      p0[0] = (float)(rng.DoubleMinMax(-data.m_vSize.x, data.m_vSize.x) * 0.5) + vOffset.x;
      p0[1] = (float)(rng.DoubleMinMax(-data.m_vSize.y, data.m_vSize.y) * 0.5) + vOffset.y;
      p0[2] = (float)(rng.DoubleMinMax(-data.m_vSize.z, data.m_vSize.z) * 0.5) + vOffset.z;

      pos.Load<4>(p0);

      pPosition[i] = transform.TransformPosition(pos);
    }
  }
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_BoxPosition);
