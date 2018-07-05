#pragma once

#include <ParticlePlugin/Basics.h>
#include <ParticlePlugin/Declarations.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/DynamicArray.h>
#include <ParticlePlugin/Renderer/ParticleRenderer.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/TangentQuadParticleShaderData.h>

typedef ezTypedResourceHandle<class ezShaderResource> ezShaderResourceHandle;
typedef ezTypedResourceHandle<class ezTexture2DResource> ezTexture2DResourceHandle;

class EZ_PARTICLEPLUGIN_DLL ezParticleSpriteRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleSpriteRenderData, ezRenderData);

public:
  ezEnum<ezParticleTypeRenderMode> m_RenderMode;
  ezTexture2DResourceHandle m_hTexture;
  ezArrayPtr<ezBaseParticleShaderData> m_BaseParticleData;
  ezArrayPtr<ezTangentQuadParticleShaderData> m_QuadParticleData;
  bool m_bApplyObjectTransform = true;
  ezUInt8 m_uiNumSpritesX = 1;
  ezUInt8 m_uiNumSpritesY = 1;
};

/// \brief Implements rendering of particle systems
class EZ_PARTICLEPLUGIN_DLL ezParticleSpriteRenderer : public ezParticleRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleSpriteRenderer, ezParticleRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezParticleSpriteRenderer);

public:
  ezParticleSpriteRenderer() {}
  ~ezParticleSpriteRenderer();

  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) override;

protected:
  void CreateDataBuffer();

  static const ezUInt32 s_uiParticlesPerBatch = 512;
  ezGALBufferHandle m_hBaseDataBuffer;
  ezGALBufferHandle m_hQuadDataBuffer;
  ezShaderResourceHandle m_hShader;
};

