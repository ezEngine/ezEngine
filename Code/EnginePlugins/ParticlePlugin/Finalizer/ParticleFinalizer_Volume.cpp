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
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleFinalizer_Volume, 1, ezRTTIDefaultAllocator<ezParticleFinalizer_Volume>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleFinalizer_Volume::ezParticleFinalizer_Volume() = default;
ezParticleFinalizer_Volume::~ezParticleFinalizer_Volume() = default;

bool ezParticleFinalizer_Volume::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void ezParticleFinalizer_Volume::CreateRequiredStreams(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  ctx.CreateStream(ezParticleStreamNames::s_sPosition, ezProcessingStream::DataType::Float4, &data.m_pStreamPosition, false);
  data.m_pStreamSize = nullptr;
}

void ezParticleFinalizer_Volume::QueryOptionalStreams(const ezParticleModuleContext& ctx) const
{
  ctx.GetInstanceData<InstanceData>().m_pStreamSize = ctx.m_pSystem->QueryStream(ezParticleStreamNames::s_sSize, ezProcessingStream::DataType::Half);
}

void ezParticleFinalizer_Volume::Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const
{
  if (uiNumElements == 0)
    return;

  EZ_PROFILE_SCOPE("PFX: Volume");

  const InstanceData& data = ctx.GetInstanceData<InstanceData>();

  const ezSimdVec4f* pPosition = data.m_pStreamPosition->GetData<ezSimdVec4f>();

  const ezSimdBBoxSphere volume = ezSimdBBoxSphere::MakeFromPoints(pPosition, static_cast<ezUInt32>(uiNumElements));

  float fMaxSize = 0;

  if (data.m_pStreamSize != nullptr)
  {
    const ezFloat16* pSize = data.m_pStreamSize->GetData<ezFloat16>();

    ezSimdVec4f vMax;
    vMax.SetZero();

    // only full blocks, the data after the active elements is stale
    constexpr ezUInt32 uiElementsPerLoop = 4;
    for (ezUInt64 i = 0; i + uiElementsPerLoop <= uiNumElements; i += uiElementsPerLoop)
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

  ctx.m_pSystem->SetBoundingVolume(ezSimdConversion::ToBBoxSphere(volume), fMaxSize);
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Finalizer_ParticleFinalizer_Volume);
