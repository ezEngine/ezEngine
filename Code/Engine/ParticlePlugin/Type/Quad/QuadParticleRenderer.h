#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <ParticlePlugin/Basics.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/Renderer/ParticleRenderer.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Shader/ShaderResource.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/BillboardQuadParticleShaderData.h>
#include <RendererCore/../../../Data/Base/Shaders/Particles/TangentQuadParticleShaderData.h>

typedef ezTypedResourceHandle<class ezShaderResource> ezShaderResourceHandle;
typedef ezTypedResourceHandle<class ezTexture2DResource> ezTexture2DResourceHandle;

class EZ_PARTICLEPLUGIN_DLL ezParticleQuadRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleQuadRenderData, ezRenderData);

public:
  ezEnum<ezParticleTypeRenderMode> m_RenderMode;
  ezTexture2DResourceHandle m_hTexture;
  ezArrayPtr<ezBaseParticleShaderData> m_BaseParticleData;
  ezArrayPtr<ezBillboardQuadParticleShaderData> m_BillboardParticleData;
  ezArrayPtr<ezTangentQuadParticleShaderData> m_TangentParticleData;
  bool m_bApplyObjectTransform = true;
  ezUInt8 m_uiNumSpritesX = 1;
  ezUInt8 m_uiNumSpritesY = 1;

  ezTexture2DResourceHandle m_hDistortionTexture;
  float m_fDistortionStrength = 0;
};

/// \brief Implements rendering of particle systems
class EZ_PARTICLEPLUGIN_DLL ezParticleQuadRenderer : public ezParticleRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleQuadRenderer, ezParticleRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezParticleQuadRenderer);

public:
  ezParticleQuadRenderer();
  ~ezParticleQuadRenderer();

  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) override;

protected:
  void CreateDataBuffer();

  static const ezUInt32 s_uiParticlesPerBatch = 1024;
  ezGALBufferHandle m_hBaseDataBuffer;
  ezGALBufferHandle m_hBillboardDataBuffer;
  ezGALBufferHandle m_hTangentDataBuffer;
  ezShaderResourceHandle m_hShader;
};
