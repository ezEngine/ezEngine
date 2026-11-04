#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_Age.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleFinalizer_Age, 1, ezRTTIDefaultAllocator<ezParticleFinalizer_Age>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleFinalizer_Age::ezParticleFinalizer_Age() = default;

bool ezParticleFinalizer_Age::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void ezParticleFinalizer_Age::CreateRequiredStreams(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  ctx.CreateStream(ezParticleStreamNames::s_sLifeTime, ezProcessingStream::DataType::Half2, &data.m_pStreamLifeTime, true);

  data.m_pStreamPosition = nullptr;
  data.m_pStreamVelocity = nullptr;

  if (!m_sOnDeathEvent.IsEmpty())
  {
    ctx.CreateStream(ezParticleStreamNames::s_sPosition, ezProcessingStream::DataType::Float4, &data.m_pStreamPosition, false);
    ctx.CreateStream(ezParticleStreamNames::s_sVelocity, ezProcessingStream::DataType::Half4, &data.m_pStreamVelocity, false);
  }
}

void ezParticleFinalizer_Age::InitializeElements(const ezParticleModuleContext& ctx, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const
{
  EZ_PROFILE_SCOPE("PFX: Age Init");

  ezFloat16Vec2* pLifeTime = ctx.GetInstanceData<InstanceData>().m_pStreamLifeTime->GetWritableData<ezFloat16Vec2>();
  const float fLifeScale = ezMath::Clamp(ctx.GetEffect()->GetFloatParameter(m_sLifeScaleParameter, 1.0f), 0.0f, 2.0f);

  if (m_LifeTime.m_fVariance == 0)
  {
    const float tLifeTime = ezMath::Max(fLifeScale * (float)m_LifeTime.m_Value.GetSeconds(), 0.01f); // make sure it's not zero
    const float tInvLifeTime = 1.0f / tLifeTime;

    for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pLifeTime[i].x = tLifeTime;
      pLifeTime[i].y = tInvLifeTime;
    }
  }
  else // random range
  {
    ezRandom& rng = ctx.GetRNG();

    for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      const float tLifeTime =
        ezMath::Max(fLifeScale * (float)rng.DoubleVariance(m_LifeTime.m_Value.GetSeconds(), m_LifeTime.m_fVariance), 0.01f); // make sure it's not zero
      const float tInvLifeTime = 1.0f / tLifeTime;

      pLifeTime[i].x = tLifeTime;
      pLifeTime[i].y = tInvLifeTime;
    }
  }
}

void ezParticleFinalizer_Age::Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const
{
  EZ_PROFILE_SCOPE("PFX: Age");

  ezFloat16Vec2* pLifeTime = ctx.GetInstanceData<InstanceData>().m_pStreamLifeTime->GetWritableData<ezFloat16Vec2>();

  const float tDiff = (float)ctx.GetTimeDiff().GetSeconds();

  for (ezUInt32 i = 0; i < uiNumElements; ++i)
  {
    pLifeTime[i].x = pLifeTime[i].x - tDiff;

    if (pLifeTime[i].x <= 0)
    {
      pLifeTime[i].x = 0;

      ctx.m_pSystem->RemoveParticle(i);
    }
  }
}

void ezParticleFinalizer_Age::OnParticleDeath(const ezParticleModuleContext& ctx, ezUInt64 uiElementIndex) const
{
  const InstanceData& data = ctx.GetInstanceData<InstanceData>();

  const ezVec4* pPosition = data.m_pStreamPosition->GetData<ezVec4>();
  const ezFloat16Vec4* pVelocity = data.m_pStreamVelocity->GetData<ezFloat16Vec4>();

  const ezVec4 vel = pVelocity[uiElementIndex];
  const ezVec3 dir(vel.x, vel.y, vel.z);
  const float speed = vel.w;

  ezParticleEvent pe;
  pe.m_EventType = m_sOnDeathEvent;
  pe.m_vPosition = pPosition[uiElementIndex].GetAsVec3();
  pe.m_vDirection = dir * speed;
  pe.m_vNormal.SetZero();

  ctx.GetEffect()->AddParticleEvent(pe);
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Finalizer_ParticleFinalizer_Age);
