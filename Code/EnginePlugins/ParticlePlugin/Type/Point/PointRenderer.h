#pragma once

#include <ParticlePlugin/ParticlePluginDLL.h>
#include <ParticlePlugin/Renderer/ParticleRenderer.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/BillboardQuadParticleShaderData.h>
#include <RendererFoundation/Resources/BufferPool.h>

class EZ_PARTICLEPLUGIN_DLL ezParticlePointRenderData final : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticlePointRenderData, ezRenderData);

public:
  ezArrayPtr<ezBaseParticleShaderData> m_BaseParticleData;
  ezArrayPtr<ezBillboardQuadParticleShaderData> m_BillboardParticleData;
  bool m_bApplyObjectTransform = true;
  ezTime m_TotalEffectLifeTime;
};

/// \brief Implements rendering of particle systems
class EZ_PARTICLEPLUGIN_DLL ezParticlePointRenderer final : public ezParticleRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticlePointRenderer, ezParticleRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezParticlePointRenderer);

public:
  ezParticlePointRenderer();
  ~ezParticlePointRenderer();

  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const override;
  virtual void RenderBatch(
    const ezRenderViewContext& renderContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const override;

protected:
  static const ezUInt32 s_uiParticlesPerBatch = 1024;
  ezGALBufferPool m_BaseDataBuffer;
  ezGALBufferPool m_BillboardDataBuffer;
};
