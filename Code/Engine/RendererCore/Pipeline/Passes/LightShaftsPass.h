#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>

class ezClusteredDataCPU;

/// Screen-space light shafts (crepuscular rays) post-processing pass.
///
/// Generates volumetric light shaft effects from the brightest directional light by
/// building a mask from the depth buffer and applying a radial blur toward
/// the projected light position. The result is additively composited into the scene color.
class EZ_RENDERERCORE_DLL ezLightShaftsPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLightShaftsPass, ezRenderPipelinePass);

public:
  ezLightShaftsPass();
  ~ezLightShaftsPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;
  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

  void SetDownsampleFactor(ezUInt32 uiFactor);                          // [ property ]
  ezUInt32 GetDownsampleFactor() const { return m_uiDownsampleFactor; } // [ property ]

  void SetNumBlurPasses(ezUInt32 uiPasses);                             // [ property ]
  ezUInt32 GetNumBlurPasses() const { return m_uiNumBlurPasses; }       // [ property ]

  void SetNumSamples(ezUInt32 uiSamples);                               // [ property ]
  ezUInt32 GetNumSamples() const { return m_uiNumSamples; }             // [ property ]

  void SetMaxBlurDistance(float fDistance);                             // [ property ]
  float GetMaxBlurDistance() const { return m_fMaxBlurDistance; }       // [ property ]

protected:
  ezVec4 CalculateOriginUVs(const ezVec3& vLightDirection, const ezRenderViewContext& renderViewContext) const;
  void UpdateConstantBuffer(const ezClusteredDataCPU& clusteredData, const ezVec2& vLightOriginUVs, float fBlurStep);

  ezRenderPipelineNodePassThroughPin m_PinColor;
  ezRenderPipelineNodeInputPin m_PinDepthInput;

  ezConstantBufferStorageHandle m_hConstantBuffer;
  ezShaderResourceHandle m_hMaskShader;
  ezShaderResourceHandle m_hRadialBlurShader;
  ezShaderResourceHandle m_hApplyShader;

  ezEnum<ezGALResourceFormat> m_TextureFormat;

  // Properties
  ezUInt8 m_uiDownsampleFactor = 3;
  ezUInt8 m_uiNumBlurPasses = 3;
  ezUInt8 m_uiNumSamples = 12;
  float m_fMaxBlurDistance = 1.0f;
};
