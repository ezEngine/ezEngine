#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Renderer/ParticleExtractor.h>
#include <Foundation/Threading/Lock.h>
#include <Core/World/World.h>
#include <RendererCore/Pipeline/View.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleExtractor, 1, ezRTTIDefaultAllocator<ezParticleExtractor>)
EZ_END_DYNAMIC_REFLECTED_TYPE

void ezParticleExtractor::Extract(const ezView& view, ezExtractedRenderData* pExtractedRenderData)
{
  ezParticleWorldModule* pModule = static_cast<ezParticleWorldModule*>(ezWorldModule::FindModule(view.GetWorld(), ezParticleWorldModule::GetStaticRTTI()));

  pModule->ExtractRenderData(view, pExtractedRenderData);
}
