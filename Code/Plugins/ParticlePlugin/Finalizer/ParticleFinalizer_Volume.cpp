#include <ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_Volume.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleFinalizerFactory_Volume, 1, ezRTTIDefaultAllocator<ezParticleFinalizerFactory_Volume>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleFinalizer_Volume, 1, ezRTTIDefaultAllocator<ezParticleFinalizer_Volume>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleFinalizerFactory_Volume::ezParticleFinalizerFactory_Volume() = default;
ezParticleFinalizerFactory_Volume::~ezParticleFinalizerFactory_Volume() = default;

const ezRTTI* ezParticleFinalizerFactory_Volume::GetFinalizerType() const
{
  return ezGetStaticRTTI<ezParticleFinalizer_Volume>();
}

void ezParticleFinalizerFactory_Volume::CopyFinalizerProperties(ezParticleFinalizer* pObject, bool bFirstTime) const
{
  ezParticleFinalizer_Volume* pFinalizer = static_cast<ezParticleFinalizer_Volume*>(pObject);
}

ezParticleFinalizer_Volume::ezParticleFinalizer_Volume() = default;
ezParticleFinalizer_Volume::~ezParticleFinalizer_Volume() = default;

void ezParticleFinalizer_Volume::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  m_pStreamSize = nullptr;
}

void ezParticleFinalizer_Volume::QueryOptionalStreams()
{
  m_pStreamSize = GetOwnerSystem()->QueryStream("Size", ezProcessingStream::DataType::Half);
}

void ezParticleFinalizer_Volume::Process(ezUInt64 uiNumElements)
{
  if (!GetOwnerEffect()->NeedsBoundingVolumeUpdate())
    return;

  EZ_PROFILE_SCOPE("PFX: Volume");

  const ezSimdVec4f* pPosition = m_pStreamPosition->GetData<ezSimdVec4f>();

  ezBoundingBoxSphere volume;
  volume.SetFromPoints(reinterpret_cast<const ezVec3*>(pPosition), static_cast<ezUInt32>(uiNumElements), sizeof(ezVec4));

  float fMaxSize[8] = {0};

  if (m_pStreamSize != nullptr)
  {
    const ezFloat16* pSize = m_pStreamSize->GetData<ezFloat16>();

    ezUInt32 idx = 0;

    for (ezUInt64 i = 0; i < uiNumElements / 8; ++i)
    {
      fMaxSize[0] = ezMath::Max(fMaxSize[0], (float)pSize[idx + 0]);
      fMaxSize[1] = ezMath::Max(fMaxSize[1], (float)pSize[idx + 1]);
      fMaxSize[2] = ezMath::Max(fMaxSize[2], (float)pSize[idx + 2]);
      fMaxSize[3] = ezMath::Max(fMaxSize[3], (float)pSize[idx + 3]);
      fMaxSize[4] = ezMath::Max(fMaxSize[4], (float)pSize[idx + 4]);
      fMaxSize[5] = ezMath::Max(fMaxSize[5], (float)pSize[idx + 5]);
      fMaxSize[6] = ezMath::Max(fMaxSize[6], (float)pSize[idx + 6]);
      fMaxSize[7] = ezMath::Max(fMaxSize[7], (float)pSize[idx + 7]);

      idx += 8;
    }

    for (ezUInt64 i = (uiNumElements / 8) * 8; i < uiNumElements; ++i)
    {
      fMaxSize[0] = ezMath::Max(fMaxSize[0], (float)pSize[i]);
    }
  }

  const float fms01 = ezMath::Max(fMaxSize[0], fMaxSize[1]);
  const float fms23 = ezMath::Max(fMaxSize[2], fMaxSize[3]);
  const float fms45 = ezMath::Max(fMaxSize[4], fMaxSize[5]);
  const float fms67 = ezMath::Max(fMaxSize[6], fMaxSize[7]);

  const float fms0123 = ezMath::Max(fms01, fms23);
  const float fms4567 = ezMath::Max(fms45, fms67);

  const float fms = ezMath::Max(fms0123, fms4567);

  GetOwnerSystem()->SetBoundingVolume(volume, fms);
}
