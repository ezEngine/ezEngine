#pragma once

#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/ParticlePluginDLL.h>
#include <ParticlePlugin/Renderer/ParticleRenderer.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererFoundation/Resources/BufferPool.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/TrailShaderData.h>

/// Render data for trail particles.
class EZ_PARTICLEPLUGIN_DLL ezParticleTrailRenderData final : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTrailRenderData, ezRenderData);

public:
  virtual bool CanBatch(const ezRenderData& other) const override;

  ezTexture2DResourceHandle m_hTexture;
  ezMaterialResourceHandle m_hCustomMaterial;
  ezUInt16 m_uiMaxTrailPoints;
  float m_fSnapshotFraction;
  ezArrayPtr<ezBaseParticleShaderData> m_BaseParticleData;
  ezArrayPtr<ezTrailParticleShaderData> m_TrailParticleData;
  ezArrayPtr<ezVec4> m_TrailPointsShared;
  ezTransform m_GlobalTransform;
  ezTime m_TotalEffectLifeTime;
  ezUInt8 m_uiNumVariationsX = 1;
  ezUInt8 m_uiNumVariationsY = 1;
  ezUInt8 m_uiNumFlipbookAnimationsX = 1;
  ezUInt8 m_uiNumFlipbookAnimationsY = 1;

  ezEnum<ezParticleTypeRenderMode> m_RenderMode;
  ezEnum<ezParticleLightingMode> m_LightingMode;
  float m_fNormalCurvature = 0.5f;
  float m_fLightDirectionality = 0.5f;
};

/// \brief Implements rendering of a trail particle systems
class EZ_PARTICLEPLUGIN_DLL ezParticleTrailRenderer final : public ezParticleRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTrailRenderer, ezParticleRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezParticleTrailRenderer);

public:
  ezParticleTrailRenderer();
  ~ezParticleTrailRenderer();

  virtual void GetSupportedRenderDataTypes(ezDynamicArray<const ezRTTI*>& out_types) const override;
  virtual void RenderBatch(
    const ezRenderViewContext& renderContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const override;

protected:
  bool ConfigureShader(const ezParticleTrailRenderData* pRenderData, const ezRenderViewContext& renderViewContext) const;

  static const ezUInt32 s_uiParticlesPerBatch = 512;
  ezGALBufferPool m_BaseDataBuffer;
  ezGALBufferPool m_TrailDataBuffer;
  ezGALBufferPool m_TrailPointsDataBuffer8;
  ezGALBufferPool m_TrailPointsDataBuffer16;
  ezGALBufferPool m_TrailPointsDataBuffer32;
  ezGALBufferPool m_TrailPointsDataBuffer64;

  mutable const ezGALBufferPool* m_pActiveTrailPointsDataBuffer = nullptr;
};
