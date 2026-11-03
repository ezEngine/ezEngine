#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleFinalizer_ApplyVelocity, 1, ezRTTIDefaultAllocator<ezParticleFinalizer_ApplyVelocity>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleFinalizer_ApplyVelocity::ezParticleFinalizer_ApplyVelocity()
{
  // a bit later than the other finalizers
  m_fPriority = 525.0f;
}

bool ezParticleFinalizer_ApplyVelocity::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void ezParticleFinalizer_ApplyVelocity::CreateRequiredStreams(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  ctx.CreateStream(ezParticleStreamNames::s_sPosition, ezProcessingStream::DataType::Float4, &data.m_pStreamPosition, false);
  ctx.CreateStream(ezParticleStreamNames::s_sVelocity, ezProcessingStream::DataType::Half4, &data.m_pStreamVelocity, false);
}

void ezParticleFinalizer_ApplyVelocity::Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const
{
  EZ_PROFILE_SCOPE("PFX: ApplyVelocity");

  const InstanceData& data = ctx.GetInstanceData<InstanceData>();

  const float tDiff = (float)ctx.GetTimeDiff().GetSeconds();

  ezProcessingStreamIterator<ezVec4> itPosition(data.m_pStreamPosition, uiNumElements, 0);
  ezProcessingStreamIterator<ezFloat16Vec4> itVelocity(data.m_pStreamVelocity, uiNumElements, 0);

  while (!itPosition.HasReachedEnd())
  {
    ezVec3& pos = reinterpret_cast<ezVec3&>(itPosition.Current());

    const ezVec4 vel = itVelocity.Current();
    const ezVec3 dir(vel.x, vel.y, vel.z);
    const float speed = vel.w;

    pos += dir * speed * tDiff;

    itPosition.Advance();
    itVelocity.Advance();
  }
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Finalizer_ParticleFinalizer_ApplyVelocity);
