#pragma once

#include <ParticlePlugin/Basics.h>
#include <RendererCore/Pipeline/Declarations.h>

/// \brief Implements rendering of particle systems
class EZ_PARTICLEPLUGIN_DLL ezParticleRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleRenderer, ezRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezParticleRenderer);

public:

  ezParticleRenderer();
  ~ezParticleRenderer();

  // ezRenderer implementation
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) override;

protected:

};

