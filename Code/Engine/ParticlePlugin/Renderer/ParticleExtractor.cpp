#include <PCH.h>
#include <ParticlePlugin/Renderer/ParticleExtractor.h>
#include <Foundation/Threading/Lock.h>
#include <Core/World/World.h>
#include <RendererCore/Pipeline/View.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleExtractor, 1, ezRTTIDefaultAllocator<ezParticleExtractor>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleExtractor::ezParticleExtractor(const char* szName)
  : ezExtractor(szName)
{
  m_DependsOn.PushBack(ezMakeHashedString("ezVisibleObjectsExtractor"));
}

void ezParticleExtractor::Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects,
  ezExtractedRenderData& extractedRenderData)
{
  EZ_LOCK(view.GetWorld()->GetReadMarker());

  if (const ezParticleWorldModule* pModule = view.GetWorld()->GetModule<ezParticleWorldModule>())
  {
    pModule->ExtractRenderData(view, extractedRenderData);
  }
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Renderer_ParticleExtractor);

