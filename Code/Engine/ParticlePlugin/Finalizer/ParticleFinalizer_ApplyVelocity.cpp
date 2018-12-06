#include <PCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleFinalizerFactory_ApplyVelocity, 1, ezRTTIDefaultAllocator<ezParticleFinalizerFactory_ApplyVelocity>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleFinalizer_ApplyVelocity, 1, ezRTTIDefaultAllocator<ezParticleFinalizer_ApplyVelocity>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleFinalizerFactory_ApplyVelocity::ezParticleFinalizerFactory_ApplyVelocity() {}

const ezRTTI* ezParticleFinalizerFactory_ApplyVelocity::GetFinalizerType() const
{
  return ezGetStaticRTTI<ezParticleFinalizer_ApplyVelocity>();
}

void ezParticleFinalizerFactory_ApplyVelocity::CopyFinalizerProperties(ezParticleFinalizer* pObject, bool bFirstTime) const
{
  ezParticleFinalizer_ApplyVelocity* pFinalizer = static_cast<ezParticleFinalizer_ApplyVelocity*>(pObject);
}

ezParticleFinalizer_ApplyVelocity::ezParticleFinalizer_ApplyVelocity() {}

ezParticleFinalizer_ApplyVelocity::~ezParticleFinalizer_ApplyVelocity() {}

void ezParticleFinalizer_ApplyVelocity::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Velocity", ezProcessingStream::DataType::Float3, &m_pStreamVelocity, false);
}

void ezParticleFinalizer_ApplyVelocity::Process(ezUInt64 uiNumElements)
{
  EZ_PROFILE_SCOPE("PFX: ApplyVelocity");

  const float tDiff = (float)m_TimeDiff.GetSeconds();

  ezProcessingStreamIterator<ezVec4> itPosition(m_pStreamPosition, uiNumElements, 0);
  ezProcessingStreamIterator<ezVec3> itVelocity(m_pStreamVelocity, uiNumElements, 0);

  while (!itPosition.HasReachedEnd())
  {
    ezVec3& pos = reinterpret_cast<ezVec3&>(itPosition.Current());

    pos += itVelocity.Current() * tDiff;

    itPosition.Advance();
    itVelocity.Advance();
  }
}
