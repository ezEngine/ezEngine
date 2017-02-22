#pragma once

#include <ParticlePlugin/Basics.h>
#include <ParticlePlugin/Declarations.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/DynamicArray.h>
#include <ParticlePlugin/Renderer/ParticleRenderer.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/ParticleSystemConstants.h>
#include <RendererCore/../../../Data/Base/Shaders/Particles/TrailShaderData.h>

typedef ezTypedResourceHandle<class ezShaderResource> ezShaderResourceHandle;
typedef ezTypedResourceHandle<class ezTexture2DResource> ezTexture2DResourceHandle;

class EZ_PARTICLEPLUGIN_DLL ezParticleTrailRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTrailRenderData, ezRenderData);

public:
  ezTexture2DResourceHandle m_hTexture;
  ezUInt32 m_uiMaxTrailPoints;
  float m_fSnapshotFraction;
  ezArrayPtr<ezTrailParticleData> m_ParticleDataShared;
  ezArrayPtr<ezVec4> m_TrailPointsShared;
  ezEnum<ezParticleTypeRenderMode> m_RenderMode;
};

/// \brief Implements rendering of a trail particle systems
class EZ_PARTICLEPLUGIN_DLL ezParticleTrailRenderer : public ezParticleRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTrailRenderer, ezParticleRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezParticleTrailRenderer);

public:
  ezParticleTrailRenderer() {}
  ~ezParticleTrailRenderer();

  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) override;

  bool ConfigureShader(const ezParticleTrailRenderData* pRenderData, const ezRenderViewContext &renderViewContext);

protected:
  void CreateDataBuffer();

  static const ezUInt32 s_uiParticlesPerBatch = 512;
  ezGALBufferHandle m_hParticleDataBuffer;
  ezGALBufferHandle m_hTrailPointsDataBuffer8;
  ezGALBufferHandle m_hTrailPointsDataBuffer16;
  ezGALBufferHandle m_hTrailPointsDataBuffer32;
  ezGALBufferHandle m_hTrailPointsDataBuffer64;
  ezGALBufferHandle m_hActiveTrailPointsDataBuffer;
  ezShaderResourceHandle m_hShader;
};

