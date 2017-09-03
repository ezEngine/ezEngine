#pragma once

#include <ParticlePlugin/Basics.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/DynamicArray.h>
#include <ParticlePlugin/Renderer/ParticleRenderer.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/ParticleSystemConstants.h>
#include <RendererCore/../../../Data/Base/Shaders/Particles/PointShaderData.h>

typedef ezTypedResourceHandle<class ezShaderResource> ezShaderResourceHandle;

class EZ_PARTICLEPLUGIN_DLL ezParticlePointRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticlePointRenderData, ezRenderData);

public:
  ezArrayPtr<ezPointParticleData> m_ParticleData;
  bool m_bApplyObjectTransform = true;
};

/// \brief Implements rendering of particle systems
class EZ_PARTICLEPLUGIN_DLL ezParticlePointRenderer : public ezParticleRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticlePointRenderer, ezParticleRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezParticlePointRenderer);

public:
  ezParticlePointRenderer() {}
  ~ezParticlePointRenderer();

  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) override;

protected:
  void CreateDataBuffer();

  static const ezUInt32 s_uiParticlesPerBatch = 1024;
  ezGALBufferHandle m_hDataBuffer;
  ezShaderResourceHandle m_hShader;
};

