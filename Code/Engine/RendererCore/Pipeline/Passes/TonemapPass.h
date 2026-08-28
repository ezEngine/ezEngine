#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>

struct ezTonemapMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    None,
    Linear,
    Reinhard,
    Filmic,
    ACES,
    AgX,

    Default = Filmic
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezTonemapMode);

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

  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override;
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
  ezTexture2DResourceHandle m_hBlackTexture;                                ///< Black texture for fallback.
  ezTexture3DResourceHandle m_hLUT1;                                        ///< First 3D lookup table for color grading.
  ezTexture3DResourceHandle m_hLUT2;                                        ///< Second 3D lookup table for color grading.

  ezColor m_MoodColor = ezColor::Orange;                                    ///< Mood color tint.
  float m_fMoodStrength = 0.0f;                                             ///< Strength of mood color effect.
  float m_fSaturation = 1.0f;                                               ///< Color saturation multiplier.
  float m_fContrast = 1.0f;                                                 ///< Contrast adjustment.
  float m_fLut1Strength = 1.0f;                                             ///< Blend strength for first LUT.
  float m_fLut2Strength = 0.0f;                                             ///< Blend strength for second LUT.

  ezEnum<ezTonemapMode> m_Mode;                                             ///< Tonemap mode to use.
  bool m_bVisualizeCurve = false;                                           ///< Debug visualization of the tonemap curve.
  ezEnum<ezTonemapMode> m_CompareCurve = ezTonemapMode::None;               ///< Tonemap curve to compare against when visualizing the tonemap curve.

  float m_fWhitePoint = 11.2f;                                              ///< White point for tone curve.
  float m_fFilmicToe = 0.2f;                                                ///< Toe strength for the filmic tonemap curve.
  float m_fFilmicLinearStrength = 0.35f;                                    ///< Linear strength for the filmic tonemap curve.
  float m_fFilmicLinearAngle = 0.1f;                                        ///< Linear angle for the filmic tonemap curve.
  float m_fFilmicShoulder = 0.15f;                                          ///< Shoulder strength for the filmic tonemap curve.

  ezConstantBufferStorageHandle m_hConstantBuffer;                          ///< Constant buffer for tonemap parameters.
  ezShaderResourceHandle m_hShader;                                         ///< Tonemap shader.
};
