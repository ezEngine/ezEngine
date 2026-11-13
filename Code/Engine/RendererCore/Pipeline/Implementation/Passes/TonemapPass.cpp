#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/Passes/TonemapPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <Shaders/Pipeline/TonemapConstants.h>

static_assert(ezTonemapMode::None == TONEMAPMODE_NONE);
static_assert(ezTonemapMode::Linear == TONEMAPMODE_LINEAR);
static_assert(ezTonemapMode::Reinhard == TONEMAPMODE_REINHARD);
static_assert(ezTonemapMode::Filmic == TONEMAPMODE_FILMIC);
static_assert(ezTonemapMode::ACES == TONEMAPMODE_ACES);
static_assert(ezTonemapMode::AgX == TONEMAPMODE_AGX);

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTonemapMode, 1)
  EZ_ENUM_CONSTANTS(ezTonemapMode::None, ezTonemapMode::Linear, ezTonemapMode::Reinhard, ezTonemapMode::Filmic, ezTonemapMode::ACES, ezTonemapMode::AgX)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTonemapPass, 3, ezRTTIDefaultAllocator<ezTonemapPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_PinColorInput),
    EZ_MEMBER_PROPERTY("Bloom", m_PinBloomInput),
    EZ_MEMBER_PROPERTY("Output", m_PinOutput),

    EZ_RESOURCE_MEMBER_PROPERTY("VignettingTexture", m_hVignettingTexture)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Texture_2D"), new ezDefaultValueAttribute("White.color")),
    EZ_MEMBER_PROPERTY("MoodColor", m_MoodColor)->AddAttributes(new ezDefaultValueAttribute(ezColor::Orange)),
    EZ_MEMBER_PROPERTY("MoodStrength", m_fMoodStrength)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Saturation", m_fSaturation)->AddAttributes(new ezClampValueAttribute(0.0f, 2.0f), new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("Contrast", m_fContrast)->AddAttributes(new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("LUT1Strength", m_fLut1Strength)->AddAttributes(new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("LUT2Strength", m_fLut2Strength)->AddAttributes(new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_RESOURCE_MEMBER_PROPERTY("LUT1", m_hLUT1)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Texture_3D")),
    EZ_RESOURCE_MEMBER_PROPERTY("LUT2", m_hLUT2)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Texture_3D")),

    EZ_MEMBER_PROPERTY("WhitePoint", m_fWhitePoint)->AddAttributes(new ezClampValueAttribute(0.0f, 50.0f), new ezDefaultValueAttribute(11.2f)),

    EZ_ENUM_ACCESSOR_PROPERTY("Mode", ezTonemapMode, GetMode, SetMode),

    EZ_ACCESSOR_PROPERTY("FilmicToe", GetFilmicToe, SetFilmicToe)->AddAttributes(new ezClampValueAttribute(0.0f, 100.0f), new ezDefaultValueAttribute(0.2f)),
    EZ_ACCESSOR_PROPERTY("FilmicLinearStrength", GetFilmicLinearStrength, SetFilmicLinearStrength)->AddAttributes(new ezClampValueAttribute(0.0f, 100.0f), new ezDefaultValueAttribute(0.35f)),
    EZ_ACCESSOR_PROPERTY("FilmicLinearAngle", GetFilmicLinearAngle, SetFilmicLinearAngle)->AddAttributes(new ezClampValueAttribute(0.0f, 100.0f), new ezDefaultValueAttribute(0.1f)),
    EZ_ACCESSOR_PROPERTY("FilmicShoulder", GetFilmicShoulder, SetFilmicShoulder)->AddAttributes(new ezClampValueAttribute(0.01f, 100.0f), new ezDefaultValueAttribute(0.15f)),

    EZ_ACCESSOR_PROPERTY("AcesToe", GetAcesToe, SetAcesToe)->AddAttributes(new ezClampValueAttribute(0.0f, 1.0f), new ezDefaultValueAttribute(0.55f)),
    EZ_ACCESSOR_PROPERTY("AcesSlope", GetAcesSlope, SetAcesSlope)->AddAttributes(new ezClampValueAttribute(0.1f, 1.0f), new ezDefaultValueAttribute(0.71f)),
    EZ_ACCESSOR_PROPERTY("AcesShoulder", GetAcesShoulder, SetAcesShoulder)->AddAttributes(new ezClampValueAttribute(0.01f, 1.0f), new ezDefaultValueAttribute(0.47f)),

    EZ_ACCESSOR_PROPERTY("AgXContrast", GetAgXContrast, SetAgXContrast)->AddAttributes(new ezClampValueAttribute(1.0f, 10.0f), new ezDefaultValueAttribute(1.5f)),
    EZ_ACCESSOR_PROPERTY("AgXMidGrey", GetAgXMidGrey, SetAgXMidGrey)->AddAttributes(new ezClampValueAttribute(0.1f, 0.5f), new ezDefaultValueAttribute(0.18f)),

    EZ_MEMBER_PROPERTY("VisualizeCurve", m_bVisualizeCurve),
    EZ_ENUM_MEMBER_PROPERTY("CompareCurve", ezTonemapMode, m_CompareCurve)->AddAttributes(new ezDefaultValueAttribute(ezTonemapMode::None)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Post Processing")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezTonemapPass::ezTonemapPass()
  : ezRenderPipelinePass("TonemapPass", true)
{
  m_hVignettingTexture = ezResourceManager::LoadResource<ezTexture2DResource>("White.color");
  m_hNoiseTexture = ezResourceManager::LoadResource<ezTexture2DResource>("Textures/BlueNoise.dds");
  m_hBlackTexture = ezResourceManager::LoadResource<ezTexture2DResource>("Black.color");

  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/Tonemap.ezShader");
  EZ_ASSERT_DEV(m_hShader.IsValid(), "Could not load tonemap shader!");

  m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezTonemapConstants>();
}

ezTonemapPass::~ezTonemapPass()
{
  ezRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

ezStatus ezTonemapPass::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs)
{
  ezRenderGraphTextureHandle hColorInput = inputs[m_PinColorInput.m_uiInputIndex].m_TextureHandle;
  if (hColorInput.IsInvalidated())
    return ezStatus(ezFmt("Color input: Not connected"));

  const ezGALTextureCreationDescription colorInputDesc = ref_graph.GetTextureDesc(hColorInput);

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  const ezGALRenderTargets& renderTargets = viewData.GetActiveRenderTargets();
  const ezGALTexture* pTexture = pDevice->GetTexture(renderTargets.m_hRTs[0]);
  if (pTexture == nullptr)
    return ezStatus(ezFmt("View does not have a valid color target"));

  const ezGALTextureCreationDescription& rtDesc = pTexture->GetDescription();
  ezGALTextureCreationDescription outputDesc;
  outputDesc.SetAsRenderTarget(colorInputDesc.m_uiWidth, colorInputDesc.m_uiHeight, rtDesc.m_Format);
  outputDesc.m_Type = colorInputDesc.m_Type;
  outputDesc.m_uiArraySize = colorInputDesc.m_uiArraySize;
  ezRenderGraphTextureHandle hOutput = ref_graph.CreateTexture(outputDesc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hOutput;

  ezRenderGraphTextureHandle hBloomInput = inputs[m_PinBloomInput.m_uiInputIndex].m_TextureHandle;

  auto pass = ref_graph.AddGraphicsPass("Tonemap");
  pass.AddColorTarget(hOutput);
  pass.ReadTexture(hColorInput, {}, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
  if (!hBloomInput.IsInvalidated())
    pass.ReadTexture(hBloomInput, {}, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
  pass.SetStereoscopic(camera.IsStereoscopic());
  pass.SetExecuteCallback(
    [=](const ezRenderGraphContext& ctx)
    {
      const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();

      // Determine how many LUTs are active
      ezUInt32 numLUTs = 0;
      ezTexture3DResourceHandle luts[2] = {};
      float lutStrengths[2] = {};

      if (m_hLUT1.IsValid())
      {
        luts[numLUTs] = m_hLUT1;
        lutStrengths[numLUTs] = m_fLut1Strength;
        numLUTs++;
      }

      if (m_hLUT2.IsValid())
      {
        luts[numLUTs] = m_hLUT2;
        lutStrengths[numLUTs] = m_fLut2Strength;
        numLUTs++;
      }

      {
        ezTonemapConstants* constants = ezRenderContext::GetConstantBufferData<ezTonemapConstants>(m_hConstantBuffer);
        constants->AutoExposureParams.SetZero();
        constants->MoodColor = m_MoodColor;
        constants->MoodStrength = m_fMoodStrength;
        constants->Saturation = m_fSaturation;
        constants->Lut1Strength = lutStrengths[0];
        constants->Lut2Strength = lutStrengths[1];

        constants->NumLUTs = numLUTs;
        constants->TonemapMode = m_Mode.GetValue();
        constants->WhitePoint = m_fWhitePoint;
        constants->TonemapNormalization = CalculateTonemapNormalization(m_Mode);

        constants->FilmicToe = m_fFilmicToe;
        constants->FilmicLinearStrength = m_fFilmicLinearStrength;
        constants->FilmicLinearAngle = m_fFilmicLinearAngle;
        constants->FilmicShoulder = m_fFilmicShoulder;

        constants->AcesToe = m_fAcesToe;
        constants->AcesSlope = m_fAcesSlope;
        constants->AcesShoulder = m_fAcesShoulder;

        constants->AgXContrast = m_fAgXContrast;
        constants->AgXMidGrey = m_fAgXMidGrey;
        CalculateAwpConstants(constants->AgXToeA, constants->AgXSlope, constants->AgXW);

        // Pre-calculate factors of a s-shaped polynomial-function
        const float m = (0.5f - 0.5f * m_fContrast) / (0.5f + 0.5f * m_fContrast);
        const float a = 2.0f * m - 2.0f;
        const float b = -3.0f * m + 3.0f;

        constants->ContrastParams = ezVec4(a, b, m, 0.0f);
      }

      renderViewContext.m_pRenderContext->BindShader(m_hShader);
      renderViewContext.m_pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);

      ezBindGroupBuilder& bindGroup = ezRenderContext::GetDefaultInstance()->GetBindGroup();
      bindGroup.BindBuffer("ezTonemapConstants", m_hConstantBuffer);
      bindGroup.BindTexture("VignettingTexture", m_hVignettingTexture, ezResourceAcquireMode::BlockTillLoaded);
      bindGroup.BindTexture("NoiseTexture", m_hNoiseTexture, ezResourceAcquireMode::BlockTillLoaded);
      bindGroup.BindTexture("SceneColorTexture", ctx.ResolveTexture(hColorInput));
      bindGroup.BindTexture("Lut1Texture", luts[0]);
      bindGroup.BindTexture("Lut2Texture", luts[1]);

      if (!hBloomInput.IsInvalidated())
      {
        bindGroup.BindTexture("BloomTexture", ctx.ResolveTexture(hBloomInput));
      }
      else
      {
        bindGroup.BindTexture("BloomTexture", m_hBlackTexture, ezResourceAcquireMode::BlockTillLoaded);
      }

      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

      if (m_bVisualizeCurve)
      {
        VisualizeCurve(renderViewContext);
      }
    });

  return EZ_SUCCESS;
}

ezResult ezTonemapPass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  ezStringBuilder sTemp = GetVignettingTextureFile();
  inout_stream << sTemp;
  inout_stream << m_MoodColor;
  inout_stream << m_fMoodStrength;
  inout_stream << m_fSaturation;
  inout_stream << m_fContrast;
  inout_stream << m_fLut1Strength;
  inout_stream << m_fLut2Strength;
  sTemp = GetLUT1TextureFile();
  inout_stream << sTemp;
  sTemp = GetLUT2TextureFile();
  inout_stream << sTemp;

  inout_stream << m_fWhitePoint;

  inout_stream << m_Mode;
  inout_stream << m_bVisualizeCurve;
  inout_stream << m_CompareCurve;
  inout_stream << m_fFilmicToe;
  inout_stream << m_fFilmicLinearStrength;
  inout_stream << m_fFilmicLinearAngle;
  inout_stream << m_fFilmicShoulder;
  inout_stream << m_fAcesToe;
  inout_stream << m_fAcesSlope;
  inout_stream << m_fAcesShoulder;
  inout_stream << m_fAgXContrast;
  inout_stream << m_fAgXMidGrey;

  return EZ_SUCCESS;
}

ezResult ezTonemapPass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  ezStringBuilder sTemp;
  inout_stream >> sTemp;
  SetVignettingTextureFile(sTemp);
  inout_stream >> m_MoodColor;
  inout_stream >> m_fMoodStrength;
  inout_stream >> m_fSaturation;
  inout_stream >> m_fContrast;
  inout_stream >> m_fLut1Strength;
  inout_stream >> m_fLut2Strength;
  inout_stream >> sTemp;
  SetLUT1TextureFile(sTemp);
  inout_stream >> sTemp;
  SetLUT2TextureFile(sTemp);

  if (uiVersion >= 2)
  {
    inout_stream >> m_fWhitePoint;
  }

  if (uiVersion >= 3)
  {
    inout_stream >> m_Mode;
    inout_stream >> m_bVisualizeCurve;
    inout_stream >> m_CompareCurve;
    inout_stream >> m_fFilmicToe;
    inout_stream >> m_fFilmicLinearStrength;
    inout_stream >> m_fFilmicLinearAngle;
    inout_stream >> m_fFilmicShoulder;
    inout_stream >> m_fAcesToe;
    inout_stream >> m_fAcesSlope;
    inout_stream >> m_fAcesShoulder;
    inout_stream >> m_fAgXContrast;
    inout_stream >> m_fAgXMidGrey;
  }

  return EZ_SUCCESS;
}

void ezTonemapPass::SetMode(ezEnum<ezTonemapMode> mode)
{
  m_Mode = static_cast<ezTonemapMode::Enum>(ezMath::Clamp<int>(mode, ezTonemapMode::None, ezTonemapMode::COUNT - 1));
}

void ezTonemapPass::SetFilmicToe(float fToe)
{
  m_fFilmicToe = ezMath::Clamp(fToe, 0.0f, 100.0f);
}

void ezTonemapPass::SetFilmicLinearStrength(float fStrength)
{
  m_fFilmicLinearStrength = ezMath::Clamp(fStrength, 0.0f, 100.0f);
}

void ezTonemapPass::SetFilmicLinearAngle(float fAngle)
{
  m_fFilmicLinearAngle = ezMath::Clamp(fAngle, 0.0f, 100.0f);
}

void ezTonemapPass::SetFilmicShoulder(float fShoulder)
{
  m_fFilmicShoulder = ezMath::Clamp(fShoulder, 0.01f, 100.0f);
}

void ezTonemapPass::SetAcesToe(float fToe)
{
  m_fAcesToe = ezMath::Saturate(fToe);
}

void ezTonemapPass::SetAcesSlope(float fSlope)
{
  m_fAcesSlope = ezMath::Clamp(fSlope, 0.1f, 1.0f);
}

void ezTonemapPass::SetAcesShoulder(float fShoulder)
{
  m_fAcesShoulder = ezMath::Clamp(fShoulder, 0.01f, 1.0f);
}

void ezTonemapPass::SetAgXContrast(float fContrast)
{
  m_fAgXContrast = ezMath::Max(fContrast, 1.0f);
}

void ezTonemapPass::SetAgXMidGrey(float fMidGrey)
{
  m_fAgXMidGrey = ezMath::Clamp(fMidGrey, 0.1f, 0.5f);
}

float ezTonemapPass::CalculateTonemapNormalization(ezTonemapMode::Enum mode) const
{
  if (mode == ezTonemapMode::Linear)
  {
    return 1.0f / m_fWhitePoint;
  }
  else if (mode == ezTonemapMode::Reinhard)
  {
    return 1.0f / (m_fWhitePoint * m_fWhitePoint);
  }
  else if (mode == ezTonemapMode::Filmic)
  {
    return 1.0f / HableFunction(m_fWhitePoint, m_fFilmicToe, m_fFilmicLinearStrength, m_fFilmicLinearAngle, m_fFilmicShoulder);
  }
  else if (mode == ezTonemapMode::ACES)
  {
    return 1.0f / ACESFunction(m_fWhitePoint, m_fAcesToe, m_fAcesSlope, m_fAcesShoulder);
  }
  else if (mode == ezTonemapMode::AgX)
  {
    // AgX curve has normalization built in
    return 1.0f;
  }

  return 1.0f;
}

// allenwp tonemapping curve; developed for use in the Godot game engine
// Source and details: https://allenwp.com/blog/2025/05/29/allenwp-tonemapping-curve/
void ezTonemapPass::CalculateAwpConstants(float& out_AwpToeA, float& out_AwpSlope, float& out_AwpW) const
{
  float awp_contrast = m_fAgXContrast;
  float awp_high_clip = m_fWhitePoint * 0.8f;

  // This constant must match the one in the shader code.
  const float inCrossover = 0.18f;
  const float outCrossover = m_fAgXMidGrey;

  // Use one of the following four approaches to get your output_max_value:
  // Current SDR (Standard Dynamic Range) content is always normalized to 1.0, so output_max_value is always 1.0 for SDR.

  // 1) SDR
  const float reference_white_luminance_nits = 100.0;
  const float max_luminance_nits = 100.0;
  const float output_max_value = max_luminance_nits / reference_white_luminance_nits;

  // 2) Traditional HDR
  // float reference_white_luminance_nits = 100.0;
  // float max_luminance_nits = 1000.0;
  // float output_max_value = max_luminance_nits / reference_white_luminance_nits;

  // 3) Variable Extended Dynamic Range (EDR) on Apple
  // float output_max_value = maximumExtendedDynamicRangeColorComponentValue;

  // 4) Variable Extended Dynamic Range (EDR) on Windows or similar
  // float reference_white_luminance_nits = get_sdr_content_brightness_nits();
  // float max_luminance_nits = get_max_luminance_nits();
  // float output_max_value = max_luminance_nits / reference_white_luminance_nits;

  awp_high_clip = ezMath::Max(awp_high_clip, output_max_value);

  const float cPow = ezMath::Pow(inCrossover, awp_contrast);
  out_AwpToeA = ((1.0f / outCrossover) - 1.0f) * cPow;
  
  const float awp_slope_denom = cPow + out_AwpToeA;
  out_AwpSlope = (awp_contrast * ezMath::Pow(inCrossover, awp_contrast - 1.0f) * out_AwpToeA) / (awp_slope_denom * awp_slope_denom);

  const float awp_shoulder_max = output_max_value - outCrossover;
  float awp_w = awp_high_clip - inCrossover;
  awp_w = awp_w * awp_w;
  awp_w = awp_w / awp_shoulder_max;
  out_AwpW = awp_w * out_AwpSlope;
}

void ezTonemapPass::VisualizeCurve(const ezRenderViewContext& renderViewContext) const
{
  ezStringBuilder sb;

  const ezVec2 vViewportSize = renderViewContext.m_pViewData->m_ViewPortRect.GetExtents();
  const ezVec2 vTopLeft = vViewportSize.CompMul(ezVec2(0.03f));
  const ezVec2 vBottomRight = vViewportSize.CompMul(ezVec2(0.4f));
  const float fLineLength = 4.0f * ezDebugRenderer::GetTextScale();
  const ezUInt32 uiTextSize = 16;

  // Draw a background rectangle
  {
    const ezRectFloat backgroundRect(vTopLeft, vBottomRight - vTopLeft);
    ezDebugRenderer::Draw2DRectangle(*renderViewContext.m_pViewDebugContext, backgroundRect, ezGraphicsUtils::GetNearPlaneDeviceDepth(), ezColor(0.0f, 0.0f, 0.0f, 0.9f));
  }

  // Axis lines and labels
  {
    ezTempHybridArray<ezDebugRendererLine, 16> lines;

    // x-axis
    {
      lines.PushBack(ezDebugRendererLine(ezVec3(vTopLeft.x, vBottomRight.y, 0.0f), ezVec3(vBottomRight.x, vBottomRight.y, 0.0f), ezColor::White));

      for (float x = 0; x <= 1.0f; x += 0.25f)
      {
        const float x2 = x * x;
        const float xPos = ezMath::Lerp(vTopLeft.x, vBottomRight.x, x2);
        const ezVec3 lineStart = ezVec3(xPos, vBottomRight.y, 0.0f);
        const ezVec3 lineEnd = ezVec3(xPos, vBottomRight.y + fLineLength, 0.0f);
        lines.PushBack(ezDebugRendererLine(lineStart, lineEnd, ezColor::White));

        // Draw label
        sb.SetFormat("{0}", ezArgF(x * m_fWhitePoint, 1));
        ezDebugRenderer::Draw2DText(*renderViewContext.m_pViewDebugContext, sb, ezVec2I32(int(lineEnd.x), int(lineEnd.y)), ezColor::White, uiTextSize, ezDebugTextHAlign::Center, ezDebugTextVAlign::Top);
      }
    }

    // y-axis
    {
      lines.PushBack(ezDebugRendererLine(ezVec3(vTopLeft.x, vTopLeft.y, 0.0f), ezVec3(vTopLeft.x, vBottomRight.y, 0.0f), ezColor::White));

      for (float y = 0; y <= 1.0f; y += 0.25f)
      {
        const float yPos = ezMath::Lerp(vBottomRight.y, vTopLeft.y, y);
        const ezVec3 lineStart = ezVec3(vTopLeft.x, yPos, 0.0f);
        const ezVec3 lineEnd = ezVec3(vTopLeft.x - fLineLength, yPos, 0.0f);
        lines.PushBack(ezDebugRendererLine(lineStart, lineEnd, ezColor::White));

        // Draw label
        sb.SetFormat("{0}", y);
        ezDebugRenderer::Draw2DText(*renderViewContext.m_pViewDebugContext, sb, ezVec2I32(int(lineEnd.x - fLineLength), int(lineEnd.y)), ezColor::White, uiTextSize, ezDebugTextHAlign::Right, ezDebugTextVAlign::Center);
      }
    }

    ezDebugRenderer::Draw2DLines(*renderViewContext.m_pViewDebugContext, lines, ezColor::White);
  }

  // Plot the curves on the CPU with one line segment every few pixels.
  // The x-axis is quadratic (like the labels), the output is clamped to the graph area.
  auto PlotCurve = [&](ezTonemapMode::Enum mode, const ezColor& color)
  {
    const float fNormalization = CalculateTonemapNormalization(mode);

    float awp_toe_a, awp_slope, awp_w;
    CalculateAwpConstants(awp_toe_a, awp_slope, awp_w);

    auto Evaluate = [&](float x) -> float
    {
      switch (mode)
      {
        case ezTonemapMode::None:
          return x;
        case ezTonemapMode::Linear:
          return x * fNormalization;
        case ezTonemapMode::Reinhard:
          return ReinhardFunction(x, fNormalization);
        case ezTonemapMode::Filmic:
          return HableFunction(x, m_fFilmicToe, m_fFilmicLinearStrength, m_fFilmicLinearAngle, m_fFilmicShoulder) * fNormalization;
        case ezTonemapMode::ACES:
          return ACESFunction(x, m_fAcesToe, m_fAcesSlope, m_fAcesShoulder) * fNormalization;
        case ezTonemapMode::AgX:
          return AllenwpCurve(x, m_fAgXContrast, m_fAgXMidGrey, awp_toe_a, awp_slope, awp_w);
        default:
          return 0.0f;
      }
    };

    const float fGraphWidth = vBottomRight.x - vTopLeft.x;
    const ezUInt32 uiNumSegments = ezMath::Max(1u, static_cast<ezUInt32>(ezMath::Ceil(fGraphWidth / 4.0f)));

    ezTempHybridArray<ezDebugRendererLine, 256> curveLines;
    curveLines.Reserve(uiNumSegments);

    ezVec3 vPrev;
    for (ezUInt32 i = 0; i <= uiNumSegments; ++i)
    {
      const float u = static_cast<float>(i) / uiNumSegments;
      const float fInput = u * u * m_fWhitePoint;
      const float fOutput = ezMath::Clamp(Evaluate(fInput), 0.0f, 1.0f);

      const ezVec3 vPos = ezVec3(ezMath::Lerp(vTopLeft.x, vBottomRight.x, u), ezMath::Lerp(vBottomRight.y, vTopLeft.y, fOutput), 0.0f);
      if (i > 0)
      {
        curveLines.PushBack(ezDebugRendererLine(vPrev, vPos));
      }
      vPrev = vPos;
    }

    ezDebugRenderer::Draw2DLines(*renderViewContext.m_pViewDebugContext, curveLines, color);
  };

  // Plot the comparison curve
  if (m_CompareCurve != ezTonemapMode::None)
  {
    sb.SetFormat("Compare: {}", ezArgEnum(m_CompareCurve));
    ezDebugRenderer::Draw2DText(*renderViewContext.m_pViewDebugContext, sb, ezVec2I32(int((vTopLeft.x + vBottomRight.x) * 0.5f), int(vTopLeft.y)), ezColor::Lime, uiTextSize, ezDebugTextHAlign::Left, ezDebugTextVAlign::Bottom);

    PlotCurve(m_CompareCurve, ezColor::Lime);
  }

  // Plot the active curve
  {
    sb.SetFormat("Active: {}", ezArgEnum(m_Mode));
    ezDebugRenderer::Draw2DText(*renderViewContext.m_pViewDebugContext, sb, ezVec2I32(int(vTopLeft.x), int(vTopLeft.y)), ezColor::Red, uiTextSize, ezDebugTextHAlign::Left, ezDebugTextVAlign::Bottom);

    PlotCurve(m_Mode, ezColor::Red);
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_TonemapPass);
