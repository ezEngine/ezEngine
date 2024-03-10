#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_LastPosition.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleFinalizerFactory_LastPosition, 1, ezRTTIDefaultAllocator<ezParticleFinalizerFactory_LastPosition>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleFinalizer_LastPosition, 1, ezRTTIDefaultAllocator<ezParticleFinalizer_LastPosition>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleFinalizerFactory_LastPosition::ezParticleFinalizerFactory_LastPosition() = default;

const ezRTTI* ezParticleFinalizerFactory_LastPosition::GetFinalizerType() const
{
  return ezGetStaticRTTI<ezParticleFinalizer_LastPosition>();
}

void ezParticleFinalizerFactory_LastPosition::CopyFinalizerProperties(ezParticleFinalizer* pObject, bool bFirstTime) const
{
  ezParticleFinalizer_LastPosition* pFinalizer = static_cast<ezParticleFinalizer_LastPosition*>(pObject);
}

//////////////////////////////////////////////////////////////////////////

ezParticleFinalizer_LastPosition::ezParticleFinalizer_LastPosition()
{
  // do this at the start of the frame, but after the initializers
  m_fPriority = -499.0f;
}

ezParticleFinalizer_LastPosition::~ezParticleFinalizer_LastPosition() = default;

void ezParticleFinalizer_LastPosition::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("LastPosition", ezProcessingStream::DataType::Float3, &m_pStreamLastPosition, false);
}

void ezParticleFinalizer_LastPosition::Process(ezUInt64 uiNumElements)
{
  EZ_PROFILE_SCOPE("PFX: LastPosition");

  ezProcessingStreamIterator<ezVec4> itPosition(m_pStreamPosition, uiNumElements, 0);
  ezProcessingStreamIterator<ezVec3> itLastPosition(m_pStreamLastPosition, uiNumElements, 0);

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
