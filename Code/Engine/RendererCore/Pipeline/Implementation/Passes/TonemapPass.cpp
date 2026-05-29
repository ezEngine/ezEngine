#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/TonemapPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/TonemapConstants.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTonemapPass, 2, ezRTTIDefaultAllocator<ezTonemapPass>)
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

  m_MoodColor = ezColor::Orange;
  m_fMoodStrength = 0.0f;
  m_fSaturation = 1.0f;
  m_fContrast = 1.0f;
  m_fLut1Strength = 1.0f;
  m_fLut2Strength = 0.0f;
  m_fWhitePoint = 11.2f;

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
  pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
    {
    const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();
    renderViewContext.UpdateViewport();

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
      constants->WhitePoint = m_fWhitePoint;

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
    bindGroup.BindTexture("BloomTexture", hBloomInput.IsInvalidated() ? ezGALTextureHandle() : ctx.ResolveTexture(hBloomInput));
    bindGroup.BindTexture("Lut1Texture", luts[0]);
    bindGroup.BindTexture("Lut2Texture", luts[1]);

    ezTempHashedString sLUTModeValues[3] = {"LUT_MODE_NONE", "LUT_MODE_ONE", "LUT_MODE_TWO"};
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LUT_MODE", sLUTModeValues[numLUTs]);

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });

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

  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_TonemapPass);
