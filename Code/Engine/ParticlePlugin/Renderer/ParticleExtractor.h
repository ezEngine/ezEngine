#pragma once

#include <ParticlePlugin/Basics.h>
#include <RendererCore/Pipeline/Extractor.h>

class ezView;
class ezExtractedRenderData;

class EZ_PARTICLEPLUGIN_DLL ezParticleExtractor : public ezExtractor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleExtractor, ezExtractor);
public:
  ezParticleExtractor() {}

  virtual void Extract(const ezView& view, ezExtractedRenderData* pExtractedRenderData) override;
};
