#include <PCH.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Volume.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Core/World/World.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <Foundation/Math/Declarations.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_Volume, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_Volume>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezHiddenAttribute()
  }
    EZ_END_ATTRIBUTES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Volume, 1, ezRTTIDefaultAllocator<ezParticleBehavior_Volume>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleBehaviorFactory_Volume::ezParticleBehaviorFactory_Volume()
{
}

const ezRTTI* ezParticleBehaviorFactory_Volume::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_Volume>();
}

void ezParticleBehaviorFactory_Volume::CopyBehaviorProperties(ezParticleBehavior* pObject) const
{
  ezParticleBehavior_Volume* pBehavior = static_cast<ezParticleBehavior_Volume*>(pObject);
}


enum class BehaviorVolumeVersion
{
  Version_0 = 0,


  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleBehaviorFactory_Volume::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)BehaviorVolumeVersion::Version_Current;
  stream << uiVersion;
}

void ezParticleBehaviorFactory_Volume::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)BehaviorVolumeVersion::Version_Current, "Invalid version {0}", uiVersion);

}


ezParticleBehavior_Volume::ezParticleBehavior_Volume()
{
}

ezParticleBehavior_Volume::~ezParticleBehavior_Volume()
{
}

void ezParticleBehavior_Volume::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float3, &m_pStreamPosition, false);
  m_pStreamSize = nullptr;
}

void ezParticleBehavior_Volume::QueryOptionalStreams()
{
  m_pStreamSize = GetOwnerSystem()->QueryStream("Size", ezProcessingStream::DataType::Float);
}

void ezParticleBehavior_Volume::Process(ezUInt64 uiNumElements)
{
  const ezVec3* pPosition = m_pStreamPosition->GetData<ezVec3>();

  ezBoundingBoxSphere volume;
  volume.SetFromPoints(pPosition, static_cast<ezUInt32>(uiNumElements));

  float fMaxSize = 0.0f;

  if (m_pStreamSize != nullptr)
  {
    const float* pSize = m_pStreamSize->GetData<float>();

    /// \todo Could unroll this loop to do multiple max computations in parallel and combine at the end
    for (ezUInt64 i = 0; i < uiNumElements; ++i)
    {
      fMaxSize = ezMath::Max(fMaxSize, pSize[i]);
    }
  }

  GetOwnerSystem()->SetBoundingVolume(volume, fMaxSize);
}
