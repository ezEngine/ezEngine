#pragma once

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/ScreenSpaceShadowConstants.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/RendererFoundationDLL.h>

/// AUTHOR: Mikael A. (wdstudiosma)
/// Screen-space contact shadows using per-pixel ray marching in view space.
///
/// Traces rays from each pixel toward the brightest directional light in view space,
/// projecting each step to screen UV and sampling the depth buffer to detect occlusion.
/// Based on the Spartan Engine approach. Designed to supplement shadow maps with fine
/// contact detail. Requires TAA to resolve temporal noise from the dithered ray offset.
class EZ_RENDERERCORE_DLL ezScreenSpaceShadowPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezScreenSpaceShadowPass, ezRenderPipelinePass);

public:
  ezScreenSpaceShadowPass();
  ~ezScreenSpaceShadowPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;
  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual void ExecuteInactive(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

protected:
  void EnsureResources(ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiArraySize);
  void DestroyResources();

  ezRenderPipelineNodeInputPin m_PinDepthStencil;

  ezConstantBufferStorageHandle m_hConstantBuffer;
  ezShaderResourceHandle m_hShader;

  ezGALTextureHandle m_hShadowTexture;

  ezUInt32 m_uiWidth = 0;
  ezUInt32 m_uiHeight = 0;

  float m_fMaxRayDistance = 0.05f;
  ezUInt32 m_uiMaxSteps = 16;
  float m_fSurfaceThickness = 0.02f;
  float m_fShadowIntensity = 1.0f;

  ezUInt32 m_uiFrameIndex = 0;
};
