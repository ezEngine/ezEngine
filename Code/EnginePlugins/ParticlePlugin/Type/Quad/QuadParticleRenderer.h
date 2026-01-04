#pragma once

#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/Renderer/ParticleRenderer.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererFoundation/Resources/BufferPool.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/BillboardQuadParticleShaderData.h>
#include <RendererCore/../../../Data/Base/Shaders/Particles/TangentQuadParticleShaderData.h>

/// Render data for quad particles.
class EZ_PARTICLEPLUGIN_DLL ezParticleQuadRenderData final : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleQuadRenderData, ezRenderData);

public:
  virtual bool CanBatch(const ezRenderData& other) const override;

  ezTexture2DResourceHandle m_hTexture;
  ezArrayPtr<ezBaseParticleShaderData> m_BaseParticleData;
  ezArrayPtr<ezBillboardQuadParticleShaderData> m_BillboardParticleData;
  ezArrayPtr<ezTangentQuadParticleShaderData> m_TangentParticleData;
  ezTransform m_GlobalTransform;
  ezTime m_TotalEffectLifeTime;
  ezUInt8 m_uiNumVariationsX = 1;
  ezUInt8 m_uiNumVariationsY = 1;
  ezUInt8 m_uiNumFlipbookAnimationsX = 1;
  ezUInt8 m_uiNumFlipbookAnimationsY = 1;

  ezTempHashedString m_QuadModePermutation;

  ezEnum<ezParticleTypeRenderMode> m_RenderMode;
  ezEnum<ezParticleLightingMode> m_LightingMode;
  float m_fNormalCurvature = 0.5f;
  float m_fLightDirectionality = 0.5f;
  ezMaterialResourceHandle m_hCustomMaterial;
};

/// Renderer for quad particle systems.
class EZ_PARTICLEPLUGIN_DLL ezParticleQuadRenderer final : public ezParticleRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleQuadRenderer, ezParticleRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezParticleQuadRenderer);

public:
  ezParticleQuadRenderer();
  ~ezParticleQuadRenderer();

  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const override;


protected:
  void ConfigureRenderMode(const ezParticleQuadRenderData* pRenderData, ezRenderContext* pRenderContext) const;

  static const ezUInt32 s_uiParticlesPerBatch = 1024;
  ezGALBufferPool m_BaseDataBuffer;
  ezGALBufferPool m_BillboardDataBuffer;
  ezGALBufferPool m_TangentDataBuffer;
};
