#pragma once

#include <ParticlePlugin/Basics.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Containers/DynamicArray.h>
#include <ParticlePlugin/Renderer/ParticleRenderer.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/ParticleSystemConstants.h>
#include <RendererCore/../../../Data/Base/Shaders/Particles/SpriteShaderData.h>

typedef ezTypedResourceHandle<class ezShaderResource> ezShaderResourceHandle;
typedef ezTypedResourceHandle<class ezTextureResource> ezTextureResourceHandle;

typedef ezRefCountedContainer<ezDynamicArray<ezSpriteParticleData>> ezSpriteParticleDataContainer;

class EZ_PARTICLEPLUGIN_DLL ezParticleSpriteRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleSpriteRenderData, ezRenderData);

public:
  ezParticleSpriteRenderData();

  ezTextureResourceHandle m_hTexture;
  ezUInt32 m_uiNumParticles;
  ezSharedPtr<ezSpriteParticleDataContainer> m_GpuData;
};



/// \brief Implements rendering of particle systems
class EZ_PARTICLEPLUGIN_DLL ezParticleSpriteRenderer : public ezParticleRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleSpriteRenderer, ezParticleRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezParticleSpriteRenderer);

public:
  ezParticleSpriteRenderer();
  ~ezParticleSpriteRenderer();

  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) override;

protected:
  void CreateDataBuffer();

  static const ezUInt32 s_uiMaxBufferSize = 256 * 1024;
  ezGALBufferHandle m_hDataBuffer;
  ezShaderResourceHandle m_hShader;
};

