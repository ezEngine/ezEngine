#pragma once

#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/ParticlePluginDLL.h>
#include <ParticlePlugin/Renderer/ParticleRenderer.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/TrailShaderData.h>

class EZ_PARTICLEPLUGIN_DLL ezParticleTrailRenderData final : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTrailRenderData, ezRenderData);

public:
  ezTexture2DResourceHandle m_hTexture;
  ezUInt32 m_uiMaxTrailPoints;
  float m_fSnapshotFraction;
  ezArrayPtr<ezBaseParticleShaderData> m_BaseParticleData;
  ezArrayPtr<ezTrailParticleShaderData> m_TrailParticleData;
  ezArrayPtr<ezVec4> m_TrailPointsShared;
  ezEnum<ezParticleTypeRenderMode> m_RenderMode;
  bool m_bApplyObjectTransform = true;
  ezTime m_TotalEffectLifeTime;
  ezUInt8 m_uiNumVariationsX = 1;
  ezUInt8 m_uiNumVariationsY = 1;
  ezUInt8 m_uiNumFlipbookAnimationsX = 1;
  ezUInt8 m_uiNumFlipbookAnimationsY = 1;
};

/// \brief Implements rendering of a trail particle systems
class EZ_PARTICLEPLUGIN_DLL ezParticleTrailRenderer final : public ezParticleRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTrailRenderer, ezParticleRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezParticleTrailRenderer);

public:
  ezParticleTrailRenderer();
  ~ezParticleTrailRenderer();

  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) const override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const override;

protected:
  bool ConfigureShader(const ezParticleTrailRenderData* pRenderData, const ezRenderViewContext& renderViewContext) const;

  static const ezUInt32 s_uiParticlesPerBatch = 512;
  ezGALBufferHandle m_hBaseDataBuffer;
  ezGALBufferHandle m_hTrailDataBuffer;
  ezGALBufferHandle m_hTrailPointsDataBuffer8;
  ezGALBufferHandle m_hTrailPointsDataBuffer16;
  ezGALBufferHandle m_hTrailPointsDataBuffer32;
  ezGALBufferHandle m_hTrailPointsDataBuffer64;

  mutable ezGALBufferHandle m_hActiveTrailPointsDataBuffer;
};
