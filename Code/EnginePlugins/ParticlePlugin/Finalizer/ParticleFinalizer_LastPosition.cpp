#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_LastPosition.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleFinalizer_LastPosition, 1, ezRTTIDefaultAllocator<ezParticleFinalizer_LastPosition>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleFinalizer_LastPosition::ezParticleFinalizer_LastPosition()
{
  // do this at the start of the frame, but after the initializers
  m_fPriority = -499.0f;
}

bool ezParticleFinalizer_LastPosition::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void ezParticleFinalizer_LastPosition::CreateRequiredStreams(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  ctx.CreateStream(ezParticleStreamNames::s_sPosition, ezProcessingStream::DataType::Float4, &data.m_pStreamPosition, false);
  ctx.CreateStream(ezParticleStreamNames::s_sLastPosition, ezProcessingStream::DataType::Float3, &data.m_pStreamLastPosition, false);
}

void ezParticleFinalizer_LastPosition::Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const
{
  EZ_PROFILE_SCOPE("PFX: LastPosition");

  const InstanceData& data = ctx.GetInstanceData<InstanceData>();

  ezProcessingStreamIterator<ezVec4> itPosition(data.m_pStreamPosition, uiNumElements, 0);
  ezProcessingStreamIterator<ezVec3> itLastPosition(data.m_pStreamLastPosition, uiNumElements, 0);

  while (!itPosition.HasReachedEnd())
  {
    ezVec3 curPos = itPosition.Current().GetAsVec3();
    ezVec3& lastPos = itLastPosition.Current();

    lastPos = curPos;

    itPosition.Advance();
    itLastPosition.Advance();
  }
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Finalizer_ParticleFinalizer_LastPosition);
