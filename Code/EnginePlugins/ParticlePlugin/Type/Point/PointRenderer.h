#pragma once

#include <ParticlePlugin/ParticlePluginDLL.h>
#include <ParticlePlugin/Renderer/ParticleRenderer.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/BillboardQuadParticleShaderData.h>
#include <RendererFoundation/Resources/BufferPool.h>

/// Render data for point particles.
class EZ_PARTICLEPLUGIN_DLL ezParticlePointRenderData final : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticlePointRenderData, ezRenderData);

public:
  ezArrayPtr<ezBaseParticleShaderData> m_BaseParticleData;               ///< Base particle data
  ezArrayPtr<ezBillboardQuadParticleShaderData> m_BillboardParticleData; ///< Billboard data
  ezTransform m_GlobalTransform;                                         ///< World transform of the particle system
  ezTime m_TotalEffectLifeTime;                                          ///< Total lifetime of the effect
};

/// Renderer for point particle systems.
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
