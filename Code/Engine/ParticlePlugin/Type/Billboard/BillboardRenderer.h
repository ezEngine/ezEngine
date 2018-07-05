#pragma once

#include <ParticlePlugin/Basics.h>
#include <ParticlePlugin/Declarations.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/DynamicArray.h>
#include <ParticlePlugin/Renderer/ParticleRenderer.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/ParticleSystemConstants.h>
#include <RendererCore/../../../Data/Base/Shaders/Particles/BillboardShaderData.h>

typedef ezTypedResourceHandle<class ezShaderResource> ezShaderResourceHandle;
typedef ezTypedResourceHandle<class ezTexture2DResource> ezTexture2DResourceHandle;

class EZ_PARTICLEPLUGIN_DLL ezParticleBillboardRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBillboardRenderData, ezRenderData);

public:
  ezEnum<ezParticleTypeRenderMode> m_RenderMode;
  ezTexture2DResourceHandle m_hTexture;
  ezArrayPtr<ezBaseParticleShaderData> m_BaseParticleData;
  ezArrayPtr<ezBillboardParticleData> m_BillboardParticleData;
  bool m_bApplyObjectTransform = true;
  ezUInt8 m_uiNumSpritesX = 1;
  ezUInt8 m_uiNumSpritesY = 1;
};

/// \brief Implements rendering of particle systems
class EZ_PARTICLEPLUGIN_DLL ezParticleBillboardRenderer : public ezParticleRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBillboardRenderer, ezParticleRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezParticleBillboardRenderer);

public:
  ezParticleBillboardRenderer() {}
  ~ezParticleBillboardRenderer();

  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) override;

protected:
  void CreateDataBuffer();

  static const ezUInt32 s_uiParticlesPerBatch = 1024;
  ezGALBufferHandle m_hBaseDataBuffer;
  ezGALBufferHandle m_hBillboardDataBuffer;
  ezShaderResourceHandle m_hShader;
};

