#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture3DResource.h>

/// Render pass that applies tonemapping and color grading to HDR images.
///
/// Converts high dynamic range color values to display-ready output by applying
/// exposure adjustment, color grading via lookup tables, saturation, contrast, and
/// optional effects like vignetting and mood color. Combines with bloom if provided.
class EZ_RENDERERCORE_DLL ezTonemapPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTonemapPass, ezRenderPipelinePass);

public:
  ezTonemapPass();
  ~ezTonemapPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;

  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

protected:
  ezRenderPipelineNodeInputPin m_PinColorInput;                             ///< HDR color input to tonemap.
  ezRenderPipelineNodeInputPin m_PinBloomInput;                             ///< Optional bloom texture to add.
  ezRenderPipelineNodeOutputPin m_PinOutput;                                ///< LDR output after tonemapping.

  EZ_ADD_RESOURCEHANDLE_ACCESSORS(VignettingTexture, m_hVignettingTexture); ///< Vignette texture.
  EZ_ADD_RESOURCEHANDLE_ACCESSORS(LUT1Texture, m_hLUT1);                    ///< Primary color grading LUT.
  EZ_ADD_RESOURCEHANDLE_ACCESSORS(LUT2Texture, m_hLUT2);                    ///< Secondary color grading LUT.

  ezTexture2DResourceHandle m_hVignettingTexture;                           ///< Vignetting effect texture.
  ezTexture2DResourceHandle m_hNoiseTexture;                                ///< Film grain noise texture.
  ezTexture3DResourceHandle m_hLUT1;                                        ///< First 3D lookup table for color grading.
  ezTexture3DResourceHandle m_hLUT2;                                        ///< Second 3D lookup table for color grading.

  ezColor m_MoodColor;                                                      ///< Mood color tint.
  float m_fMoodStrength;                                                    ///< Strength of mood color effect.
  float m_fSaturation;                                                      ///< Color saturation multiplier.
  float m_fContrast;                                                        ///< Contrast adjustment.
  float m_fLut1Strength;                                                    ///< Blend strength for first LUT.
  float m_fLut2Strength;                                                    ///< Blend strength for second LUT.
  float m_fWhitePoint;                                                      ///< White point for tone curve.

  ezConstantBufferStorageHandle m_hConstantBuffer;                          ///< Constant buffer for tonemap parameters.
  ezShaderResourceHandle m_hShader;                                         ///< Tonemap shader.
};
