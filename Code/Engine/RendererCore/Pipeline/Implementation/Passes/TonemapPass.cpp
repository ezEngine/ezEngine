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

    EZ_ENUM_MEMBER_PROPERTY("Mode", ezTonemapMode, m_Mode),

    EZ_ACCESSOR_PROPERTY("FilmicToe", GetFilmicToe, SetFilmicToe)->AddAttributes(new ezClampValueAttribute(0.0f, 1.0f), new ezDefaultValueAttribute(0.2f)),
    EZ_ACCESSOR_PROPERTY("FilmicLinearStrength", GetFilmicLinearStrength, SetFilmicLinearStrength)->AddAttributes(new ezClampValueAttribute(0.0f, 1.0f), new ezDefaultValueAttribute(0.35f)),
    EZ_ACCESSOR_PROPERTY("FilmicLinearAngle", GetFilmicLinearAngle, SetFilmicLinearAngle)->AddAttributes(new ezClampValueAttribute(0.0f, 1.0f), new ezDefaultValueAttribute(0.1f)),
    EZ_ACCESSOR_PROPERTY("FilmicShoulder", GetFilmicShoulder, SetFilmicShoulder)->AddAttributes(new ezClampValueAttribute(0.01f, 1.0f), new ezDefaultValueAttribute(0.15f)),

    EZ_ACCESSOR_PROPERTY("AcesToe", GetAcesToe, SetAcesToe)->AddAttributes(new ezClampValueAttribute(0.0f, 1.0f), new ezDefaultValueAttribute(0.55f)),
    EZ_ACCESSOR_PROPERTY("AcesSlope", GetAcesSlope, SetAcesSlope)->AddAttributes(new ezClampValueAttribute(0.1f, 1.0f), new ezDefaultValueAttribute(0.71f)),
    EZ_ACCESSOR_PROPERTY("AcesShoulder", GetAcesShoulder, SetAcesShoulder)->AddAttributes(new ezClampValueAttribute(0.01f, 1.0f), new ezDefaultValueAttribute(0.47f)),

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

        constants->TonemapMode = m_Mode != ezTonemapMode::None ? m_Mode.GetValue() : ezTonemapMode::Linear;
        constants->WhitePoint = m_fWhitePoint;
        constants->TonemapNormalization = CalculateTonemapNormalization(m_Mode);

        constants->FilmicToe = m_fFilmicToe;
        constants->FilmicLinearStrength = m_fFilmicLinearStrength;
        constants->FilmicLinearAngle = m_fFilmicLinearAngle;
        constants->FilmicShoulder = m_fFilmicShoulder;

        constants->AcesToe = m_fAcesToe;
        constants->AcesSlope = m_fAcesSlope;
        constants->AcesShoulder = m_fAcesShoulder;

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

      ezTempHashedString sLUTModeValues[3] = {"LUT_MODE_NONE", "LUT_MODE_ONE", "LUT_MODE_TWO"};
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LUT_MODE", sLUTModeValues[numLUTs]);

      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

      VisualizeCurve(renderViewContext);
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
  }

  return EZ_SUCCESS;
}

void ezTonemapPass::SetFilmicToe(float fToe)
{
  m_fFilmicToe = ezMath::Saturate(fToe);
}

void ezTonemapPass::SetFilmicLinearStrength(float fStrength)
{
  m_fFilmicLinearStrength = ezMath::Saturate(fStrength);
}

void ezTonemapPass::SetFilmicLinearAngle(float fAngle)
{
  m_fFilmicLinearAngle = ezMath::Saturate(fAngle);
}

void ezTonemapPass::SetFilmicShoulder(float fShoulder)
{
  m_fFilmicShoulder = ezMath::Clamp(fShoulder, 0.01f, 1.0f);
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
  else
  {
    return 1.0f;
  }
}

void ezTonemapPass::VisualizeCurve(const ezRenderViewContext& renderViewContext) const
{
  if (!m_bVisualizeCurve)
    return;

  ezStringBuilder sb;

  const ezVec2 vViewportSize = renderViewContext.m_pViewData->m_ViewPortRect.GetExtents();
  const ezVec2 vTopLeft = vViewportSize.CompMul(ezVec2(0.03f));
  const ezVec2 vBottomRight = vViewportSize.CompMul(ezVec2(0.4f));
  const float fLineLength = 4.0f * ezDebugRenderer::GetTextScale();
  const ezUInt32 uiTextSize = 16;

  // Draw a background rectangle
  {
    const ezRectFloat backgroundRect(vTopLeft, vBottomRight - vTopLeft);
    ezDebugRenderer::Draw2DRectangle(*renderViewContext.m_pViewDebugContext, backgroundRect, 1.0f, ezColor(0.0f, 0.0f, 0.0f, 0.9f));
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
    if (mode == ezTonemapMode::None)
      mode = ezTonemapMode::Linear;

    const float fNormalization = CalculateTonemapNormalization(mode);

    auto Evaluate = [&](float x) -> float
    {
      switch (mode)
      {
        case ezTonemapMode::Linear:
          return x * fNormalization;
        case ezTonemapMode::Reinhard:
          return ReinhardFunction(x, fNormalization);
        case ezTonemapMode::Filmic:
          return HableFunction(x, m_fFilmicToe, m_fFilmicLinearStrength, m_fFilmicLinearAngle, m_fFilmicShoulder) * fNormalization;
        case ezTonemapMode::ACES:
          // The ACES input and output matrices preserve gray values, so they can be skipped for the curve.
          return ACESFunction(x, m_fAcesToe, m_fAcesSlope, m_fAcesShoulder) * fNormalization;
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
    ezReflectionUtils::EnumerationToString(m_CompareCurve, sb, ezReflectionUtils::EnumConversionMode::ValueNameOnly);
    sb.Prepend("Compare: ");
    ezDebugRenderer::Draw2DText(*renderViewContext.m_pViewDebugContext, sb, ezVec2I32(int((vTopLeft.x + vBottomRight.x) * 0.5f), int(vTopLeft.y)), ezColor::Lime, uiTextSize, ezDebugTextHAlign::Left, ezDebugTextVAlign::Bottom);

    PlotCurve(m_CompareCurve, ezColor::Lime);
  }

  // Plot the active curve
  {
    ezReflectionUtils::EnumerationToString(m_Mode, sb, ezReflectionUtils::EnumConversionMode::ValueNameOnly);
    sb.Prepend("Active: ");
    ezDebugRenderer::Draw2DText(*renderViewContext.m_pViewDebugContext, sb, ezVec2I32(int(vTopLeft.x), int(vTopLeft.y)), ezColor::Red, uiTextSize, ezDebugTextHAlign::Left, ezDebugTextVAlign::Bottom);

    PlotCurve(m_Mode, ezColor::Red);
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_TonemapPass);
