#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_PullAlong.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_PullAlong, 1, ezRTTIDefaultAllocator<ezParticleBehavior_PullAlong>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Strength", m_fStrength)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleBehavior_PullAlong::ezParticleBehavior_PullAlong() = default;

enum class BehaviorPullAlongVersion
{
  Version_0 = 0,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleBehavior_PullAlong::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = (int)BehaviorPullAlongVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_fStrength;
}

void ezParticleBehavior_PullAlong::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)BehaviorPullAlongVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_fStrength;
}

bool ezParticleBehavior_PullAlong::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void ezParticleBehavior_PullAlong::CreateRequiredStreams(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  data.m_bFirstTime = true;
  data.m_vApplyPull.SetZero();

  ctx.CreateStream(ezParticleStreamNames::s_sPosition, ezProcessingStream::DataType::Float4, &data.m_pStreamPosition, false);
}

void ezParticleBehavior_PullAlong::Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  EZ_PROFILE_SCOPE("PFX: PullAlong");

  const ezVec3 vPos = ctx.m_pSystem->GetTransform().m_vPosition;

  if (!data.m_bFirstTime)
  {
    data.m_vApplyPull = (vPos - data.m_vLastEmitterPosition) * ezMath::Clamp(m_fStrength, 0.0f, 1.0f);
  }
  else
  {
    data.m_bFirstTime = false;
    data.m_vApplyPull.SetZero();
  }

  data.m_vLastEmitterPosition = vPos;

  if (data.m_vApplyPull.IsZero())
    return;

  ezProcessingStreamIterator<ezSimdVec4f> itPosition(data.m_pStreamPosition, uiNumElements, 0);
  ezSimdVec4f pull;
  pull.Load<3>(&data.m_vApplyPull.x);

  while (!itPosition.HasReachedEnd())
  {
    itPosition.Current() += pull;

    itPosition.Advance();
  }
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_PullAlong);
