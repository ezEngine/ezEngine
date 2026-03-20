#pragma once

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/SSRConstants.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/RendererFoundationDLL.h>

/// AUTHOR: Mikael A. (wdstudiosma)
/// Screen-space reflections pass using Hi-Z ray marching and indirect dispatches.
///
/// Traces reflection rays against a hierarchical depth buffer for pixels that pass a roughness
/// threshold test. Uses a classify-then-trace architecture: a classification compute shader
/// identifies reflective pixels and builds a compacted work list, then the tracing shader
/// is launched via indirect dispatch to process only those pixels. Temporal accumulation
/// stabilizes the result across frames.
///
/// Place this pass after DeferredLighting in the pipeline. It reads the current frame's lit
/// color (via a pass-through pin) and depth/GBuffer data. The SSR result is published through
/// ezSSRDataProvider for DeferredLighting to consume on the next frame.
class EZ_RENDERERCORE_DLL ezSSRPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSSRPass, ezRenderPipelinePass);

public:
  ezSSRPass();
  ~ezSSRPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;
  virtual void InitRenderPipelinePass(const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual void ExecuteInactive(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

  void SetMaxRaySteps(ezUInt32 uiSteps);
  ezUInt32 GetMaxRaySteps() const;
  void SetRoughnessThreshold(float fThreshold);
  float GetRoughnessThreshold() const;

protected:
  void EnsureResources(ezUInt32 uiWidth, ezUInt32 uiHeight);
  void DestroyResources();

  ezRenderPipelineNodeInputPin m_PinDepthStencil;
  ezRenderPipelineNodeInputPin m_PinGBuffer1;
  ezRenderPipelineNodePassThroughPin m_PinColor;

  ezConstantBufferStorageHandle m_hConstantBuffer;

  ezShaderResourceHandle m_hBuildHiZShader;
  ezShaderResourceHandle m_hClassifyShader;
  ezShaderResourceHandle m_hPrepareArgsShader;
  ezShaderResourceHandle m_hTraceShader;
  ezShaderResourceHandle m_hSpatialBlurShader;
  ezShaderResourceHandle m_hTemporalShader;

  // Hi-Z texture with full mip chain
  ezGALTextureHandle m_hHiZTexture;

  // Temporary copy of the Hi-Z texture for ping-pong reads during mip chain construction.
  // Avoids SRV/UAV conflicts when reading the previous mip while writing the next.
  ezGALTextureHandle m_hHiZTemp;

  // Temporal ping-pong SSR result textures
  ezGALTextureHandle m_hSSRResultA;
  ezGALTextureHandle m_hSSRResultB;
  bool m_bUseResultA = true;

  // Raw trace output (written by trace, read by spatial blur)
  ezGALTextureHandle m_hSSRTraceResult;

  // Spatial blur output (written by blur, published as SSR result)
  ezGALTextureHandle m_hSSRBlurIntermediate;

  // Classify output buffer (packed pixel coordinates)
  ezGALBufferHandle m_hClassifyBuffer;

  // Indirect dispatch argument buffer (3 x uint32)
  ezGALBufferHandle m_hIndirectArgsBuffer;

  // Tracked resolution for resource recreation
  ezUInt32 m_uiWidth = 0;
  ezUInt32 m_uiHeight = 0;
  ezUInt32 m_uiHiZMipCount = 0;

  // Properties
  ezUInt32 m_uiMaxRaySteps = 64;
  float m_fThickness = 0.5f;
  float m_fMaxRayDistance = 100.0f;
  float m_fRoughnessThreshold = 0.5f;
  float m_fTemporalBlendWeight = 0.05f;
  float m_fEdgeFadeStart = 0.05f;
  float m_fEdgeFadeEnd = 0.15f;

  ezUInt32 m_uiFrameIndex = 0;
  float m_fBlurRadius = 4.0f;
  float m_fBlurSharpness = 10.0f;

  ezMat4 m_PrevViewProjectionMatrix = ezMat4::MakeIdentity();
};