#include <PCH.h>
#include <ParticlePlugin/Renderer/ParticleExtractor.h>
#include <Foundation/Threading/Lock.h>
#include <Core/World/World.h>
#include <RendererCore/Pipeline/View.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleExtractor, 1, ezRTTIDefaultAllocator<ezParticleExtractor>)
EZ_END_DYNAMIC_REFLECTED_TYPE

void ezParticleExtractor::Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects,
  ezExtractedRenderData* pExtractedRenderData)
{
  EZ_LOCK(view.GetWorld()->GetReadMarker());

  if (const ezParticleWorldModule* pModule = view.GetWorld()->GetModule<ezParticleWorldModule>())
  {
    pModule->ExtractRenderData(view, pExtractedRenderData);
  }
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Renderer_ParticleExtractor);

