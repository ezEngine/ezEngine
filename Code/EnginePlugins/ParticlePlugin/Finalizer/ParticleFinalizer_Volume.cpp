#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_Volume.h>
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
  if (uiNumElements == 0)
    return;

  EZ_PROFILE_SCOPE("PFX: Volume");

  const ezSimdVec4f* pPosition = m_pStreamPosition->GetData<ezSimdVec4f>();

  const ezSimdBBoxSphere volume = ezSimdBBoxSphere::MakeFromPoints(pPosition, static_cast<ezUInt32>(uiNumElements));

  float fMaxSize = 0;

  if (m_pStreamSize != nullptr)
  {
    const ezFloat16* pSize = m_pStreamSize->GetData<ezFloat16>();

    ezSimdVec4f vMax;
    vMax.SetZero();

    constexpr ezUInt32 uiElementsPerLoop = 4;
    for (ezUInt64 i = 0; i < uiNumElements; i += uiElementsPerLoop)
    {
      const float x = pSize[i + 0];
      const float y = pSize[i + 1];
      const float z = pSize[i + 2];
      const float w = pSize[i + 3];

      vMax = vMax.CompMax(ezSimdVec4f(x, y, z, w));
    }

    for (ezUInt64 i = (uiNumElements / uiElementsPerLoop) * uiElementsPerLoop; i < uiNumElements; ++i)
    {
      fMaxSize = ezMath::Max(fMaxSize, (float)pSize[i]);
    }

    fMaxSize = ezMath::Max(fMaxSize, (float)vMax.HorizontalMax<4>());
  }

  GetOwnerSystem()->SetBoundingVolume(ezSimdConversion::ToBBoxSphere(volume), fMaxSize);
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Finalizer_ParticleFinalizer_Volume);
