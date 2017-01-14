#pragma once

#include <ParticlePlugin/Basics.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Containers/DynamicArray.h>
#include <ParticlePlugin/Renderer/ParticleRenderer.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/ParticleSystemConstants.h>
#include <RendererCore/../../../Data/Base/Shaders/Particles/FragmentShaderData.h>

typedef ezTypedResourceHandle<class ezShaderResource> ezShaderResourceHandle;
typedef ezTypedResourceHandle<class ezTexture2DResource> ezTexture2DResourceHandle;

typedef ezRefCountedContainer<ezDynamicArray<ezFragmentParticleData>> ezFragmentParticleDataContainer;

class EZ_PARTICLEPLUGIN_DLL ezParticleFragmentRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleFragmentRenderData, ezRenderData);

public:
  ezParticleFragmentRenderData();

  ezTexture2DResourceHandle m_hTexture;
  ezUInt32 m_uiNumParticles;
  ezSharedPtr<ezFragmentParticleDataContainer> m_GpuData;
};



/// \brief Implements rendering of particle systems
class EZ_PARTICLEPLUGIN_DLL ezParticleFragmentRenderer : public ezParticleRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleFragmentRenderer, ezParticleRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezParticleFragmentRenderer);

public:
  ezParticleFragmentRenderer();
  ~ezParticleFragmentRenderer();

  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) override;

protected:
  void CreateDataBuffer();

  static const ezUInt32 s_uiParticlesPerBatch = 512;
  ezGALBufferHandle m_hDataBuffer;
  ezShaderResourceHandle m_hShader;
};

