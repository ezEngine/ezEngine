#pragma once

#include <ParticlePlugin/Basics.h>
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
  ezArrayPtr<ezTrailParticleData> m_ParticleDataShared;
  ezArrayPtr<ezTrailParticlePointsData> m_TrailPointsShared;
};



/// \brief Implements rendering of particle systems
class EZ_PARTICLEPLUGIN_DLL ezParticleTrailRenderer : public ezParticleRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTrailRenderer, ezParticleRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezParticleTrailRenderer);

public:
  ezParticleTrailRenderer() {}
  ~ezParticleTrailRenderer();

  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) override;

protected:
  void CreateDataBuffer();

  static const ezUInt32 s_uiParticlesPerBatch = 512;
  ezGALBufferHandle m_hParticleDataBuffer;
  ezGALBufferHandle m_hTrailPointsDataBuffer;
  ezShaderResourceHandle m_hShader;
};

