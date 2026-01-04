#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleFinalizerFactory_ApplyVelocity, 1, ezRTTIDefaultAllocator<ezParticleFinalizerFactory_ApplyVelocity>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleFinalizer_ApplyVelocity, 1, ezRTTIDefaultAllocator<ezParticleFinalizer_ApplyVelocity>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleFinalizerFactory_ApplyVelocity::ezParticleFinalizerFactory_ApplyVelocity() = default;

const ezRTTI* ezParticleFinalizerFactory_ApplyVelocity::GetFinalizerType() const
{
  return ezGetStaticRTTI<ezParticleFinalizer_ApplyVelocity>();
}

void ezParticleFinalizerFactory_ApplyVelocity::CopyFinalizerProperties(ezParticleFinalizer* pObject, bool bFirstTime) const
{
  ezParticleFinalizer_ApplyVelocity* pFinalizer = static_cast<ezParticleFinalizer_ApplyVelocity*>(pObject);
}

ezParticleFinalizer_ApplyVelocity::ezParticleFinalizer_ApplyVelocity()
{
  // a bit later than the other finalizers
  m_fPriority = 525.0f;
}

ezParticleFinalizer_ApplyVelocity::~ezParticleFinalizer_ApplyVelocity() = default;

void ezParticleFinalizer_ApplyVelocity::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Velocity", ezProcessingStream::DataType::Half4, &m_pStreamVelocity, false);
}

void ezParticleFinalizer_ApplyVelocity::Process(ezUInt64 uiNumElements)
{
  EZ_PROFILE_SCOPE("PFX: ApplyVelocity");

  const float tDiff = (float)m_TimeDiff.GetSeconds();

  ezProcessingStreamIterator<ezVec4> itPosition(m_pStreamPosition, uiNumElements, 0);
  ezProcessingStreamIterator<ezFloat16Vec4> itVelocity(m_pStreamVelocity, uiNumElements, 0);

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
