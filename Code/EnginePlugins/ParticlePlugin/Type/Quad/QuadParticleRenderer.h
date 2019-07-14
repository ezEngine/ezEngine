#pragma once

#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/Renderer/ParticleRenderer.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/BillboardQuadParticleShaderData.h>
#include <RendererCore/../../../Data/Base/Shaders/Particles/TangentQuadParticleShaderData.h>

class EZ_PARTICLEPLUGIN_DLL ezParticleQuadRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleQuadRenderData, ezRenderData);

public:
  ezEnum<ezParticleTypeRenderMode> m_RenderMode;
  ezTexture2DResourceHandle m_hTexture;
  ezArrayPtr<ezBaseParticleShaderData> m_BaseParticleData;
  ezArrayPtr<ezBillboardQuadParticleShaderData> m_BillboardParticleData;
  ezArrayPtr<ezTangentQuadParticleShaderData> m_TangentParticleData;
  bool m_bApplyObjectTransform = true;
  ezUInt8 m_uiNumVariationsX = 1;
  ezUInt8 m_uiNumVariationsY = 1;
  ezUInt8 m_uiNumFlipbookAnimationsX = 1;
  ezUInt8 m_uiNumFlipbookAnimationsY = 1;

  ezTexture2DResourceHandle m_hDistortionTexture;
  float m_fDistortionStrength = 0;
  ezTempHashedString m_QuadModePermutation;
};

/// \brief Implements rendering of particle systems
class EZ_PARTICLEPLUGIN_DLL ezParticleQuadRenderer : public ezParticleRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleQuadRenderer, ezParticleRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezParticleQuadRenderer);

public:
  ezParticleQuadRenderer();
  ~ezParticleQuadRenderer();

  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) const override;
  virtual void RenderBatch(
    const ezRenderViewContext& renderContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const override;


protected:
  void ConfigureRenderMode(const ezParticleQuadRenderData* pRenderData, ezRenderContext* pRenderContext) const;

  static const ezUInt32 s_uiParticlesPerBatch = 1024;
  ezGALBufferHandle m_hBaseDataBuffer;
  ezGALBufferHandle m_hBillboardDataBuffer;
  ezGALBufferHandle m_hTangentDataBuffer;
};
