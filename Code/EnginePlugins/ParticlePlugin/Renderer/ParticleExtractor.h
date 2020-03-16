#pragma once

#include <ParticlePlugin/ParticlePluginDLL.h>
#include <RendererCore/Pipeline/Extractor.h>

class ezView;
class ezExtractedRenderData;

class EZ_PARTICLEPLUGIN_DLL ezParticleExtractor final : public ezExtractor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleExtractor, ezExtractor);
public:
  ezParticleExtractor(const char* szName = "ParticleExtractor");

  virtual void Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects,
    ezExtractedRenderData& extractedRenderData) override;
};
